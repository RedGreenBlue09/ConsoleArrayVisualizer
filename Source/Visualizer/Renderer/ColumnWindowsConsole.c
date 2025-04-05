
#include "Visualizer.h"

#include <string.h>
#include <Windows.h>

#include "Utils/Atomic.h"
#include "Utils/Common.h"
#include "Utils/GuardedMalloc.h"
#include "Utils/LinkedList.h"
#include "Utils/MemoryPool.h"
#include "Utils/SharedLock.h"
#include "Utils/SpinLock.h"
#include "Utils/Time.h"

// Buffer stuff
static HANDLE ghAltBuffer = NULL;
static CONSOLE_SCREEN_BUFFER_INFOEX gBufferInfo = { 0 };
static CHAR_INFO* gaBufferCache;
static SMALL_RECT gUpdatedRect;

// Unicode support is not going to be added
// as it's too slow with current limitations.
// https://github.com/microsoft/terminal/discussions/13339
// https://github.com/microsoft/terminal/issues/10810#issuecomment-897800855
//

// Console attribute values
// cmd "color /?" explains this very well.
#define ConsoleAttribute_Background 0x0FU
#define ConsoleAttribute_Normal     0x78U
#define ConsoleAttribute_Read       0x1EU
#define ConsoleAttribute_Write      0x4BU
#define ConsoleAttribute_Pointer    0x3CU
#define ConsoleAttribute_Correct    0x2EU
#define ConsoleAttribute_Incorrect  0x4BU

// For uninitialization
static HANDLE ghOldBuffer;
static LONG_PTR gOldWindowStyle;

// Use two's complement's addition property to prevent overflow
static_assert(-123456789 == ~123456789 + 1);

// Array
typedef struct {
	visualizer_int Value;
	uint8_t aMarkerCount[Visualizer_MarkerAttribute_EnumCount];
} array_member;

typedef struct {
	atomic bool bUpdated;
	sharedlock SharedLock;
	// Worst case: 2^24 members each containing 2^8 repeated attributes
	// This will break rendering but will still work due to unsigned integer wrapping.
	intptr_t MemberCount;
	atomic visualizer_long ValueSum;
	atomic uint32_t aMarkerCount[Visualizer_MarkerAttribute_EnumCount];
} column_info_atomic;

typedef struct {
	non_atomic(bool) bUpdated;
	sharedlock SharedLock;

	intptr_t MemberCount;
	non_atomic(visualizer_long) ValueSum;
	non_atomic(uint32_t) aMarkerCount[Visualizer_MarkerAttribute_EnumCount];
} column_info_non_atomic;

typedef union {
	column_info_atomic Atomic;
	column_info_non_atomic NonAtomic;
} column_info;

static_assert(sizeof(column_info) == sizeof(column_info_non_atomic));

typedef struct {
	llist_node      Node;
	atomic bool     bRemoved;
	atomic bool     bResized;
	// atomic bool     bRangeUpdated;
	intptr_t Size;
	visualizer_int  ValueMin;
	visualizer_int  ValueMax;
	array_member*   aState;

	intptr_t        ColumnCount;
	column_info*    aColumn;

	// Statistics
	// Using global counters makes the renderer prone to desync
	// and also increases contention but that saves a lot of memory
	atomic uint64_t ReadCount;
	atomic uint64_t WriteCount;
} array_prop;

static pool gArrayPropPool;
static array_prop* gpArrayPropHead;

static thrd_t gRenderThread;
static atomic bool gbRun;

static spinlock gAlgorithmNameLock;
static char* gsAlgorithmName; // NULL terminated

static double gfDefaultDelay; // Delay for 1 element array
static atomic double gfAlgorithmSleepMultiplier;
static atomic double gfUserSleepMultiplier;

thread_pool* Visualizer_pThreadPool;

static visualizer_array_handle PoolIndexToHandle(pool_index PoolIndex) {
	return (visualizer_array_handle)(PoolIndex + 1);
}

static pool_index HandleToPoolIndex(visualizer_array_handle hHandle) {
	return (pool_index)hHandle - 1;
}

static void* GetHandleData(pool* pPool, visualizer_array_handle hHandle) {
	return Pool_IndexToAddress(pPool, HandleToPoolIndex(hHandle));
}

static bool ValidateHandle(pool* pPool, visualizer_array_handle hHandle) {
	return ((pool_index)hHandle > 0) & ((pool_index)hHandle <= pPool->nBlock);
}

