
#include "Visualizer.h"

#include <string.h>
#include <Windows.h>

#include "Utils/Atomic.h"
#include "Utils/Common.h"
#include "Utils/GuardedMalloc.h"
#include "Utils/IntMath.h"
#include "Utils/LinkedList.h"
#include "Utils/MemoryPool.h"
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
	spinlock Lock;
	// Worst case: 2^24 members each containing 2^8 repeated attributes
	// This will break rendering but will still work due to unsigned integer wrapping.
	usize MemberCount; // Doesn't have to be duplicated per-thread but it's simpler to keep it this way.
	visualizer_long ValueSum;
	uint32_t aMarkerCount[Visualizer_MarkerAttribute_EnumCount];
} column_info;

typedef struct {
	alignas(sizeof(void*) * 16) column_info* aColumn;
	atomic uint64_t ReadCount;
	atomic uint64_t WriteCount;
} array_tls;

typedef struct {
	llist_node      Node;
	atomic bool     bRemoved;
	// atomic bool     bRangeUpdated;
	usize           Size;
	visualizer_int  ValueMin;
	visualizer_int  ValueMax;
	array_member*   aState;

	usize           ColumnCount;
	array_tls*      aTls;
} array_prop;

static pool gArrayPropPool;
static array_prop* gpArrayPropHead;

static thrd_t gRenderThread;
static atomic bool gbRun;

static spinlock gAlgorithmNameLock;
static char* gsAlgorithmName; // NULL terminated

static floatptr_t gfDefaultDelay; // Delay for 1 element array
static floatptr_t gfAlgorithmSleepMultiplier;
static atomic floatptr_t gfUserSleepMultiplier;

static atomic uint64_t gTimerStartTime;
static atomic uint64_t gTimerStopTime;

thread_pool* Visualizer_pThreadPool;

static visualizer_array PoolIndexToHandle(pool_index PoolIndex) {
	return (visualizer_array)(PoolIndex + 1);
}

static pool_index HandleToPoolIndex(visualizer_array hHandle) {
	return (pool_index)hHandle - 1;
}

static void* GetHandleData(pool* pPool, visualizer_array hHandle) {
	return Pool_IndexToAddress(pPool, HandleToPoolIndex(hHandle));
}

static bool ValidateHandle(pool* pPool, visualizer_array hHandle) {
	return ((pool_index)hHandle > 0) & ((pool_index)hHandle <= pPool->nBlock);
}

// Returns iB
static inline usize NearestNeighborScale(usize iA, usize nA, usize nB) {
	// Worst case: If nB = 2^15 then nA can only be 2^48
	uint64_t Rem;
	return (usize)div_u64(((uint64_t)iA * nB + nB / 2), nA, &Rem);
}

