
#include "Visualizer/Renderer/ColumnWindowsConsole.h"

#include <threads.h>
#include <stdalign.h>
#include <string.h>
#include <Windows.h>

#include "Utils/Common.h"
#include "Utils/GuardedMalloc.h"
#include "Utils/LinkedList.h"
#include "Utils/MemoryPool.h"
#include "Utils/SharedLock.h"
#include "Utils/SpinLock.h"
#include "Utils/Time.h"

// Buffer stuff
static HANDLE hAltBuffer = NULL;
static CONSOLE_SCREEN_BUFFER_INFOEX BufferInfo = { 0 };
static CHAR_INFO* aBufferCache;
SMALL_RECT UpdatedRect;

// Unicode support is not going to be added
// as it's too slow with current limitations.
// https://github.com/microsoft/terminal/discussions/13339
// https://github.com/microsoft/terminal/issues/10810#issuecomment-897800855
//

// Console Attr
// cmd "color /?" explains this very well.
#define ATTR_WINCON_BACKGROUND 0x0FU
#define ATTR_WINCON_NORMAL     0x78U
#define ATTR_WINCON_READ       0x1EU
#define ATTR_WINCON_WRITE      0x4BU
#define ATTR_WINCON_POINTER    0x3CU
#define ATTR_WINCON_CORRECT    0x4BU
#define ATTR_WINCON_INCORRECT  0x2EU

// For uninitialization
static HANDLE hOldBuffer;
static LONG_PTR OldWindowStyle;

// Abuse two's complement's addition property to prevent overflow
static_assert(-123456789 == ~123456789 + 1);

// Array
typedef struct {
	isort_t Value;
	uint8_t aMarkerCount[Visualizer_MarkerAttribute_EnumCount];
} ArrayMember;

typedef struct {
	atomic bool bUpdated;
	sharedlock SharedLock;
	// Worst case: 2^24 members each containing 2^8 repeated attributes
	// This will break rendering but will still work due to unsigned integer wrapping.
	intptr_t MemberCount;
	atomic long_isort_t ValueSum;
	atomic uint32_t aMarkerCount[Visualizer_MarkerAttribute_EnumCount];
} ColumnInfoAtomic;

typedef struct {
	non_atomic(bool) bUpdated;
	sharedlock SharedLock;

	intptr_t MemberCount;
	non_atomic(long_isort_t) ValueSum;
	non_atomic(uint32_t) aMarkerCount[Visualizer_MarkerAttribute_EnumCount];
} ColumnInfoNonAtomic;

typedef union {
	ColumnInfoAtomic Atomic;
	ColumnInfoNonAtomic NonAtomic;
} ColumnInfo;

static_assert(sizeof(ColumnInfo) == sizeof(ColumnInfoNonAtomic));

typedef struct {
	llist_node      Node;
	atomic bool     bRemoved;
	atomic bool     bResized;
	// atomic bool     bRangeUpdated;
	atomic intptr_t Size;
	atomic isort_t  ValueMin;
	atomic isort_t  ValueMax;
	ArrayMember*    aState;

	intptr_t        ColumnCount;
	ColumnInfo*     aColumn;

	// Statistics
	// Using global counters makes the renderer prone to desync
	// and also increases contention but that saves a lot of memory
	atomic uint64_t ReadCount;
	atomic uint64_t WriteCount;
} ArrayProp;

static pool ArrayPropPool;
static ArrayProp* pArrayPropHead;

thrd_t RenderThread;
static atomic bool bRun;

spinlock AlgorithmNameLock;
char* sAlgorithmName; // NULL terminated

static Visualizer_Handle PoolIndexToHandle(pool_index PoolIndex) {
	return (Visualizer_Handle)(PoolIndex + 1);
}

static pool_index HandleToPoolIndex(Visualizer_Handle hHandle) {
	return (pool_index)hHandle - 1;
}

static void* GetHandleData(pool* pPool, Visualizer_Handle hHandle) {
	return PoolIndexToAddress(pPool, HandleToPoolIndex(hHandle));
}

static bool ValidateHandle(pool* pPool, Visualizer_Handle hHandle) {
	return ((pool_index)hHandle > 0) & ((pool_index)hHandle <= pPool->nBlock);
}