// Returns iB
static inline intptr_t NearestNeighborScale(intptr_t iA, intptr_t nA, intptr_t nB) {
	// Worst case: If nB = 2^15 then nA can only be 2^48
	return ((int64_t)iA * nB + ((uintptr_t)nB / 2)) / nA;
}

static void UpdateCellCacheRow(intptr_t iRow, const char* sText, intptr_t nText) {
	if (iRow >= gBufferInfo.dwSize.Y)
		return;
	if (nText > gBufferInfo.dwSize.X)
		nText = gBufferInfo.dwSize.X;
	// Use wide char to avoid conversion
	int16_t i = 0;
	for (; i < nText; ++i)
		gaBufferCache[gBufferInfo.dwSize.X * iRow + i].Char.UnicodeChar = (wchar_t)sText[i];
	for (; i < gBufferInfo.dwSize.X; ++i)
		gaBufferCache[gBufferInfo.dwSize.X * iRow + i].Char.UnicodeChar = L' ';

	if (iRow < gUpdatedRect.Top)
		gUpdatedRect.Top = (int16_t)iRow;
	gUpdatedRect.Left = 0;
	if (iRow > gUpdatedRect.Bottom)
		gUpdatedRect.Bottom = (int16_t)iRow;
	if (nText > gUpdatedRect.Right)
		gUpdatedRect.Right = (int16_t)nText;
}

static const USHORT aWinConAttrTable[Visualizer_MarkerAttribute_EnumCount] = {
	ConsoleAttribute_Pointer,
	ConsoleAttribute_Read,
	ConsoleAttribute_Write,
	ConsoleAttribute_Correct,
	ConsoleAttribute_Incorrect
};

typedef struct {
	bool bNeedUpdate;
	uint16_t ConsoleAttr;
	int16_t Height;
} ColumnUpdateParam;

static ColumnUpdateParam GetColumnUpdateParam(array_prop* pArrayProp, intptr_t iColumn) {
	if (!atomic_load_explicit(&pArrayProp->aColumn[iColumn].Atomic.bUpdated, memory_order_relaxed))
		return (ColumnUpdateParam){ false, 0, 0 };

	// Choose the correct value & attribute

	column_info_non_atomic* pColumn = &pArrayProp->aColumn[iColumn].NonAtomic;
	visualizer_long ValueSum;
	uint32_t aMarkerCount[Visualizer_MarkerAttribute_EnumCount];

	SharedLock_LockExclusive(&pColumn->SharedLock);

	pColumn->bUpdated = false;
	ValueSum = pColumn->ValueSum;
	memcpy(aMarkerCount, (uint32_t*)&pColumn->aMarkerCount, sizeof(aMarkerCount));

	SharedLock_UnlockExclusive(&pColumn->SharedLock);

	// NOTE: Precision loss: This force it to have a precision = range
	// Fix is possible but does it worth it?
	visualizer_int Value = (visualizer_int)(ValueSum / pColumn->MemberCount);

	// Find the attribute that have the most occurrences. TODO: SIMD
	uint8_t MaxAttrCount = aMarkerCount[0];
	visualizer_marker_attribute Attr = 0;
	for (uint8_t i = 1; i < Visualizer_MarkerAttribute_EnumCount; ++i) {
		uint8_t Count = aMarkerCount[i];
		if (Count >= MaxAttrCount) {
			MaxAttrCount = Count;
			Attr = i;
		}
	}

	uint16_t ConsoleAttr;
	if (MaxAttrCount == 0)
		ConsoleAttr = ConsoleAttribute_Normal;
	else
		ConsoleAttr = aWinConAttrTable[Attr];

	// Use wrap around characteristic of unsigned integer
	visualizer_uint ValueMin = pArrayProp->ValueMin;
	visualizer_uint AbsoluteValue = (visualizer_uint)Value - ValueMin;
	visualizer_uint AbsoluteValueMax = (visualizer_uint)pArrayProp->ValueMax - ValueMin;
	if (AbsoluteValue > AbsoluteValueMax)
		AbsoluteValue = AbsoluteValueMax;

	// Scale the value to the corresponding screen height

	SHORT Height = (SHORT)((visualizer_ulong)AbsoluteValue * gBufferInfo.dwSize.Y / AbsoluteValueMax);

	return (ColumnUpdateParam){ true, ConsoleAttr, Height };
}