static void UpdateCellCacheRow(int16_t iRow, const char* sText, usize nText) {
	if (iRow >= gBufferInfo.dwSize.Y)
		return;
	if (nText > gBufferInfo.dwSize.X)
		nText = gBufferInfo.dwSize.X;
	// Use wide char to avoid conversion
	uint16_t i = 0;
	for (; i < nText; ++i)
		gaBufferCache[(uint32_t)gBufferInfo.dwSize.X * iRow + i].Char.UnicodeChar = (wchar_t)sText[i];
	for (; i < gBufferInfo.dwSize.X; ++i) // TODO: Remember old length
		gaBufferCache[(uint32_t)gBufferInfo.dwSize.X * iRow + i].Char.UnicodeChar = L' ';

	if (iRow < gUpdatedRect.Top)
		gUpdatedRect.Top = iRow;
	gUpdatedRect.Left = 0;
	if (iRow > gUpdatedRect.Bottom)
		gUpdatedRect.Bottom = iRow;
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

static ColumnUpdateParam GetColumnUpdateParam(array_prop* pArrayProp, usize iColumn) {
	usize ThreadCount = Visualizer_pThreadPool->ThreadCount;
	for (usize iThread = 0; iThread < ThreadCount; ++iThread)
		if (atomic_load_explicit(&pArrayProp->aTls[iThread].aColumn[iColumn].bUpdated, memory_order_relaxed))
			goto HasUpdate;
	return (ColumnUpdateParam){ false, 0, 0 };

	HasUpdate:
	// Choose the correct value & attribute

	visualizer_long ValueSum = 0;
	uint32_t aMarkerCount[Visualizer_MarkerAttribute_EnumCount] = { 0 };

	for (usize iThread = 0; iThread < ThreadCount; ++iThread) {
		column_info* pColumn = &pArrayProp->aTls[iThread].aColumn[iColumn];

		SpinLock_Lock(&pColumn->Lock);

		atomic_store_explicit(&pColumn->bUpdated, false, memory_order_relaxed);
		ValueSum += pColumn->ValueSum;
		for (usize iMarker = 0; iMarker < Visualizer_MarkerAttribute_EnumCount; ++iMarker)
			aMarkerCount[iMarker] += pColumn->aMarkerCount[iMarker]; // Possible with SIMD

		SpinLock_Unlock(&pColumn->Lock);
	}

	// NOTE: Precision loss: This force it to have a precision = range
	// Fix is possible but does it worth it?
	uint64_t Rem;
	visualizer_int Value = (visualizer_int)div_u64(ValueSum, pArrayProp->aTls->aColumn[iColumn].MemberCount, &Rem);

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

	SHORT Height = (SHORT)div_u64(AbsoluteValue * (uint64_t)gBufferInfo.dwSize.Y, AbsoluteValueMax, &Rem);

	return (ColumnUpdateParam){ true, ConsoleAttr, Height };
}

static void UpdateCellCacheColumn(int16_t iConsoleColumn, ColumnUpdateParam Parameter) {
	if (!Parameter.bNeedUpdate)
		return;

	gUpdatedRect.Top = 0;
	if (iConsoleColumn < gUpdatedRect.Left)
		gUpdatedRect.Left = iConsoleColumn;
	gUpdatedRect.Bottom = gBufferInfo.dwSize.Y - 1;
	if (iConsoleColumn > gUpdatedRect.Right)
		gUpdatedRect.Right = iConsoleColumn;

	int16_t i = 0;
	for (; i < gBufferInfo.dwSize.Y - Parameter.Height; ++i)
		gaBufferCache[(int32_t)gBufferInfo.dwSize.X * i + iConsoleColumn].Attributes = ConsoleAttribute_Background;
	for (; i < gBufferInfo.dwSize.Y; ++i)
		gaBufferCache[(int32_t)gBufferInfo.dwSize.X * i + iConsoleColumn].Attributes = Parameter.ConsoleAttr;
}

static void ClearScreen() {
	int32_t BufferSize = (int32_t)gBufferInfo.dwSize.X * gBufferInfo.dwSize.Y;
	for (int32_t i = 0; i < BufferSize; ++i) {
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

static_assert(sizeof(usize) <= 8);

// Max length of sString is 20
// strlen("18446744073709551615") == 20
static usize Uint64ToString(uint64_t X, char* sString) {
	if (X == 0) {
		sString[0] = '0';
		return 1;
	}

	usize i = 0;
	for (; X > 0; X /= 10)
		sString[i++] = '0' + (X % 10);
	usize Length = i;

	usize ii = 0;
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
	uint64_t Millisecond = Second / 1000;
	uint64_t ThreadTimeStart = clock64();
	uint64_t UpdateInterval = Second / 60; // TODO: Query from system

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
			free(pArrayProp->aTls);
			Pool_DeallocateAddress(&gArrayPropPool, pArrayProp);
			gpArrayPropHead = NULL;

			ClearScreen();
			continue;
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
			isize iColumnOld = -1;
			for (int16_t i = 0; i < gBufferInfo.dwSize.X; ++i) {
				isize iColumn = (isize)NearestNeighborScale(i, gBufferInfo.dwSize.X, pArrayProp->ColumnCount);
				if (iColumn > iColumnOld) {
					UpdateParam = GetColumnUpdateParam(pArrayProp, (usize)iColumn);
					iColumnOld = iColumn;
				}
				UpdateCellCacheColumn(i, UpdateParam);
			}
		}

		// Update cell text rows
		int16_t Row = 0;

		// Algorithm name
		{
			SpinLock_Lock(&gAlgorithmNameLock);

			char* sAlgorithmNameTemp = gsAlgorithmName;
			UpdateCellCacheRow(Row, sAlgorithmNameTemp, strlen(sAlgorithmNameTemp));

			SpinLock_Unlock(&gAlgorithmNameLock);
		}
		++Row;

		usize Length;
		uint64_t Rem;

		// FPS
		uint64_t NewFpsUpdateCount = div_u64(ThreadDuration, FpsUpdateInterval, &Rem);
		if (NewFpsUpdateCount > FpsUpdateCount) {
			char aFpsString[48] = "FPS: ";
			Length = static_strlen("FPS: ");
			Length += Uint64ToString(
				FramesRendered * div_u64(Second, (NewFpsUpdateCount - FpsUpdateCount) * FpsUpdateInterval, &Rem),
				aFpsString + Length
			);
			UpdateCellCacheRow(Row, aFpsString, Length);

			FpsUpdateCount = NewFpsUpdateCount;
			FramesRendered = 0;
		}
		++Row;

		// Visual time
		{
			uint64_t CachedTimerStartTime = atomic_load_explicit(&gTimerStartTime, memory_order_acquire);
			uint64_t CachedTimerStopTime = atomic_load_explicit(&gTimerStopTime, memory_order_relaxed);
			if (CachedTimerStopTime == UINT64_MAX)
				CachedTimerStopTime = clock64();
			uint64_t VisualTime = div_u64(CachedTimerStopTime - CachedTimerStartTime, Millisecond, &Rem);

			char aVisualTimeString[48] = "Visual time: ";
			Length = static_strlen("Visual time: ");
			Length += Uint64ToString(VisualTime / 1000, aVisualTimeString + Length);

			aVisualTimeString[Length++] = '.';
			aVisualTimeString[Length++] = '0' + (char)(VisualTime / 100 % 10);
			aVisualTimeString[Length++] = '0' + (char)(VisualTime / 10 % 10);
			aVisualTimeString[Length++] = '0' + (char)(VisualTime % 10);
			aVisualTimeString[Length++] = ' ';
			aVisualTimeString[Length++] = 's';

			UpdateCellCacheRow(Row, aVisualTimeString, Length);
		}
		++Row;

		// Empty
		++Row;

		// Array
		{
			char aArrayIndexString[48] = "Array #";
			Length = static_strlen("Array #");
			Length += Uint64ToString(
				Pool_AddressToIndex(&gArrayPropPool, pArrayProp),
				aArrayIndexString + Length
			);
			UpdateCellCacheRow(Row, aArrayIndexString, Length);
		}
		++Row;

		// Size
		{
			char aSizeString[48] = "Size: ";
			Length = static_strlen("Size: ");
			Length += Uint64ToString(pArrayProp->Size, aSizeString + Length);
			UpdateCellCacheRow(Row, aSizeString, Length);
		}
		++Row;

		// Reads & Writes
		uint64_t ReadCount = 0;
		uint64_t WriteCount = 0;
		for (uint8_t i = 0; i < Visualizer_pThreadPool->ThreadCount; ++i) {
			ReadCount += atomic_load_explicit(&pArrayProp->aTls[i].ReadCount, memory_order_relaxed);
			WriteCount += atomic_load_explicit(&pArrayProp->aTls[i].WriteCount, memory_order_relaxed);
		}

		// Reads
		{

			char aReadCountString[48] = "Reads: ";
			Length = static_strlen("Reads: ");
			Length += Uint64ToString(ReadCount, aReadCountString + Length);
			UpdateCellCacheRow(Row, aReadCountString, Length);
		}
		++Row;

		// Writes
		{
			char aWriteCountString[48] = "Writes: ";
			Length = static_strlen("Writes: ");
			Length += Uint64ToString(WriteCount, aWriteCountString + Length);
			UpdateCellCacheRow(Row, aWriteCountString, Length);
		}
		++Row;

		WriteConsoleOutputW(
			ghAltBuffer,
			gaBufferCache,
			gBufferInfo.dwSize,
			(COORD){ gUpdatedRect.Left , gUpdatedRect.Top },
			&gUpdatedRect
		);
		++FramesRendered;

		ThreadDuration = clock64() - ThreadTimeStart;
		div_u64(ThreadDuration, UpdateInterval, &Rem);
		sleep64(UpdateInterval - Rem);
	}

	return 0;

}

void Visualizer_Initialize(usize ExtraThreadCount) {

	Pool_Initialize(&gArrayPropPool, 16, sizeof(array_prop));
	gsAlgorithmName = malloc_guarded(sizeof(*gsAlgorithmName));
	*gsAlgorithmName = '\0';
	gfDefaultDelay = (floatptr_t)(clock64_resolution() * 10); // 10s
	gfAlgorithmSleepMultiplier = 1.0;
	atomic_init(&gfUserSleepMultiplier, 1.0);
	atomic_init(&gTimerStopTime, 0);
	atomic_init(&gTimerStartTime, 0);

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

	int32_t BufferSize = (int32_t)gBufferInfo.dwSize.X * gBufferInfo.dwSize.Y;
	gaBufferCache = malloc_guarded(BufferSize * sizeof(*gaBufferCache));
	ClearScreen();

	// Thread pool

	Visualizer_pThreadPool = ThreadPool_Create((uint8_t)(ExtraThreadCount + 1));

	// Render thread

	atomic_init(&gbRun, true);
	thrd_create(&gRenderThread, RenderThreadMain, NULL);
}

void Visualizer_Uninitialize() {

	// Stop render thread

	atomic_store_explicit(&gbRun, false, memory_order_relaxed);
	int ThreadReturn;
	thrd_join(gRenderThread, &ThreadReturn);

	// Cleanup thread pool

	ThreadPool_Destroy(Visualizer_pThreadPool);

	// Free alternate buffer

	SetConsoleActiveScreenBuffer(ghOldBuffer);
	CloseHandle(ghAltBuffer);

	// Restore window mode

	HWND hWindow = GetConsoleWindow();
	SetWindowLongPtrW(hWindow, GWL_STYLE, gOldWindowStyle);
	SetWindowPos(hWindow, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE);

	free(gaBufferCache);

	// Free other stuff

	Pool_Destroy(&gArrayPropPool);

}

visualizer_array Visualizer_AddArray(
	usize Size,
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

	if (aArrayState)
		for (usize i = 0; i < Size; ++i)
			pArrayProp->aState[i] = (array_member){ aArrayState[i] };
	else
		memset(pArrayProp->aState, 0, Size * sizeof(*pArrayProp->aState));

	pArrayProp->ColumnCount = min2(Size, gBufferInfo.dwSize.X);

	usize ThreadCount = Visualizer_pThreadPool->ThreadCount;
	usize ColumnArraySize = pArrayProp->ColumnCount * sizeof(*pArrayProp->aTls->aColumn);
	// Group allocations
	uint8_t* aBuffer = calloc_guarded((sizeof(*pArrayProp->aTls) + ColumnArraySize) * ThreadCount, 1);

	pArrayProp->aTls = (void*)aBuffer; // Only need to free this
	aBuffer += sizeof(*pArrayProp->aTls) * ThreadCount;

	for (usize iThread = 0; iThread < ThreadCount; ++iThread) { // Window resize? Nah, too expensive
		column_info* aColumn = (void*)aBuffer;
		aBuffer += ColumnArraySize;
		pArrayProp->aTls[iThread].aColumn = aColumn;
		for (usize iMember = 0; iMember < Size; ++iMember) {
			usize iColumn = NearestNeighborScale(iMember, Size, pArrayProp->ColumnCount);
			++aColumn[iColumn].MemberCount;
		}
	}

	if (ValueMax <= ValueMin)
		ValueMax = ValueMin + 1;
	pArrayProp->ValueMin = ValueMin;
	pArrayProp->ValueMax = ValueMax;
	atomic_init(&pArrayProp->bRemoved, false);

	for (uint8_t i = 0; i < Visualizer_pThreadPool->ThreadCount; ++i) {
		atomic_init(&pArrayProp->aTls[i].ReadCount, 0);
		atomic_init(&pArrayProp->aTls[i].WriteCount, 0);
	}

	if (!gpArrayPropHead) // TODO: Multi array
		gpArrayPropHead = pArrayProp; // FIXME: Atomic
	return PoolIndexToHandle(Index);
}

void Visualizer_RemoveArray(visualizer_array hArray) {
	assert(ValidateHandle(&gArrayPropPool, hArray));
	pool_index Index = HandleToPoolIndex(hArray);
	array_prop* pArrayProp = Pool_IndexToAddress(&gArrayPropPool, Index);

	atomic_store_fence_light(&pArrayProp->bRemoved, true);
}

/*

void Visualizer_ResizeArray(
	visualizer_array hArray,
	usize NewSize
) {
	assert(ValidateHandle(&gArrayPropPool, hArray));
	pool_index Index = HandleToPoolIndex(hArray);
	array_prop* pArrayProp = Pool_IndexToAddress(&gArrayPropPool, Index);

	if ((NewSize <= 0) || (NewSize == pArrayProp->Size))
		return;

	// Realloc arrays
	array_member* aNewArrayState = malloc_guarded(NewSize * sizeof(*aNewArrayState));
	memcpy(aNewArrayState, pArrayProp->aState, NewSize * sizeof(*aNewArrayState));

	// Initialize the new part
	usize OldSize = pArrayProp->Size;
	memset(aNewArrayState + OldSize, 0, (NewSize - OldSize) * sizeof(*aNewArrayState));

	// Update ArrayProp. FIXME: use spinlock
	if (atomic_load_explicit(&pArrayProp->bResized, memory_order_relaxed))
		free(pArrayProp->aState); // Renderer hasn't took over the new array
	pArrayProp->aState = aNewArrayState;
	pArrayProp->Size = NewSize;
	atomic_store_explicit(&pArrayProp->bResized, true, memory_order_release);
}
*/

// UNSUPPORTED: Multiple threads on the same member
typedef uint8_t member_update_type;
#define MemberUpdateType_No              0 // Not used
#define MemberUpdateType_Attribute      (1 << 0)
#define MemberUpdateType_Value          (1 << 1)

static ext_forceinline void UpdateMember(
	usize iThread,
	array_prop* pArrayProp,
	usize iPosition,
	member_update_type UpdateType,
	bool bAddAttribute,
	visualizer_marker_attribute Attribute,
	visualizer_int Value
) {
	array_member* pArrayMember = &pArrayProp->aState[iPosition];

	// Most branches are known at compile time and can be optimized

	usize iColumn = NearestNeighborScale(iPosition, pArrayProp->Size, pArrayProp->ColumnCount);
	column_info* pColumn = &pArrayProp->aTls[iThread].aColumn[iColumn];

	if (UpdateType & MemberUpdateType_Attribute) {
		if (bAddAttribute)
			++pArrayMember->aMarkerCount[Attribute];
		else
			--pArrayMember->aMarkerCount[Attribute];
	}

	visualizer_int OldValue = Value;
	if (UpdateType & MemberUpdateType_Value)
		swap(&pArrayMember->Value, &OldValue);

	SpinLock_Lock(&pColumn->Lock);

	if (UpdateType & MemberUpdateType_Value)
		pColumn->ValueSum += (visualizer_long)Value - OldValue;
	if (UpdateType & MemberUpdateType_Attribute) {
		if (bAddAttribute)
			++pColumn->aMarkerCount[Attribute];
		else
			--pColumn->aMarkerCount[Attribute];
	}
	atomic_store_explicit(&pColumn->bUpdated, true, memory_order_relaxed);

	SpinLock_Unlock(&pColumn->Lock);
};

void Visualizer_UpdateArrayState(visualizer_array hArray, visualizer_int* aState) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);
	for (usize i = 0; i < pArrayProp->Size; ++i)
		UpdateMember(0, pArrayProp, i, MemberUpdateType_Value, false, 0, aState[i]);
}