// Returns iB
static inline intptr_t NearestNeighborScale(intptr_t iA, intptr_t nA, intptr_t nB) {
	// Worst case: If nB = 2^15 then nA can only be 2^48
	return ((int64_t)iA * nB + ((uintptr_t)nB / 2)) / nA;
}

static void UpdateCellCacheRow(intptr_t iRow, const char* sText, intptr_t nText) {
	if (iRow >= BufferInfo.dwSize.Y)
		return;
	if (nText > BufferInfo.dwSize.X)
		nText = BufferInfo.dwSize.X;
	// Use wide char to avoid conversion
	int16_t i = 0;
	for (; i < nText; ++i)
		aBufferCache[BufferInfo.dwSize.X * iRow + i].Char.UnicodeChar = (wchar_t)sText[i];
	for (; i < BufferInfo.dwSize.X; ++i)
		aBufferCache[BufferInfo.dwSize.X * iRow + i].Char.UnicodeChar = L' ';

	if (iRow < UpdatedRect.Top)
		UpdatedRect.Top = (int16_t)iRow;
	UpdatedRect.Left = 0;
	if (iRow > UpdatedRect.Bottom)
		UpdatedRect.Bottom = (int16_t)iRow;
	if (nText > UpdatedRect.Right)
		UpdatedRect.Right = (int16_t)nText;
}

static const USHORT aWinConAttrTable[Visualizer_MarkerAttribute_EnumCount] = {
	ATTR_WINCON_POINTER,
	ATTR_WINCON_READ,
	ATTR_WINCON_WRITE,
	ATTR_WINCON_CORRECT,
	ATTR_WINCON_INCORRECT
};

typedef struct {
	bool bNeedUpdate;
	uint16_t ConsoleAttr;
	int16_t Height;
} ColumnUpdateParam;

static ColumnUpdateParam GetColumnUpdateParam(ArrayProp* pArrayProp, intptr_t iColumn) {
	if (!pArrayProp->aColumn[iColumn].Atomic.bUpdated)
		return (ColumnUpdateParam){ false, 0, 0 };

	// Choose the correct value & attribute

	ColumnInfoNonAtomic* pColumn = &pArrayProp->aColumn[iColumn].NonAtomic;
	long_isort_t ValueSum;
	uint32_t aMarkerCount[Visualizer_MarkerAttribute_EnumCount];

	sharedlock_lock_exclusive(&pColumn->SharedLock);

	pColumn->bUpdated = false;
	ValueSum = pColumn->ValueSum;
	memcpy(aMarkerCount, (uint32_t*)&pColumn->aMarkerCount, sizeof(aMarkerCount));

	sharedlock_unlock_exclusive(&pColumn->SharedLock);

	// NOTE: Precision loss: This force it to have a precision = range
	// Fix is possible but does it worth it?
	isort_t Value = (isort_t)(ValueSum / pColumn->MemberCount);

	// Find the attribute that have the most occurrences. TODO: SIMD
	uint8_t MaxAttrCount = aMarkerCount[0];
	Visualizer_MarkerAttribute Attr = 0;
	for (uint8_t i = 1; i < Visualizer_MarkerAttribute_EnumCount; ++i) {
		uint8_t Count = aMarkerCount[i];
		if (Count >= MaxAttrCount) {
			MaxAttrCount = Count;
			Attr = i;
		}
	}

	uint16_t ConsoleAttr;
	if (MaxAttrCount == 0)
		ConsoleAttr = ATTR_WINCON_NORMAL;
	else
		ConsoleAttr = aWinConAttrTable[Attr];

	// Use wrap around characteristic of unsigned integer
	usort_t ValueMin = pArrayProp->ValueMin;
	usort_t AbsoluteValue = (usort_t)Value - ValueMin;
	usort_t AbsoluteValueMax = (usort_t)pArrayProp->ValueMax - ValueMin;
	if (AbsoluteValue > AbsoluteValueMax)
		AbsoluteValue = AbsoluteValueMax;

	// Scale the value to the corresponding screen height

	SHORT Height = (SHORT)((long_usort_t)AbsoluteValue * BufferInfo.dwSize.Y / AbsoluteValueMax);

	return (ColumnUpdateParam){ true, ConsoleAttr, Height };
}