static void UpdateCellCacheColumn(int16_t iConsoleColumn, ColumnUpdateParam Parameter) {
	if (!Parameter.bNeedUpdate)
		return;

	gUpdatedRect.Top = 0;
	if (iConsoleColumn < gUpdatedRect.Left)
		gUpdatedRect.Left = (SHORT)iConsoleColumn;
	gUpdatedRect.Bottom = gBufferInfo.dwSize.Y - 1;
	if (iConsoleColumn > gUpdatedRect.Right)
		gUpdatedRect.Right = (SHORT)iConsoleColumn;

	intptr_t i = 0;
	for (; i < (intptr_t)(gBufferInfo.dwSize.Y - Parameter.Height); ++i)
		gaBufferCache[gBufferInfo.dwSize.X * i + iConsoleColumn].Attributes = ConsoleAttribute_Background;
	for (; i < gBufferInfo.dwSize.Y; ++i)
		gaBufferCache[gBufferInfo.dwSize.X * i + iConsoleColumn].Attributes = Parameter.ConsoleAttr;
}

static void ClearScreen() {
	LONG BufferSize = gBufferInfo.dwSize.X * gBufferInfo.dwSize.Y;
	for (intptr_t i = 0; i < BufferSize; ++i) {
		gaBufferCache[i].Char.UnicodeChar = ' ';
		gaBufferCache[i].Attributes = ConsoleAttribute_Background;
	}
	SMALL_RECT Rect = {
		0,
		0,
		gBufferInfo.dwSize.X - 1,
		gBufferInfo.dwSize.Y - 1,
	};
	WriteConsoleOutputW(
		ghAltBuffer,
		gaBufferCache,
		gBufferInfo.dwSize,
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

	uint64_t Second = clock64_resolution();
	uint64_t ThreadTimeStart = clock64();
	uint64_t UpdateInterval = Second / 60;

	uint64_t FpsUpdateInterval = Second / 2; // FPS counter interval
	uint64_t FpsUpdateCount = 0;
	uint64_t FramesRendered = 0;

	while (atomic_load_explicit(&gbRun, memory_order_relaxed)) {
		uint64_t ThreadDuration = clock64() - ThreadTimeStart;
		
		// TODO: Multi array
		if (!gpArrayPropHead) {
			continue;
		}

		array_prop* pArrayProp = gpArrayPropHead;

		// Remove
		if (atomic_load_explicit(&pArrayProp->bRemoved, memory_order_relaxed)) {
			atomic_thread_fence_light(&pArrayProp->bRemoved, memory_order_acquire);
			free(pArrayProp->aState);
			Pool_DeallocateAddress(&gArrayPropPool, pArrayProp);
			gpArrayPropHead = NULL;

			ClearScreen();
			continue;
		}

		// TODO: Resize window
		// Resize array

		intptr_t Size = pArrayProp->Size;
		if (atomic_load_explicit(&pArrayProp->bResized, memory_order_relaxed)) {
			atomic_thread_fence_light(&pArrayProp->bResized, memory_order_acquire);

			for (intptr_t iColumn = 0; iColumn < pArrayProp->ColumnCount; ++iColumn)
				pArrayProp->aColumn[iColumn].NonAtomic.MemberCount = 0;

			for (intptr_t iMember = 0; iMember < Size; ++iMember) {
				intptr_t iColumn = NearestNeighborScale(iMember, Size, pArrayProp->ColumnCount);
				++pArrayProp->aColumn[iColumn].NonAtomic.MemberCount;
			}

			intptr_t iMember = 0;
			for (intptr_t iColumn = 0; iColumn < pArrayProp->ColumnCount; ++iColumn) {

				column_info_non_atomic* pColumn = &pArrayProp->aColumn[iColumn].NonAtomic;

				SharedLock_LockExclusive(&pColumn->SharedLock);

				// Reset values
				pColumn->ValueSum = 0;
				memset(
					pColumn->aMarkerCount,
					0,
					Visualizer_MarkerAttribute_EnumCount * sizeof(*pColumn->aMarkerCount)
				);

				// Regenerate values
				intptr_t iEndMember = iMember + pColumn->MemberCount;
				for (; iMember < iEndMember; ++iMember) {
					array_member* pMember = &pArrayProp->aState[iMember];

					pColumn->ValueSum += pMember->Value;
					for (uint8_t i = 0; i < Visualizer_MarkerAttribute_EnumCount; ++i)
						pColumn->aMarkerCount[i] += pMember->aMarkerCount[i];
				}
				pColumn->bUpdated = true;

				SharedLock_UnlockExclusive(&pColumn->SharedLock);

			}

			atomic_store_fence_light(&pArrayProp->bResized, false);

		}

		// Update cell cache columns

		gUpdatedRect = (SMALL_RECT){
			gBufferInfo.dwSize.X - 1,
			gBufferInfo.dwSize.Y - 1,
			0,
			0
		};

		{
			ColumnUpdateParam UpdateParam = { false, 0, 0 };
			intptr_t iColumnOld = -1;
			for (int16_t i = 0; i < gBufferInfo.dwSize.X; ++i) {
				intptr_t iColumn = NearestNeighborScale(i, gBufferInfo.dwSize.X, pArrayProp->ColumnCount);
				if (iColumn > iColumnOld) {
					UpdateParam = GetColumnUpdateParam(pArrayProp, iColumn);
					iColumnOld = iColumn;
				}
				UpdateCellCacheColumn(i, UpdateParam);
			}
		}

		// Update cell text rows

		// Algorithm name
		SpinLock_Lock(&gAlgorithmNameLock);

		char* sAlgorithmNameTemp = gsAlgorithmName;
		if (sAlgorithmNameTemp == NULL)
			sAlgorithmNameTemp = "";
		UpdateCellCacheRow(0, sAlgorithmNameTemp, strlen(sAlgorithmNameTemp));

		SpinLock_Unlock(&gAlgorithmNameLock);

		intptr_t Length;
		intptr_t NumberLength;

		// FPS
		uint64_t NewFpsUpdateCount = ThreadDuration / FpsUpdateInterval;
		if (NewFpsUpdateCount > FpsUpdateCount) {

			char aFpsString[48] = "FPS: ";
			Length = static_strlen("FPS: ");
			NumberLength = Uint64ToString(
				FramesRendered * Second / ((NewFpsUpdateCount - FpsUpdateCount) * FpsUpdateInterval),
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
			Pool_AddressToIndex(&gArrayPropPool, pArrayProp),
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
		NumberLength = Uint64ToString(
			atomic_load_explicit(&pArrayProp->ReadCount, memory_order_relaxed),
			aReadCountString + Length
		);
		Length += NumberLength;
		UpdateCellCacheRow(6, aReadCountString, Length);

		// Write
		char aWriteCountString[48] = "Writes: ";
		Length = static_strlen("Writes: ");
		NumberLength = Uint64ToString(
			atomic_load_explicit(&pArrayProp->WriteCount, memory_order_relaxed),
			aWriteCountString + Length
		);
		Length += NumberLength;
		UpdateCellCacheRow(7, aWriteCountString, Length);

		WriteConsoleOutputW(
			ghAltBuffer,
			gaBufferCache,
			gBufferInfo.dwSize,
			(COORD){ gUpdatedRect.Left , gUpdatedRect.Top },
			&gUpdatedRect
		);
		++FramesRendered;

		ThreadDuration = clock64() - ThreadTimeStart;
		sleep64(UpdateInterval - (ThreadDuration % UpdateInterval));
	}

	return 0;

}

void Visualizer_Initialize(size_t ExtraThreadCount) {

	Pool_Initialize(&gArrayPropPool, 16, sizeof(array_prop));
	gsAlgorithmName = NULL;
	atomic_store_explicit(&gfAlgorithmSleepMultiplier, 1.0, memory_order_relaxed);
	atomic_store_explicit(&gfUserSleepMultiplier, 1.0, memory_order_relaxed);

	// New window style

	HWND hWindow = GetConsoleWindow();
	gOldWindowStyle = GetWindowLongPtrW(hWindow, GWL_STYLE);
	SetWindowLongPtrW(
		hWindow,
		GWL_STYLE,
		gOldWindowStyle & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX
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

	ghAltBuffer = CreateConsoleScreenBuffer(
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		CONSOLE_TEXTMODE_BUFFER,
		NULL
	);
	ghOldBuffer = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleActiveScreenBuffer(ghAltBuffer);

	// Set cursor to top left

	gBufferInfo.cbSize = sizeof(gBufferInfo);
	GetConsoleScreenBufferInfoEx(ghAltBuffer, &gBufferInfo);
	gBufferInfo.dwCursorPosition = (COORD){ 0, 0 };
	gBufferInfo.wAttributes = ConsoleAttribute_Background;
	// WORKAROUND: Window size is 1 row smaller than expected
	gBufferInfo.srWindow.Bottom = gBufferInfo.dwSize.Y;
	SetConsoleScreenBufferInfoEx(ghAltBuffer, &gBufferInfo);

	// Initialize buffer cache

	LONG BufferSize = gBufferInfo.dwSize.X * gBufferInfo.dwSize.Y;
	gaBufferCache = malloc_guarded(BufferSize * sizeof(*gaBufferCache));
	ClearScreen();

	// Render thread

	atomic_store_explicit(&gbRun, true, memory_order_relaxed);
	thrd_create(&gRenderThread, RenderThreadMain, NULL);

	// Other stuff

	gfDefaultDelay = (double)(clock64_resolution() * 10); // 10s
	Visualizer_pThreadPool = ThreadPool_Create(ExtraThreadCount);
}

void Visualizer_Uninitialize() {

	ThreadPool_Destroy(Visualizer_pThreadPool);

	// Stop render thread

	atomic_store_explicit(&gbRun, false, memory_order_relaxed);
	int ThreadReturn;
	thrd_join(gRenderThread, &ThreadReturn);

	// Free alternate buffer

	SetConsoleActiveScreenBuffer(ghOldBuffer);
	CloseHandle(ghAltBuffer);

	// Restore window mode

	HWND hWindow = GetConsoleWindow();
	SetWindowLongPtrW(hWindow, GWL_STYLE, gOldWindowStyle);
	SetWindowPos(hWindow, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE);

	free(gaBufferCache);

	// Free arrays & markers

	Pool_Destroy(&gArrayPropPool);

}

visualizer_array_handle Visualizer_AddArray(
	intptr_t Size,
	visualizer_int* aArrayState,
	visualizer_int ValueMin,
	visualizer_int ValueMax
) {
	pool_index Index = Pool_Allocate(&gArrayPropPool);
	if (Index == POOL_INVALID_INDEX) return NULL;
	array_prop* pArrayProp = Pool_IndexToAddress(&gArrayPropPool, Index);

	if (Size < 1) return NULL; // TODO: Allow this

	pArrayProp->Size = Size;
	pArrayProp->aState = malloc_guarded(Size * sizeof(*pArrayProp->aState));
	pArrayProp->ColumnCount = min2(Size, gBufferInfo.dwSize.X);
	pArrayProp->aColumn = calloc_guarded(pArrayProp->ColumnCount, sizeof(*pArrayProp->aColumn));
	if (aArrayState)
		for (intptr_t i = 0; i < Size; ++i)
			pArrayProp->aState[i] = (array_member){ aArrayState[i] };
	else
		for (intptr_t i = 0; i < Size; ++i)
			pArrayProp->aState[i] = (array_member){ 0 };

	if (ValueMax <= ValueMin)
		ValueMax = ValueMin + 1;
	pArrayProp->ValueMin = ValueMin;
	pArrayProp->ValueMax = ValueMax;
	atomic_store_fence_light(&pArrayProp->bRemoved, false);
	atomic_store_fence_light(&pArrayProp->bResized, true);

	atomic_store_explicit(&pArrayProp->ReadCount, 0, memory_order_relaxed);
	atomic_store_explicit(&pArrayProp->WriteCount, 0, memory_order_relaxed);

	if (!gpArrayPropHead) // TODO: Multi array
		gpArrayPropHead = pArrayProp;
	return PoolIndexToHandle(Index);
}

void Visualizer_RemoveArray(visualizer_array_handle hArray) {
	assert(ValidateHandle(&gArrayPropPool, hArray));
	pool_index Index = HandleToPoolIndex(hArray);
	array_prop* pArrayProp = Pool_IndexToAddress(&gArrayPropPool, Index);

	atomic_store_fence_light(&pArrayProp->bRemoved, true);
}

// UNSUPPORTED: Multiple threads on the same member
typedef uint8_t member_update_type;
#define MemberUpdateType_No              0 // Not used
#define MemberUpdateType_Attribute      (1 << 0)
#define MemberUpdateType_Value          (1 << 1)

static inline void UpdateMember(
	array_prop* pArrayProp,
	intptr_t iPosition,
	member_update_type UpdateType,
	bool bAddAttribute,
	visualizer_marker_attribute Attribute,
	visualizer_int Value
) {
	array_member* pArrayMember = &pArrayProp->aState[iPosition];

	// Most branches are known at compile time and can be optimized

	intptr_t iColumn = NearestNeighborScale(iPosition, pArrayProp->Size, pArrayProp->ColumnCount);
	column_info_atomic* pColumn = &pArrayProp->aColumn[iColumn].Atomic;

	// This lock is put up here for the render thread to handle window resize
	SharedLock_LockShared(&pColumn->SharedLock);

	if (UpdateType & MemberUpdateType_Attribute) {
		if (bAddAttribute)
			++pArrayMember->aMarkerCount[Attribute];
		else
			--pArrayMember->aMarkerCount[Attribute];
	}

	visualizer_int OldValue = Value;
	if (UpdateType & MemberUpdateType_Value)
		swap(&pArrayMember->Value, &OldValue);

	if (UpdateType & MemberUpdateType_Value)
		atomic_fetch_add_explicit(&pColumn->ValueSum, Value - OldValue, memory_order_relaxed);
	if (UpdateType & MemberUpdateType_Attribute) {
		if (bAddAttribute)
			atomic_fetch_add_explicit(&pColumn->aMarkerCount[Attribute], 1, memory_order_relaxed);
		else
			atomic_fetch_sub_explicit(&pColumn->aMarkerCount[Attribute], 1, memory_order_relaxed);
	}

	SharedLock_UnlockShared(&pColumn->SharedLock);
	atomic_store_explicit(&pColumn->bUpdated, true, memory_order_relaxed);
};

void Visualizer_UpdateArrayState(visualizer_array_handle hArray, visualizer_int* aState) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);
	for (intptr_t i = 0; i < pArrayProp->Size; ++i)
		UpdateMember(pArrayProp, i, MemberUpdateType_Value, false, 0, aState[i]);
}

// TODO: Implement array resize

/*
void Visualizer_UpdateArray(
	visualizer_array_handle hArray,
	intptr_t NewSize,
	visualizer_int ValueMin,
	visualizer_int ValueMax
) {

	assert(ValidateHandle(&gArrayPropPool, hArray));
	pool_index Index = HandleToPoolIndex(hArray);
	array_prop* pArrayProp = Pool_IndexToAddress(&gArrayPropPool, Index);

	// Clear screen

	uint32_t Written;
	FillConsoleOutputAttribute(
		ghAltBuffer,
		ConsoleAttribute_Background,
		gBufferInfo.dwSize.X * gBufferInfo.dwSize.Y,
		(COORD){ 0, 0 },
		&Written
	);

	pArrayProp->ValueMin = ValueMin;
	pArrayProp->ValueMax = ValueMax;

	// Handle array resize

	if ((NewSize > 0) && (NewSize != pArrayProp->Size)) {

		// Realloc arrays

		visualizer_int* aResizedArrayState = realloc_guarded(
			pArrayProp->aState,
			NewSize * sizeof(visualizer_int)
		);
		visualizer_marker_attribute* aResizedAttribute = realloc_guarded(
			pArrayProp->aAttribute,
			NewSize * sizeof(visualizer_marker_attribute)
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
		Visualizer_UpdateItem(&UpdateRequest);
	}

	return;

}
*/

// Marker helpers

static void AddMarker(
	array_prop* pArrayProp,
	intptr_t iPosition,
	visualizer_marker_attribute Attribute
) {
	UpdateMember(pArrayProp, iPosition, MemberUpdateType_Attribute, true, Attribute, 0);
}

static void AddMarkerWithValue(
	array_prop* pArrayProp,
	intptr_t iPosition,
	visualizer_marker_attribute Attribute,
	visualizer_int Value
) {
	UpdateMember(
		pArrayProp,
		iPosition,
		MemberUpdateType_Attribute | MemberUpdateType_Value,
		true,
		Attribute,
		Value
	);
}

static void RemoveMarkerHelper(array_prop* pArrayProp, intptr_t iPosition, visualizer_marker_attribute Attribute) {
	UpdateMember(pArrayProp, iPosition, MemberUpdateType_Attribute, false, Attribute, 0);
}

// It does not update the iPosition member
static void MoveMarkerHelper(
	array_prop* pArrayProp,
	intptr_t iPosition,
	intptr_t iNewPosition,
	visualizer_marker_attribute Attribute
) {
	UpdateMember(pArrayProp, iPosition, MemberUpdateType_Attribute, false, Attribute, 0);
	UpdateMember(pArrayProp, iNewPosition, MemberUpdateType_Attribute, true, Attribute, 0);
}

// Delays

void Visualizer_SetAlgorithmSleepMultiplier(double fAlgorithmSleepMultiplier){
	atomic_store_explicit(&gfAlgorithmSleepMultiplier, fAlgorithmSleepMultiplier, memory_order_relaxed);
}

void Visualizer_SetUserSleepMultiplier(double fUserSleepMultiplier) {
	atomic_store_explicit(&gfUserSleepMultiplier, fUserSleepMultiplier, memory_order_relaxed);
}

void Visualizer_Sleep(double fSleepMultiplier) {
#ifdef VISUALIZER_DISABLE_SLEEP
#else
	sleep64(
		(uint64_t)(
			gfDefaultDelay *
			atomic_load_explicit(&gfAlgorithmSleepMultiplier, memory_order_relaxed) *
			atomic_load_explicit(&gfUserSleepMultiplier, memory_order_relaxed) *
			fSleepMultiplier
		)
	);
#endif
}

// Read & Write

void Visualizer_UpdateRead(visualizer_array_handle hArray, intptr_t iPosition, double fSleepMultiplier) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);

	atomic_fetch_add_explicit(&pArrayProp->ReadCount, 1, memory_order_relaxed);
	AddMarker(pArrayProp, iPosition, Visualizer_MarkerAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	RemoveMarkerHelper(pArrayProp, iPosition, Visualizer_MarkerAttribute_Read);
}

void Visualizer_UpdateRead2(visualizer_array_handle hArray, intptr_t iPositionA, intptr_t iPositionB, double fSleepMultiplier) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);

	atomic_fetch_add_explicit(&pArrayProp->ReadCount, 2, memory_order_relaxed);
	AddMarker(pArrayProp, iPositionA, Visualizer_MarkerAttribute_Read);
	AddMarker(pArrayProp, iPositionB, Visualizer_MarkerAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	RemoveMarkerHelper(pArrayProp, iPositionA, Visualizer_MarkerAttribute_Read);
	RemoveMarkerHelper(pArrayProp, iPositionB, Visualizer_MarkerAttribute_Read);
}