// Marker helpers

static inline void AddMarker(usize iThread, array_prop* pArrayProp, usize iPosition, visualizer_marker_attribute Attribute) {
	UpdateMember(iThread, pArrayProp, iPosition, MemberUpdateType_Attribute, true, Attribute, 0);
}

static inline void AddMarkerWithValue(usize iThread, array_prop* pArrayProp, usize iPosition, visualizer_marker_attribute Attribute, visualizer_int Value) {
	UpdateMember(
		iThread,
		pArrayProp,
		iPosition,
		MemberUpdateType_Attribute | MemberUpdateType_Value,
		true,
		Attribute,
		Value
	);
}

static inline void RemoveMarkerHelper(usize iThread, array_prop* pArrayProp, usize iPosition, visualizer_marker_attribute Attribute) {
	UpdateMember(iThread, pArrayProp, iPosition, MemberUpdateType_Attribute, false, Attribute, 0);
}

// It does not update the iPosition member
static inline void MoveMarkerHelper(usize iThread, array_prop* pArrayProp, usize iPosition, usize iNewPosition, visualizer_marker_attribute Attribute) {
	UpdateMember(iThread, pArrayProp, iPosition, MemberUpdateType_Attribute, false, Attribute, 0);
	UpdateMember(iThread, pArrayProp, iNewPosition, MemberUpdateType_Attribute, true, Attribute, 0);
}