static void UpdateCellCacheColumn(int16_t iConsoleColumn, ColumnUpdateParam Parameter) {
	if (!Parameter.bNeedUpdate)
		return;

	UpdatedRect.Top = 0;
	if (iConsoleColumn < UpdatedRect.Left)
		UpdatedRect.Left = (SHORT)iConsoleColumn;
	UpdatedRect.Bottom = BufferInfo.dwSize.Y - 1;
	if (iConsoleColumn > UpdatedRect.Right)
		UpdatedRect.Right = (SHORT)iConsoleColumn;

	intptr_t i = 0;
	for (; i < (intptr_t)(BufferInfo.dwSize.Y - Parameter.Height); ++i)
		aBufferCache[BufferInfo.dwSize.X * i + iConsoleColumn].Attributes = ATTR_WINCON_BACKGROUND;
	for (; i < BufferInfo.dwSize.Y; ++i)
		aBufferCache[BufferInfo.dwSize.X * i + iConsoleColumn].Attributes = Parameter.ConsoleAttr;
}

static void ClearScreen() {
	LONG BufferSize = BufferInfo.dwSize.X * BufferInfo.dwSize.Y;
	for (intptr_t i = 0; i < BufferSize; ++i) {
		aBufferCache[i].Char.UnicodeChar = ' ';
		aBufferCache[i].Attributes = ATTR_WINCON_BACKGROUND;
	}
	SMALL_RECT Rect = {
		0,
		0,
		BufferInfo.dwSize.X - 1,
		BufferInfo.dwSize.Y - 1,
	};
	WriteConsoleOutputW(
		hAltBuffer,
		aBufferCache,
		BufferInfo.dwSize,
		(COORD){ 0, 0 },
		&Rect
	);
}

static_assert(sizeof(uintptr_t) <= 8);

// Length of sString must be as least 20
static intptr_t Uint64ToString(uint64_t X, char* sString) {
	if (X == 0) {
		sString[0] = '0';
		return 1;
	}

	intptr_t i = 0;
	for (; X > 0; X /= 10)
		sString[i++] = '0' + (X % 10);
	intptr_t Length = i;

	intptr_t ii = 0;
	--i;
	while (ii < i) {
		swap(&sString[ii], &sString[i]);
		++ii;
		--i;
	}

	return Length;
}