void Visualizer_UpdateReadMulti(
	visualizer_array_handle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	double fSleepMultiplier
) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);

	assert(Length <= 16);

	atomic_fetch_add_explicit(&pArrayProp->ReadCount, Length, memory_order_relaxed);
	for (intptr_t i = 0; i < Length; ++i)
		AddMarker(pArrayProp, iStartPosition + i, Visualizer_MarkerAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	for (intptr_t i = 0; i < Length; ++i)
		RemoveMarkerHelper(pArrayProp, iStartPosition + i, Visualizer_MarkerAttribute_Read);
}

void Visualizer_UpdateWrite(visualizer_array_handle hArray, intptr_t iPosition, visualizer_int NewValue, double fSleepMultiplier) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);

	atomic_fetch_add_explicit(&pArrayProp->WriteCount, 1, memory_order_relaxed);
	AddMarkerWithValue(pArrayProp, iPosition, Visualizer_MarkerAttribute_Write, NewValue);
	Visualizer_Sleep(fSleepMultiplier);
	RemoveMarkerHelper(pArrayProp, iPosition, Visualizer_MarkerAttribute_Write);
}

void Visualizer_UpdateReadWrite(
	visualizer_array_handle hArray,
	intptr_t iPositionA,
	intptr_t iPositionB,
	double fSleepMultiplier
) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);

	atomic_fetch_add_explicit(&pArrayProp->ReadCount, 1, memory_order_relaxed);
	atomic_fetch_add_explicit(&pArrayProp->WriteCount, 1, memory_order_relaxed);
	visualizer_int NewValueA = pArrayProp->aState[iPositionB].Value;
	AddMarkerWithValue(pArrayProp, iPositionA, Visualizer_MarkerAttribute_Write, NewValueA);
	AddMarker(pArrayProp, iPositionB, Visualizer_MarkerAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	RemoveMarkerHelper(pArrayProp, iPositionA, Visualizer_MarkerAttribute_Write);
	RemoveMarkerHelper(pArrayProp, iPositionB, Visualizer_MarkerAttribute_Read);
}