// Delays

void Visualizer_SetAlgorithmSleepMultiplier(floatptr_t fAlgorithmSleepMultiplier) {
	// Doesn't make sense to be atomic as it is only set at the beginning of the algorithm.
	gfAlgorithmSleepMultiplier = fAlgorithmSleepMultiplier;
}

void Visualizer_SetUserSleepMultiplier(floatptr_t fUserSleepMultiplier) {
	atomic_store_explicit(&gfUserSleepMultiplier, fUserSleepMultiplier, memory_order_relaxed);
}

void Visualizer_Sleep(floatptr_t fSleepMultiplier) {
#ifdef VISUALIZER_DISABLE_SLEEP
#else
	sleep64(
		(uint64_t)(
			gfDefaultDelay *
			gfAlgorithmSleepMultiplier *
			atomic_load_explicit(&gfUserSleepMultiplier, memory_order_relaxed) *
			fSleepMultiplier
		)
	);
#endif
}

// Read & Write helper

void Visualizer_UpdateRead(
	usize iThread,
	visualizer_array hArray,
	usize iPosition,
	floatptr_t fSleepMultiplier
) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);
	atomic_fetch_add_explicit(&pArrayProp->aTls[iThread].ReadCount, 1, memory_order_relaxed);
	AddMarker(iThread, pArrayProp, iPosition, Visualizer_MarkerAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	RemoveMarkerHelper(iThread, pArrayProp, iPosition, Visualizer_MarkerAttribute_Read);
}