static int RenderThreadMain(void* pData) {

	// microseconds
	uint64_t ClockResolution = clock64_resolution();
	uint64_t ThreadTimeStart = clock64();
	uint64_t UpdateInterval = 15625; // 64 FPS

	uint64_t FpsUpdateInterval = 500000; // FPS counter interval
	uint64_t FpsUpdateCount = 0;
	uint64_t FramesRendered = 0;

	while (bRun) {
		uint64_t ThreadDuration = (clock64() - ThreadTimeStart) * 1000000 / ClockResolution;
		
		// TODO: Multi array
		if (!pArrayPropHead) {
			continue;
		}

		ArrayProp* pArrayProp = pArrayPropHead;

		// Remove
		if (pArrayProp->bRemoved) {
			free(pArrayProp->aState);
			PoolDeallocateAddress(&ArrayPropPool, pArrayProp);
			pArrayPropHead = NULL;

			ClearScreen();
			continue;
		}

		// TODO: Resize window
		// Resize array

		intptr_t Size = pArrayProp->Size;
		if (pArrayProp->bResized) {

			for (intptr_t iColumn = 0; iColumn < pArrayProp->ColumnCount; ++iColumn)
				pArrayProp->aColumn[iColumn].NonAtomic.MemberCount = 0;

			for (intptr_t iMember = 0; iMember < Size; ++iMember) {
				intptr_t iColumn = NearestNeighborScale(iMember, Size, pArrayProp->ColumnCount);
				++pArrayProp->aColumn[iColumn].NonAtomic.MemberCount;
			}

			intptr_t iMember = 0;
			for (intptr_t iColumn = 0; iColumn < pArrayProp->ColumnCount; ++iColumn) {

				ColumnInfoNonAtomic* pColumn = &pArrayProp->aColumn[iColumn].NonAtomic;

				sharedlock_lock_exclusive(&pColumn->SharedLock);

				// Reset values
				pColumn->ValueSum = 0;
				memset(
					(uint8_t*)pColumn->aMarkerCount,
					0,
					Visualizer_MarkerAttribute_EnumCount * sizeof(*pColumn->aMarkerCount)
				);

				// Regenerate values
				intptr_t iEndMember = iMember + pColumn->MemberCount;
				for (; iMember < iEndMember; ++iMember) {
					ArrayMember* pMember = &pArrayProp->aState[iMember];

					pColumn->ValueSum += pMember->Value;
					for (uint8_t i = 0; i < Visualizer_MarkerAttribute_EnumCount; ++i)
						pColumn->aMarkerCount[i] += pMember->aMarkerCount[i]; // FIXME: Doesn't have to be atomic
					// TODO: Add non-atomic ArrayProp
				}
				pColumn->bUpdated = true;

				sharedlock_unlock_exclusive(&pColumn->SharedLock);

			}

			pArrayProp->bResized = false;

		}

		// Update cell cache columns

		UpdatedRect = (SMALL_RECT){
			BufferInfo.dwSize.X - 1,
			BufferInfo.dwSize.Y - 1,
			0,
			0
		};

		{
			ColumnUpdateParam UpdateParam = { false, 0, 0 };
			intptr_t iColumnOld = -1;
			for (int16_t i = 0; i < BufferInfo.dwSize.X; ++i) {
				intptr_t iColumn = NearestNeighborScale(i, BufferInfo.dwSize.X, pArrayProp->ColumnCount);
				if (iColumn > iColumnOld) {
					UpdateParam = GetColumnUpdateParam(pArrayProp, iColumn);
					iColumnOld = iColumn;
				}
				UpdateCellCacheColumn(i, UpdateParam);
			}
		}

		// Update cell text rows

		// Algorithm name
		spinlock_lock(&AlgorithmNameLock);

		char* sAlgorithmNameTemp = sAlgorithmName;
		if (sAlgorithmNameTemp == NULL)
			sAlgorithmNameTemp = "";
		UpdateCellCacheRow(0, sAlgorithmNameTemp, strlen(sAlgorithmNameTemp));

		spinlock_unlock(&AlgorithmNameLock);

		intptr_t Length;
		intptr_t NumberLength;

		// FPS
		uint64_t NewFpsUpdateCount = ThreadDuration / FpsUpdateInterval;
		if (NewFpsUpdateCount > FpsUpdateCount) {

			char aFpsString[48] = "FPS: ";
			Length = static_strlen("FPS: ");
			NumberLength = Uint64ToString(
				FramesRendered * 1000000 / ((NewFpsUpdateCount - FpsUpdateCount) * FpsUpdateInterval),
				aFpsString + Length
			);
			Length += NumberLength;
			UpdateCellCacheRow(1, aFpsString, Length);

			FpsUpdateCount = NewFpsUpdateCount;
			FramesRendered = 0;

		}

		// Array
		char aArrayIndexString[48] = "Array #";
		Length = static_strlen("Array #");
		NumberLength = Uint64ToString(
			PoolAddressToIndex(&ArrayPropPool, pArrayProp),
			aArrayIndexString + Length
		);
		Length += NumberLength;
		UpdateCellCacheRow(3, aArrayIndexString, Length);

		// Size
		char aSizeString[48] = "Size: ";
		Length = static_strlen("Size: ");
		NumberLength = Uint64ToString(pArrayProp->Size, aSizeString + Length);
		Length += NumberLength;
		UpdateCellCacheRow(5, aSizeString, Length);

		// Reads
		char aReadCountString[48] = "Reads: ";
		Length = static_strlen("Reads: ");
		NumberLength = Uint64ToString(pArrayProp->ReadCount, aReadCountString + Length);
		Length += NumberLength;
		UpdateCellCacheRow(6, aReadCountString, Length);

		// Writee
		char aWriteCountString[48] = "Writes: ";
		Length = static_strlen("Writes: ");
		NumberLength = Uint64ToString(pArrayProp->WriteCount, aWriteCountString + Length);
		Length += NumberLength;
		UpdateCellCacheRow(7, aWriteCountString, Length);

		WriteConsoleOutputW(
			hAltBuffer,
			aBufferCache,
			BufferInfo.dwSize,
			(COORD){ UpdatedRect.Left , UpdatedRect.Top },
			&UpdatedRect
		);
		++FramesRendered;

		ThreadDuration = (clock64() - ThreadTimeStart) * 1000000 / ClockResolution;
		sleep64(UpdateInterval - (ThreadDuration % UpdateInterval));
	}

	return 0;

}

