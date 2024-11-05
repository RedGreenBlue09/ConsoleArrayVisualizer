
#include <threads.h>

#include "Visualizer/Visualizer.h"
#include "Utils/GuardedMalloc.h"
#include "Utils/SharedLock.h"
#include "Utils/LinkedList.h"

#include <Windows.h>
#include "Visualizer/Renderer/ColumnWindowsConsole.h"

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
	pool_index iArray;
	intptr_t iPosition;
	Visualizer_MarkerAttribute Attribute;
} Marker;

typedef struct {
	llist_node   Node;
	intptr_t     Size;
	isort_t      ValueMin;
	isort_t      ValueMax;
	ArrayMember* aState;
	// TODO: Horizontal scaling
} ArrayProp;

static pool ArrayPropPool;
static pool MarkerPool; // TODO: Ditch this pool
static ArrayProp* pArrayPropHead;

thrd_t RenderThread;
static bool bRun;

static int RenderThreadMain(void* pData) {

	while (bRun) {
		// TODO: Multi array
		// TODO: Handle array removal
		if (!pArrayPropHead)
			continue;

		ArrayProp* pArrayProp = pArrayPropHead;
		intptr_t Size = pArrayProp->Size;

		// Update cell cache
		for (intptr_t i = 0; i < Size; ++i) {
			RendererCwc_UpdateCellCache(pArrayProp, i);
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
	PoolInitialize(&MarkerPool, 256, sizeof(Marker));

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

	GetConsoleScreenBufferInfoEx(hAltBuffer, &BufferInfo);

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

	PoolDestroy(&MarkerPool);
	PoolDestroy(&ArrayPropPool);

}

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
	pArrayProp->aState = malloc_guarded(Size * sizeof(isort_t));
	if (aArrayState)
		for (intptr_t i = 0; i < Size; ++i)
			pArrayProp->aState[i] = (ArrayMember){ aArrayState[i] };
	else
		for (intptr_t i = 0; i < Size; ++i)
			pArrayProp->aState[i] = (ArrayMember){ 0 };

	pArrayProp->ValueMin = ValueMin;
	pArrayProp->ValueMax = ValueMax;

	return Visualizer_PoolIndexToHandle(Index);
}

void RendererCwc_RemoveArray(Visualizer_Handle hArray) {

	assert(ValidateHandle(&ArrayPropPool, hArray));
	pool_index Index = HandleToPoolIndex(hArray);
	ArrayProp* pArrayProp = PoolIndexToAddress(&ArrayPropPool, Index);

	free(pArrayProp->aState);
	// FIXME: Dealloc markers

	PoolDeallocate(&ArrayPropPool, Index);

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

void RendererCwc_AddMarker(
	Visualizer_Handle hArray,
	intptr_t iPosition,
	Visualizer_MarkerAttribute Attribute
) {
	assert(ValidateHandle(&ArrayPropPool, hArray));
	pool_index Index = HandleToPoolIndex(hArray);
	ArrayProp* pArrayProp = PoolIndexToAddress(&ArrayPropPool, Index);

	ArrayMember* pMember = &pArrayProp->aState[iPosition];

	sharedlock_lock_shared(&pMember->SharedLock);
	++pMember->aMarkerCount[Attribute];
	sharedlock_unlock_shared(&pMember->SharedLock);
}

void RendererCwc_AddMarkerWithValue(
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
}

void RendererCwc_RemoveMarker(
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
	--pMember->aMarkerCount[Attribute];
	sharedlock_unlock_shared(&pMember->SharedLock);
}

static const USHORT aWinConAttrTable[Visualizer_MarkerAttribute_EnumCount] = {
	ATTR_WINCON_READ,
	ATTR_WINCON_WRITE,
	ATTR_WINCON_POINTER,
	ATTR_WINCON_CORRECT,
	ATTR_WINCON_INCORRECT
};

static void RendererCwc_UpdateCellCache(
	ArrayProp* pArrayProp,
	intptr_t iPosition
) {
	ArrayMember* pMember = &pArrayProp->aState[iPosition];

	// Choose the correct attribute

	uint16_t ConsoleAttr;
	{
		// Find the attribute that have the most occurrences
		// TODO: SIMD

		sharedlock_lock_exclusive(&pMember->SharedLock);
		uint8_t MaxCount = pMember->aMarkerCount[0];
		Visualizer_MarkerAttribute Attr = 0;
		for (uint8_t i = 1; i < Visualizer_MarkerAttribute_EnumCount; ++i) {
			uint8_t Count = pMember->aMarkerCount[i];
			if (Count >= MaxCount) {
				MaxCount = Count;
				Attr = i;
			}
		}
		sharedlock_unlock_exclusive(&pMember->SharedLock);

		if (MaxCount == 0)
			ConsoleAttr = ATTR_WINCON_BACKGROUND;
		else
			ConsoleAttr = aWinConAttrTable[Attr];
	}

	// Choose the correct value

	isort_t Value = pMember->Value;

	isort_t ValueMin = pArrayProp->ValueMin;
	isort_t ValueMax = pArrayProp->ValueMax;

	Value -= ValueMin;
	ValueMax -= ValueMin; // FIXME: Underflow

	if (Value < 0) // TODO: Negative
		Value = 0;
	if (Value > ValueMax)
		Value = ValueMax;

	// Scale the value to the corresponding screen height

	double HeightFloat = (double)Value * (double)BufferInfo.dwSize.Y / (double)ValueMax;
	SHORT FloorHeight = (SHORT)HeightFloat;

	// Update the cell cache

	SHORT ConsoleCol = (SHORT)iPosition;
	if (ConsoleCol >= BufferInfo.dwSize.X)
		ConsoleCol = BufferInfo.dwSize.X - 1;

	{
		intptr_t i = 0;
		for (; i < (intptr_t)(BufferInfo.dwSize.Y - FloorHeight); ++i)
			aBufferCache[BufferInfo.dwSize.X * i + ConsoleCol].Attributes = ATTR_WINCON_BACKGROUND;
		for (; i < BufferInfo.dwSize.Y; ++i)
			aBufferCache[BufferInfo.dwSize.X * i + ConsoleCol].Attributes = ConsoleAttr;
	}
}