void Visualizer_UpdateRead2(
	usize iThread,
	visualizer_array hArray,
	usize iPositionA,
	usize iPositionB,
	floatptr_t fSleepMultiplier
) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);
	atomic_fetch_add_explicit(&pArrayProp->aTls[iThread].ReadCount, 2, memory_order_relaxed);
	AddMarker(iThread, pArrayProp, iPositionA, Visualizer_MarkerAttribute_Read);
	AddMarker(iThread, pArrayProp, iPositionB, Visualizer_MarkerAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	RemoveMarkerHelper(iThread, pArrayProp, iPositionA, Visualizer_MarkerAttribute_Read);
	RemoveMarkerHelper(iThread, pArrayProp, iPositionB, Visualizer_MarkerAttribute_Read);
}

void Visualizer_UpdateReadMulti(
	usize iThread,
	visualizer_array hArray,
	usize iStartPosition,
	usize Length,
	floatptr_t fSleepMultiplier
) {
	assert(Length <= 16);
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);
	atomic_fetch_add_explicit(&pArrayProp->aTls[iThread].ReadCount, Length, memory_order_relaxed);
	for (usize i = 0; i < Length; ++i)
		AddMarker(iThread, pArrayProp, iStartPosition + i, Visualizer_MarkerAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	for (usize i = 0; i < Length; ++i)
		RemoveMarkerHelper(iThread, pArrayProp, iStartPosition + i, Visualizer_MarkerAttribute_Read);
}