void RendererCwc_Initialize() {

	PoolInitialize(&ArrayPropPool, 16, sizeof(ArrayProp));
	sAlgorithmName = NULL;

	// New window style

	HWND hWindow = GetConsoleWindow();
	OldWindowStyle = GetWindowLongPtrW(hWindow, GWL_STYLE);
	SetWindowLongPtrW(
		hWindow,
		GWL_STYLE,
		OldWindowStyle & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX
	);
	SetWindowPos(
		hWindow,
		NULL,
		0,
		0,
		0,
		0,
		SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE
	);

	// New buffer

	hAltBuffer = CreateConsoleScreenBuffer(
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		CONSOLE_TEXTMODE_BUFFER,
		NULL
	);
	hOldBuffer = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleActiveScreenBuffer(hAltBuffer);

	// Set cursor to top left

	BufferInfo.cbSize = sizeof(BufferInfo);
	GetConsoleScreenBufferInfoEx(hAltBuffer, &BufferInfo);
	BufferInfo.dwCursorPosition = (COORD){ 0, 0 };
	BufferInfo.wAttributes = ATTR_WINCON_BACKGROUND;
	SetConsoleScreenBufferInfoEx(hAltBuffer, &BufferInfo);

	// Initialize buffer cache

	LONG BufferSize = BufferInfo.dwSize.X * BufferInfo.dwSize.Y;
	aBufferCache = malloc_guarded(BufferSize * sizeof(*aBufferCache));
	ClearScreen();

	// Render thread

	bRun = true;
	thrd_create(&RenderThread, RenderThreadMain, NULL);

}

void RendererCwc_Uninitialize() {

	// Stop render thread

	bRun = false;
	int ThreadReturn;
	thrd_join(RenderThread, &ThreadReturn);

	// Free alternate buffer

	SetConsoleActiveScreenBuffer(hOldBuffer);
	CloseHandle(hAltBuffer);

	// Restore window mode

	HWND hWindow = GetConsoleWindow();
	SetWindowLongPtrW(hWindow, GWL_STYLE, OldWindowStyle);
	SetWindowPos(hWindow, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE);

	free(aBufferCache);

	// Free arrays & markers

	PoolDestroy(&ArrayPropPool);

}

Visualizer_Handle RendererCwc_AddArray(
	intptr_t Size,
	isort_t* aArrayState,
	isort_t ValueMin,
	isort_t ValueMax
) {
	pool_index Index = PoolAllocate(&ArrayPropPool);
	if (Index == POOL_INVALID_INDEX) return NULL;
	ArrayProp* pArrayProp = PoolIndexToAddress(&ArrayPropPool, Index);

	if (Size < 1) return NULL; // TODO: Allow this

	pArrayProp->Size = Size;
	pArrayProp->aState = malloc_guarded(Size * sizeof(*pArrayProp->aState));
	pArrayProp->ColumnCount = min2(Size, BufferInfo.dwSize.X);
	pArrayProp->aColumn = calloc_guarded(pArrayProp->ColumnCount, sizeof(*pArrayProp->aColumn));
	if (aArrayState)
		for (intptr_t i = 0; i < Size; ++i)
			pArrayProp->aState[i] = (ArrayMember){ aArrayState[i] };
	else
		for (intptr_t i = 0; i < Size; ++i)
			pArrayProp->aState[i] = (ArrayMember){ 0 };

	pArrayProp->ValueMin = ValueMin;
	pArrayProp->ValueMax = ValueMax;
	pArrayProp->bRemoved = false;
	pArrayProp->bResized = true;

	pArrayProp->ReadCount = 0;
	pArrayProp->WriteCount = 0;

	if (!pArrayPropHead) // TODO: Multi array
		pArrayPropHead = pArrayProp;
	return PoolIndexToHandle(Index);
}