void Visualizer_UpdateSwap(
	visualizer_array_handle hArray,
	intptr_t iPositionA,
	intptr_t iPositionB,
	double fSleepMultiplier
) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);

	atomic_fetch_add_explicit(&pArrayProp->ReadCount, 2, memory_order_relaxed);
	atomic_fetch_add_explicit(&pArrayProp->WriteCount, 2, memory_order_relaxed);
	visualizer_int NewValueA = pArrayProp->aState[iPositionB].Value;
	visualizer_int NewValueB = pArrayProp->aState[iPositionA].Value;
	AddMarkerWithValue(pArrayProp, iPositionA, Visualizer_MarkerAttribute_Write, NewValueA);
	AddMarkerWithValue(pArrayProp, iPositionB, Visualizer_MarkerAttribute_Write, NewValueB);
	Visualizer_Sleep(fSleepMultiplier);
	RemoveMarkerHelper(pArrayProp, iPositionA, Visualizer_MarkerAttribute_Write);
	RemoveMarkerHelper(pArrayProp, iPositionB, Visualizer_MarkerAttribute_Write);
}

void Visualizer_UpdateWriteMulti(
	visualizer_array_handle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	visualizer_int* aNewValue,
	double fSleepMultiplier
) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);

	assert(Length <= 16);

	atomic_fetch_add_explicit(&pArrayProp->WriteCount, Length, memory_order_relaxed);
	for (intptr_t i = 0; i < Length; ++i)
		AddMarkerWithValue(pArrayProp, iStartPosition + i, Visualizer_MarkerAttribute_Write, aNewValue[i]);
	Visualizer_Sleep(fSleepMultiplier);
	for (intptr_t i = 0; i < Length; ++i)
		RemoveMarkerHelper(pArrayProp, iStartPosition + i, Visualizer_MarkerAttribute_Write);
}