void Visualizer_UpdateWrite(
	usize iThread,
	visualizer_array hArray,
	usize iPosition,
	visualizer_int NewValue,
	floatptr_t fSleepMultiplier
) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);
	atomic_fetch_add_explicit(&pArrayProp->aTls[iThread].WriteCount, 1, memory_order_relaxed);
	AddMarkerWithValue(iThread, pArrayProp, iPosition, Visualizer_MarkerAttribute_Write, NewValue);
	Visualizer_Sleep(fSleepMultiplier);
	RemoveMarkerHelper(iThread, pArrayProp, iPosition, Visualizer_MarkerAttribute_Write);
}

void Visualizer_UpdateReadWrite(
	usize iThread,
	visualizer_array hArrayA,
	visualizer_array hArrayB,
	usize iPositionA,
	usize iPositionB,
	floatptr_t fSleepMultiplier
) {
	array_prop* pArrayPropA = GetHandleData(&gArrayPropPool, hArrayA);
	array_prop* pArrayPropB = GetHandleData(&gArrayPropPool, hArrayB);
	atomic_fetch_add_explicit(&pArrayPropA->aTls[iThread].WriteCount, 1, memory_order_relaxed);
	atomic_fetch_add_explicit(&pArrayPropB->aTls[iThread].ReadCount, 1, memory_order_relaxed);
	visualizer_int NewValueA = pArrayPropB->aState[iPositionB].Value;
	AddMarkerWithValue(iThread, pArrayPropA, iPositionA, Visualizer_MarkerAttribute_Write, NewValueA);
	AddMarker(iThread, pArrayPropB, iPositionB, Visualizer_MarkerAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	RemoveMarkerHelper(iThread, pArrayPropA, iPositionA, Visualizer_MarkerAttribute_Write);
	RemoveMarkerHelper(iThread, pArrayPropB, iPositionB, Visualizer_MarkerAttribute_Read);
}

void Visualizer_UpdateSwap(
	usize iThread,
	visualizer_array hArray,
	usize iPositionA,
	usize iPositionB,
	floatptr_t fSleepMultiplier
) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);
	atomic_fetch_add_explicit(&pArrayProp->aTls[iThread].ReadCount, 2, memory_order_relaxed);
	atomic_fetch_add_explicit(&pArrayProp->aTls[iThread].WriteCount, 2, memory_order_relaxed);
	visualizer_int NewValueA = pArrayProp->aState[iPositionB].Value;
	visualizer_int NewValueB = pArrayProp->aState[iPositionA].Value;
	AddMarkerWithValue(iThread, pArrayProp, iPositionA, Visualizer_MarkerAttribute_Write, NewValueA);
	AddMarkerWithValue(iThread, pArrayProp, iPositionB, Visualizer_MarkerAttribute_Write, NewValueB);
	Visualizer_Sleep(fSleepMultiplier);
	RemoveMarkerHelper(iThread, pArrayProp, iPositionA, Visualizer_MarkerAttribute_Write);
	RemoveMarkerHelper(iThread, pArrayProp, iPositionB, Visualizer_MarkerAttribute_Write);
}

