
#include "Visualizer/Visualizer.h"
#include <threads.h>
#include <Windows.h>

#include "Utils/GuardedMalloc.h"
#include "Utils/LinkedList.h"
#include "Utils/SharedLock.h"
#include "Utils/Time.h"

//
//#include <stdio.h>

// Buffer stuff
static HANDLE hAltBuffer = NULL;
static CONSOLE_SCREEN_BUFFER_INFOEX BufferInfo = { 0 };
static CHAR_INFO* aBufferCache;
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

// Array
typedef struct {
	_Atomic isort_t Value;
	_Atomic uint8_t aMarkerCount[Visualizer_MarkerAttribute_EnumCount];
	sharedlock SharedLock;
} ArrayMember;

typedef struct {
	llist_node       Node;
	_Atomic bool     bRemoved;
	// _Atomic bool     bSizeUpdated;
	_Atomic bool     bRangeUpdated;
	_Atomic intptr_t Size;
	_Atomic isort_t  ValueMin;
	_Atomic isort_t  ValueMax;
	ArrayMember*     aState;
	// TODO: Horizontal scaling
} ArrayProp;

static pool ArrayPropPool;

//static pool MarkerPool; // TODO: Ditch this pool
static ArrayProp* pArrayPropHead;

thrd_t RenderThread;
static _Atomic bool bRun;

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

static void UpdateCellCache(ArrayProp* pArrayProp, intptr_t iPosition);

static int RenderThreadMain(void* pData) {

	while (bRun) {

		sleep64(15625);

		// TODO: Multi array
		if (!pArrayPropHead)
			continue;

		ArrayProp* pArrayProp = pArrayPropHead;

		if (pArrayProp->bRemoved) {
			PoolDeallocateAddress(&ArrayPropPool, pArrayProp);
			pArrayPropHead = NULL;
			continue;
		}

		intptr_t Size = pArrayProp->Size;
		// Update cell cache
		for (intptr_t i = 0; i < Size; ++i) {
			UpdateCellCache(pArrayProp, i);
		}

		// Write to console
		// TODO: Write only updated regions
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
			(COORD) { 0, 0 },
			&Rect
		);
	}

	return 0;

}

void RendererCwc_Initialize() {

	PoolInitialize(&ArrayPropPool, 16, sizeof(ArrayProp));

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

	// GetConsoleScreenBufferInfoEx(hAltBuffer, &BufferInfo);

	// Initialize buffer cache

	LONG BufferSize = BufferInfo.dwSize.X * BufferInfo.dwSize.Y;
	aBufferCache = malloc_guarded(BufferSize * sizeof(*aBufferCache));
	for (intptr_t i = 0; i < BufferSize; ++i) {
		aBufferCache[i].Char.UnicodeChar = ' ';
		aBufferCache[i].Attributes = ATTR_WINCON_BACKGROUND;
	}

	// Clear screen

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
		(COORD) { 0, 0 },
		&Rect
	);

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
	pArrayProp->aState = malloc_guarded(Size * sizeof(ArrayMember));
	if (aArrayState)
		for (intptr_t i = 0; i < Size; ++i)
			pArrayProp->aState[i] = (ArrayMember){ aArrayState[i] };
	else
		for (intptr_t i = 0; i < Size; ++i)
			pArrayProp->aState[i] = (ArrayMember){ 0 };

	pArrayProp->ValueMin = ValueMin;
	pArrayProp->ValueMax = ValueMax;
	pArrayProp->bRemoved = false;

	if (!pArrayPropHead) // TODO: Multi array
		pArrayPropHead = pArrayProp;
	return PoolIndexToHandle(Index);
}

void RendererCwc_RemoveArray(Visualizer_Handle hArray) {

	assert(ValidateHandle(&ArrayPropPool, hArray));
	pool_index Index = HandleToPoolIndex(hArray);
	ArrayProp* pArrayProp = PoolIndexToAddress(&ArrayPropPool, Index);

	free(pArrayProp->aState);

	pArrayProp->bRemoved = true;
	//PoolDeallocate(&ArrayPropPool, Index);

}