// Marker

visualizer_marker Visualizer_CreateMarker(
	visualizer_array_handle hArray,
	intptr_t iPosition,
	visualizer_marker_attribute Attribute
) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);
	AddMarker(pArrayProp, iPosition, Attribute);
	return (visualizer_marker){ hArray, iPosition, Attribute };
}

void Visualizer_RemoveMarker(visualizer_marker Marker) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, Marker.hArray);
	RemoveMarkerHelper(pArrayProp, Marker.iPosition, Marker.Attribute);
}

void Visualizer_MoveMarker(visualizer_marker* pMarker, intptr_t iNewPosition) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, pMarker->hArray);
	MoveMarkerHelper(pArrayProp, pMarker->iPosition, iNewPosition, pMarker->Attribute);
	pMarker->iPosition = iNewPosition;
}

void Visualizer_SetAlgorithmName(char* sAlgorithmName) {
	intptr_t Size = strlen(sAlgorithmName) + 1;
	char* sAlgorithmNameTemp = malloc_guarded(Size * sizeof(*sAlgorithmName));
	memcpy(sAlgorithmNameTemp, sAlgorithmName, Size * sizeof(*sAlgorithmName));

	SpinLock_Lock(&gAlgorithmNameLock);
	swap(&gsAlgorithmName, &sAlgorithmNameTemp);
	SpinLock_Unlock(&gAlgorithmNameLock);
	free(sAlgorithmNameTemp);
}

void Visualizer_ClearReadWriteCounter(visualizer_array_handle hArray) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);
	atomic_store_explicit(&pArrayProp->ReadCount, 0, memory_order_relaxed);
	atomic_store_explicit(&pArrayProp->WriteCount, 0, memory_order_relaxed);
}

// FIXME: Proper impl
void Visualizer_UpdateCorrectness(visualizer_array_handle hArray, intptr_t iPosition, bool bCorrect) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);
	visualizer_marker_attribute Attribute =
		bCorrect ?
		Visualizer_MarkerAttribute_Correct : 
		Visualizer_MarkerAttribute_Incorrect;
	UpdateMember(pArrayProp, iPosition, MemberUpdateType_Attribute, true, Attribute, 0);
}

void Visualizer_ClearCorrectness(visualizer_array_handle hArray, intptr_t iPosition, bool bCorrect) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);
	visualizer_marker_attribute Attribute =
		bCorrect ?
		Visualizer_MarkerAttribute_Correct :
		Visualizer_MarkerAttribute_Incorrect;
	UpdateMember(pArrayProp, iPosition, MemberUpdateType_Attribute, false, Attribute, 0);
}