void Visualizer_UpdateWriteMulti(
	usize iThread,
	visualizer_array hArray,
	usize iStartPosition,
	usize Length,
	visualizer_int* aNewValue,
	floatptr_t fSleepMultiplier
) {
	assert(Length <= 16);
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);
	atomic_fetch_add_explicit(&pArrayProp->aTls[iThread].WriteCount, Length, memory_order_relaxed);
	for (usize i = 0; i < Length; ++i)
		AddMarkerWithValue(iThread, pArrayProp, iStartPosition + i, Visualizer_MarkerAttribute_Write, aNewValue[i]);
	Visualizer_Sleep(fSleepMultiplier);
	for (usize i = 0; i < Length; ++i)
		RemoveMarkerHelper(iThread, pArrayProp, iStartPosition + i, Visualizer_MarkerAttribute_Write);
}

// Marker

visualizer_marker Visualizer_CreateMarker(usize iThread, visualizer_array hArray, usize iPosition, visualizer_marker_attribute Attribute) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);
	AddMarker(iThread, pArrayProp, iPosition, Attribute);
	return (visualizer_marker){ hArray, iPosition, Attribute };
}

void Visualizer_RemoveMarker(usize iThread, visualizer_marker Marker) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, Marker.hArray);
	RemoveMarkerHelper(iThread, pArrayProp, Marker.iPosition, Marker.Attribute);
}