void RendererCwc_RemoveArray(Visualizer_Handle hArray) {
	assert(ValidateHandle(&ArrayPropPool, hArray));
	pool_index Index = HandleToPoolIndex(hArray);
	ArrayProp* pArrayProp = PoolIndexToAddress(&ArrayPropPool, Index);

	pArrayProp->bRemoved = true;
}

// UNSUPPORTED: Multiple threads on the same member
typedef uint8_t MemberUpdateType;
#define MemberUpdateType_No              0 // Not used
#define MemberUpdateType_Attribute      (1 << 0)
#define MemberUpdateType_Value          (1 << 1)

static inline void UpdateMember(
	ArrayProp* pArrayProp,
	intptr_t iPosition,
	MemberUpdateType UpdateType,
	bool bAddAttribute,
	Visualizer_MarkerAttribute Attribute,
	isort_t Value
) {
	ArrayMember* pArrayMember = &pArrayProp->aState[iPosition];

	// Most branches are known at compile time and can be optimized

	intptr_t iColumn = NearestNeighborScale(iPosition, pArrayProp->Size, pArrayProp->ColumnCount);
	ColumnInfoAtomic* pColumn = &pArrayProp->aColumn[iColumn].Atomic;

	// This lock is put up here for the render thread to handle window resize
	sharedlock_lock_shared(&pColumn->SharedLock);

	if (UpdateType & MemberUpdateType_Attribute) {
		if (bAddAttribute)
			++pArrayMember->aMarkerCount[Attribute];
		else
			--pArrayMember->aMarkerCount[Attribute];
	}

	isort_t OldValue = Value;
	if (UpdateType & MemberUpdateType_Value)
		swap(&pArrayMember->Value, &OldValue);

	if (UpdateType & MemberUpdateType_Value)
		pColumn->ValueSum += (long_isort_t)Value - OldValue;
	if (UpdateType & MemberUpdateType_Attribute) {
		if (bAddAttribute)
			++pColumn->aMarkerCount[Attribute];
		else
			--pColumn->aMarkerCount[Attribute];
	}
	pColumn->bUpdated = true;

	sharedlock_unlock_shared(&pColumn->SharedLock);
};

void RendererCwc_UpdateArrayState(Visualizer_Handle hArray, isort_t* aState) {
	ArrayProp* pArrayProp = GetHandleData(&ArrayPropPool, hArray);
	for (intptr_t i = 0; i < pArrayProp->Size; ++i)
		UpdateMember(pArrayProp, i, MemberUpdateType_Value, false, 0, aState[i]);
}

/*
void RendererCwc_UpdateArray(
	Visualizer_Handle hArray,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
) {

	assert(ValidateHandle(&ArrayPropPool, hArray));
	pool_index Index = HandleToPoolIndex(hArray);
	ArrayProp* pArrayProp = PoolIndexToAddress(&ArrayPropPool, Index);

	// Clear screen

	uint32_t Written;
	FillConsoleOutputAttribute(
		hAltBuffer,
		ATTR_WINCON_BACKGROUND,
		BufferInfo.dwSize.X * BufferInfo.dwSize.Y,
		(COORD){ 0, 0 },
		&Written
	);

	pArrayProp->ValueMin = ValueMin;
	pArrayProp->ValueMax = ValueMax;

	// Handle array resize

	if ((NewSize > 0) && (NewSize != pArrayProp->Size)) {

		// Realloc arrays

		isort_t* aResizedArrayState = realloc_guarded(
			pArrayProp->aState,
			NewSize * sizeof(isort_t)
		);
		Visualizer_MarkerAttribute* aResizedAttribute = realloc_guarded(
			pArrayProp->aAttribute,
			NewSize * sizeof(Visualizer_MarkerAttribute)
		);

		// Initialize the new part

		intptr_t OldSize = pArrayProp->Size;

		for (intptr_t i = OldSize; i < NewSize; ++i)
			aResizedArrayState[i] = 0;

		for (intptr_t i = OldSize; i < NewSize; ++i)
			aResizedAttribute[i] = Visualizer_MarkerAttribute_Normal;

		pArrayProp->aState = aResizedArrayState;
		pArrayProp->aAttribute = aResizedAttribute;
		pArrayProp->Size = NewSize;

	}

	// Re-render with new props

	Visualizer_UpdateRequest UpdateRequest;
	UpdateRequest.iArray = ArrayIndex;
	UpdateRequest.UpdateType = Visualizer_UpdateType_NoUpdate;
	intptr_t Size = pArrayProp->Size;
	for (intptr_t i = 0; i < Size; ++i) {
		UpdateRequest.iPosition = i;
		RendererCwc_UpdateItem(&UpdateRequest);
	}

	return;

}
*/