void RendererCwc_UpdateArrayState(Visualizer_Handle hArray, isort_t* aState) {
	assert(ValidateHandle(&ArrayPropPool, hArray));
	pool_index Index = HandleToPoolIndex(hArray);
	ArrayProp* pArrayProp = PoolIndexToAddress(&ArrayPropPool, Index);

	for (intptr_t i = 0; i < pArrayProp->Size; ++i) {
		ArrayMember* pMember = &pArrayProp->aState[i];
		//sharedlock_lock_shared(&pMember->SharedLock);
		pMember->Value = aState[i];
		//sharedlock_unlock_shared(&pMember->SharedLock);
	}
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

Visualizer_Marker RendererCwc_AddMarker(
	Visualizer_Handle hArray,
	intptr_t iPosition,
	Visualizer_MarkerAttribute Attribute
) {
	assert(ValidateHandle(&ArrayPropPool, hArray));
	pool_index Index = HandleToPoolIndex(hArray);
	ArrayProp* pArrayProp = PoolIndexToAddress(&ArrayPropPool, Index);

	ArrayMember* pMember = &pArrayProp->aState[iPosition];

	//sharedlock_lock_shared(&pMember->SharedLock);
	++pMember->aMarkerCount[Attribute];
	//sharedlock_unlock_shared(&pMember->SharedLock);

	return (Visualizer_Marker){ hArray , iPosition, Attribute };
}

Visualizer_Marker RendererCwc_AddMarkerWithValue(
	Visualizer_Handle hArray,
	intptr_t iPosition,
	Visualizer_MarkerAttribute Attribute,
	isort_t Value
) {
	assert(ValidateHandle(&ArrayPropPool, hArray));
	pool_index Index = HandleToPoolIndex(hArray);
	ArrayProp* pArrayProp = PoolIndexToAddress(&ArrayPropPool, Index);

	ArrayMember* pMember = &pArrayProp->aState[iPosition];
	
	sharedlock_lock_shared(&pMember->SharedLock);
	++pMember->aMarkerCount[Attribute];
	pMember->Value = Value;
	sharedlock_unlock_shared(&pMember->SharedLock);

	return (Visualizer_Marker) { hArray, iPosition, Attribute };
}

void RendererCwc_RemoveMarker(Visualizer_Marker Marker) {
	assert(ValidateHandle(&ArrayPropPool, Marker.hArray));
	pool_index Index = HandleToPoolIndex(Marker.hArray);
	ArrayProp* pArrayProp = PoolIndexToAddress(&ArrayPropPool, Index);

	ArrayMember* pMember = &pArrayProp->aState[Marker.iPosition];

	//sharedlock_lock_shared(&pMember->SharedLock);
	--pMember->aMarkerCount[Marker.Attribute];
	//sharedlock_unlock_shared(&pMember->SharedLock);
}

void RendererCwc_MoveMarker(Visualizer_Marker* pMarker, intptr_t iNewPosition) {
	assert(ValidateHandle(&ArrayPropPool, pMarker->hArray));
	pool_index Index = HandleToPoolIndex(pMarker->hArray);
	ArrayProp* pArrayProp = PoolIndexToAddress(&ArrayPropPool, Index);

	// Remove from old position

	ArrayMember* pMember = &pArrayProp->aState[pMarker->iPosition];

	//sharedlock_lock_shared(&pMember->SharedLock);
	--pMember->aMarkerCount[pMarker->Attribute];
	//sharedlock_unlock_shared(&pMember->SharedLock);

	// Add to new position
	pMarker->iPosition = iNewPosition;

	pMember = &pArrayProp->aState[pMarker->iPosition];

	//sharedlock_lock_shared(&pMember->SharedLock);
	++pMember->aMarkerCount[pMarker->Attribute];
	//sharedlock_unlock_shared(&pMember->SharedLock);
}

static const USHORT aWinConAttrTable[Visualizer_MarkerAttribute_EnumCount] = {
	ATTR_WINCON_READ,
	ATTR_WINCON_WRITE,
	ATTR_WINCON_POINTER,
	ATTR_WINCON_CORRECT,
	ATTR_WINCON_INCORRECT
};

static void UpdateCellCache(ArrayProp* pArrayProp, intptr_t iPosition) {
	ArrayMember* pMember = &pArrayProp->aState[iPosition];

	// Choose the correct value & attribute

	uint16_t ConsoleAttr;
	isort_t Value;

	sharedlock_lock_exclusive(&pMember->SharedLock);

	// Find the attribute that have the most occurrences. TODO: SIMD
	uint8_t MaxAttrCount = pMember->aMarkerCount[0];
	Visualizer_MarkerAttribute Attr = 0;
	for (uint8_t i = 1; i < Visualizer_MarkerAttribute_EnumCount; ++i) {
		uint8_t Count = pMember->aMarkerCount[i];
		if (Count >= MaxAttrCount) {
			MaxAttrCount = Count;
			Attr = i;
		}
	}

	Value = pMember->Value;

	sharedlock_unlock_exclusive(&pMember->SharedLock);

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

	double HeightFloat = (double)AbsoluteValue * (double)BufferInfo.dwSize.Y / (double)AbsoluteValueMax;
	SHORT FloorHeight = (SHORT)HeightFloat;

	// Update the cell cache

	SHORT ConsoleCol = (SHORT)iPosition;
	if (ConsoleCol > BufferInfo.dwSize.X - 1)
		ConsoleCol = BufferInfo.dwSize.X - 1;

	{
		intptr_t i = 0;
		for (; i < (intptr_t)(BufferInfo.dwSize.Y - FloorHeight); ++i)
			aBufferCache[BufferInfo.dwSize.X * i + ConsoleCol].Attributes = ATTR_WINCON_BACKGROUND;
		for (; i < BufferInfo.dwSize.Y; ++i)
			aBufferCache[BufferInfo.dwSize.X * i + ConsoleCol].Attributes = ConsoleAttr;
	}
}