void Visualizer_MoveMarker(usize iThread, visualizer_marker* pMarker, usize iNewPosition) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, pMarker->hArray);
	MoveMarkerHelper(iThread, pArrayProp, pMarker->iPosition, iNewPosition, pMarker->Attribute);
	pMarker->iPosition = iNewPosition;
}

void Visualizer_SetAlgorithmName(char* sAlgorithmName) {
	if (sAlgorithmName == NULL)
		sAlgorithmName = "";

	usize Size = strlen(sAlgorithmName) + 1;
	char* sAlgorithmNameTemp = malloc_guarded(Size * sizeof(*sAlgorithmName)); // TODO: Use stack
	memcpy(sAlgorithmNameTemp, sAlgorithmName, Size * sizeof(*sAlgorithmName));

	SpinLock_Lock(&gAlgorithmNameLock);
	swap(&gsAlgorithmName, &sAlgorithmNameTemp);
	SpinLock_Unlock(&gAlgorithmNameLock);
	free(sAlgorithmNameTemp);
}

void Visualizer_ClearReadWriteCounter() {
	array_prop* pArrayProp = gpArrayPropHead; // TODO: Multiple arrays
	for (uint8_t i = 0; i < Visualizer_pThreadPool->ThreadCount; ++i) {
		atomic_store_explicit(&pArrayProp->aTls[i].ReadCount, 0, memory_order_relaxed);
		atomic_store_explicit(&pArrayProp->aTls[i].WriteCount, 0, memory_order_relaxed);
	}
}

// Correctness

static const visualizer_marker_attribute gaCorrectnessAttribute[2] = {
	Visualizer_MarkerAttribute_Incorrect,
	Visualizer_MarkerAttribute_Correct
};

void Visualizer_UpdateCorrectness(usize iThread, visualizer_array hArray, usize iPosition, bool bCorrect, floatptr_t fSleepMultiplier) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);
	visualizer_marker_attribute Attribute = gaCorrectnessAttribute[bCorrect];
	UpdateMember(iThread, pArrayProp, iPosition, MemberUpdateType_Attribute, true, Attribute, 0);
	Visualizer_Sleep(1.0);
}

void Visualizer_ClearCorrectness(usize iThread, visualizer_array hArray, usize iPosition, bool bCorrect) {
	array_prop* pArrayProp = GetHandleData(&gArrayPropPool, hArray);
	visualizer_marker_attribute Attribute = gaCorrectnessAttribute[bCorrect];
	UpdateMember(iThread, pArrayProp, iPosition, MemberUpdateType_Attribute, false, Attribute, 0);
}

// Timer

void Visualizer_StartTimer() {
	atomic_store_explicit(&gTimerStopTime, UINT64_MAX, memory_order_relaxed);
	atomic_store_explicit(&gTimerStartTime, clock64(), memory_order_release);
}

void Visualizer_StopTimer() {
	atomic_store_explicit(&gTimerStopTime, clock64(), memory_order_relaxed);
}

void Visualizer_ResetTimer() {
	atomic_store_explicit(&gTimerStopTime, 0, memory_order_relaxed);
	atomic_store_explicit(&gTimerStartTime, 0, memory_order_release);
}