// Marker

typedef struct {
	ArrayProp* pArrayProp;
	intptr_t iPosition;
	Visualizer_MarkerAttribute Attribute;
} MarkerProp;

static MarkerProp AddMarker(
	ArrayProp* pArrayProp,
	intptr_t iPosition,
	Visualizer_MarkerAttribute Attribute
) {

	UpdateMember(pArrayProp, iPosition, MemberUpdateType_Attribute, true, Attribute, 0);

	return (MarkerProp){ pArrayProp, iPosition, Attribute };
}

static MarkerProp AddMarkerWithValue(
	ArrayProp* pArrayProp,
	intptr_t iPosition,
	Visualizer_MarkerAttribute Attribute,
	isort_t Value
) {
	UpdateMember(
		pArrayProp,
		iPosition,
		MemberUpdateType_Attribute | MemberUpdateType_Value,
		true,
		Attribute,
		Value
	);
	return (MarkerProp){ pArrayProp, iPosition, Attribute };
}

static void RemoveMarker(MarkerProp Marker) {
	UpdateMember(Marker.pArrayProp, Marker.iPosition, MemberUpdateType_Attribute, false, Marker.Attribute, 0);
}

// It does not update the iPosition member
static void MoveMarker(MarkerProp Marker, intptr_t iNewPosition) {
	UpdateMember(Marker.pArrayProp, Marker.iPosition, MemberUpdateType_Attribute, false, Marker.Attribute, 0);
	UpdateMember(Marker.pArrayProp, iNewPosition, MemberUpdateType_Attribute, true, Marker.Attribute, 0);
}

#define VISUALIZER_DISABLE_SLEEP
#ifndef VISUALIZER_DISABLE_SLEEP

static const uint64_t DefaultDelay = 10000; // microseconds

static void SleepByMultiplier(double fSleepMultiplier) {
	sleep64((uint64_t)((double)DefaultDelay * fSleepMultiplier));
}

#else
	#define SleepByMultiplier(X) 
#endif

// Read & Write

void RendererCwc_UpdateRead(Visualizer_Handle hArray, intptr_t iPosition, double fSleepMultiplier) {
	ArrayProp* pArrayProp = GetHandleData(&ArrayPropPool, hArray);

	pArrayProp->ReadCount += 1;
	MarkerProp Marker = AddMarker(pArrayProp, iPosition, Visualizer_MarkerAttribute_Read);
	SleepByMultiplier(fSleepMultiplier);
	RemoveMarker(Marker);
}

// Update 2 items (used for comparisions).
void RendererCwc_UpdateRead2(Visualizer_Handle hArray, intptr_t iPositionA, intptr_t iPositionB, double fSleepMultiplier) {
	ArrayProp* pArrayProp = GetHandleData(&ArrayPropPool, hArray);

	pArrayProp->ReadCount += 2;
	MarkerProp MarkerA = AddMarker(pArrayProp, iPositionA, Visualizer_MarkerAttribute_Read);
	MarkerProp MarkerB = AddMarker(pArrayProp, iPositionB, Visualizer_MarkerAttribute_Read);
	SleepByMultiplier(fSleepMultiplier);
	RemoveMarker(MarkerA);
	RemoveMarker(MarkerB);
}

void RendererCwc_UpdateReadMulti(
	Visualizer_Handle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	double fSleepMultiplier
) {
	ArrayProp* pArrayProp = GetHandleData(&ArrayPropPool, hArray);

	assert(Length <= 16);

	pArrayProp->ReadCount += Length;
	MarkerProp aMarker[16];
	for (intptr_t i = 0; i < Length; ++i)
		aMarker[i] = AddMarker(pArrayProp, iStartPosition + i, Visualizer_MarkerAttribute_Read);
	SleepByMultiplier(fSleepMultiplier);
	for (intptr_t i = 0; i < Length; ++i)
		RemoveMarker(aMarker[i]);
}

void RendererCwc_UpdateWrite(Visualizer_Handle hArray, intptr_t iPosition, isort_t NewValue, double fSleepMultiplier) {
	ArrayProp* pArrayProp = GetHandleData(&ArrayPropPool, hArray);

	pArrayProp->WriteCount += 1;
	MarkerProp Marker = AddMarkerWithValue(pArrayProp, iPosition, Visualizer_MarkerAttribute_Write, NewValue);
	SleepByMultiplier(fSleepMultiplier);
	RemoveMarker(Marker);
}

void RendererCwc_UpdateWrite2(
	Visualizer_Handle hArray,
	intptr_t iPositionA,
	intptr_t iPositionB,
	isort_t NewValueA,
	isort_t NewValueB,
	double fSleepMultiplier
) {
	ArrayProp* pArrayProp = GetHandleData(&ArrayPropPool, hArray);

	pArrayProp->WriteCount += 2;
	MarkerProp MarkerA = AddMarkerWithValue(pArrayProp, iPositionA, Visualizer_MarkerAttribute_Write, NewValueA);
	MarkerProp MarkerB = AddMarkerWithValue(pArrayProp, iPositionB, Visualizer_MarkerAttribute_Write, NewValueB);
	SleepByMultiplier(fSleepMultiplier);
	RemoveMarker(MarkerA);
	RemoveMarker(MarkerB);
}

void RendererCwc_UpdateWriteMulti(
	Visualizer_Handle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	isort_t* aNewValue,
	double fSleepMultiplier
) {
	ArrayProp* pArrayProp = GetHandleData(&ArrayPropPool, hArray);

	assert(Length <= 16);

	pArrayProp->WriteCount += Length;
	MarkerProp aMarker[16];
	for (intptr_t i = 0; i < Length; ++i)
		aMarker[i] = AddMarkerWithValue(pArrayProp, iStartPosition + i, Visualizer_MarkerAttribute_Write, aNewValue[i]);
	SleepByMultiplier(fSleepMultiplier);
	for (intptr_t i = 0; i < Length; ++i)
		RemoveMarker(aMarker[i]);
}

// Pointer

Visualizer_Pointer RendererCwc_CreatePointer(Visualizer_Handle hArray, intptr_t iPosition) {
	ArrayProp* pArrayProp = GetHandleData(&ArrayPropPool, hArray);
	MarkerProp Marker = AddMarker(pArrayProp, iPosition, Visualizer_MarkerAttribute_Pointer);
	return (Visualizer_Pointer){ hArray, Marker.iPosition, Marker.Attribute };
}

void RendererCwc_RemovePointer(Visualizer_Pointer Pointer) {
	ArrayProp* pArrayProp = GetHandleData(&ArrayPropPool, Pointer.hArray);
	RemoveMarker((MarkerProp){ pArrayProp, Pointer.iPosition, Pointer.Attribute });
}

void RendererCwc_MovePointer(Visualizer_Pointer* pPointer, intptr_t iNewPosition) {
	ArrayProp* pArrayProp = GetHandleData(&ArrayPropPool, pPointer->hArray);
	MarkerProp Marker = (MarkerProp){ pArrayProp, pPointer->iPosition, pPointer->Attribute };
	MoveMarker(Marker, iNewPosition);
	pPointer->iPosition = iNewPosition;
}

void RendererCwc_SetAlgorithmName(char* sAlgorithmNameArg) {
	intptr_t Size = strlen(sAlgorithmNameArg) + 1;
	char* sAlgorithmNameTemp = malloc_guarded(Size * sizeof(*sAlgorithmNameArg));
	memcpy(sAlgorithmNameTemp, sAlgorithmNameArg, Size * sizeof(*sAlgorithmNameArg));

	spinlock_lock(&AlgorithmNameLock);
	swap(&sAlgorithmName, &sAlgorithmNameTemp);
	spinlock_unlock(&AlgorithmNameLock);
	free(sAlgorithmNameTemp);
}

void RendererCwc_ClearReadWriteCounter(Visualizer_Handle hArray) {
	ArrayProp* pArrayProp = GetHandleData(&ArrayPropPool, hArray);
	pArrayProp->ReadCount = 0;
	pArrayProp->WriteCount = 0;
}
