
#include "Visualizer/Visualizer.h"
#include "Utils/GuardedMalloc.h"

#include <Windows.h>
#include "Visualizer/Renderer/ColumnWindowsConsole.h"

//
#include <stdio.h>

// Buffer stuff
static HANDLE hAltBuffer = NULL;
static CONSOLE_SCREEN_BUFFER_INFOEX csbiBufferCache = { 0 };
static CHAR_INFO* aciBufferCache;
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
static HANDLE hOldBuffer = NULL;
static LONG_PTR OldWindowStyle = 0;

// Array
typedef struct {

	AV_ARRAYPROP_RENDERER;

	// TODO: Horizontal scaling

} RCWC_ARRAYPROP;

static tree234* RendererCwc_ptreeGlobalArrayProp; // tree of RCWC_ARRAYPROP_RENDERER

static int RendererCwc_ArrayPropIdCmp(void* pA, void* pB) {
	RCWC_ARRAYPROP* pvapA = pA;
	RCWC_ARRAYPROP* pvapB = pB;
	return (pvapA->ArrayId > pvapB->ArrayId) - (pvapA->ArrayId < pvapB->ArrayId);
}

void RendererCwc_Initialize() {

	// Initialize RendererCwc_ptreeGlobalArrayProp

	RendererCwc_ptreeGlobalArrayProp = newtree234(RendererCwc_ArrayPropIdCmp);

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

	csbiBufferCache.cbSize = sizeof(csbiBufferCache);
	GetConsoleScreenBufferInfoEx(hAltBuffer, &csbiBufferCache);
	csbiBufferCache.dwCursorPosition = (COORD){ 0, 0 };
	csbiBufferCache.wAttributes = ATTR_WINCON_BACKGROUND;
	SetConsoleScreenBufferInfoEx(hAltBuffer, &csbiBufferCache);

	GetConsoleScreenBufferInfoEx(hAltBuffer, &csbiBufferCache);

	// Initialize buffer cache

	LONG BufferSize = csbiBufferCache.dwSize.X * csbiBufferCache.dwSize.Y;
	aciBufferCache = malloc_guarded(BufferSize * sizeof(CHAR_INFO));
	for (intptr_t i = 0; i < BufferSize; ++i) {
		aciBufferCache[i].Char.UnicodeChar = ' ';
		aciBufferCache[i].Attributes = ATTR_WINCON_BACKGROUND;
	}

	// Clear screen

	SMALL_RECT Rect = {
		0,
		0,
		csbiBufferCache.dwSize.X - 1,
		csbiBufferCache.dwSize.Y - 1,
	};
	WriteConsoleOutputW(
		hAltBuffer,
		aciBufferCache,
		csbiBufferCache.dwSize,
		(COORD){ 0, 0 },
		&Rect
	);

	return;
}

void RendererCwc_Uninitialize() {

	// Uninitialize RendererCwc_ptreeGlobalArrayProp

	for (
		RCWC_ARRAYPROP* prap = delpos234(RendererCwc_ptreeGlobalArrayProp, 0);
		prap != NULL;
		prap = delpos234(RendererCwc_ptreeGlobalArrayProp, 0)
	) {
		free(prap);
	}
	freetree234(RendererCwc_ptreeGlobalArrayProp);

	// Free alternate buffer

	SetConsoleActiveScreenBuffer(hOldBuffer);
	CloseHandle(hAltBuffer);

	// Restore window mode

	HWND hWindow = GetConsoleWindow();
	SetWindowLongPtrW(hWindow, GWL_STYLE, OldWindowStyle);
	SetWindowPos(hWindow, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE);

	free(aciBufferCache);

	return;

}

void RendererCwc_AddArray(intptr_t ArrayId, intptr_t Size) {

	RCWC_ARRAYPROP* prapArrayProp = malloc_guarded(sizeof(RCWC_ARRAYPROP));

	prapArrayProp->ArrayId = ArrayId;
	prapArrayProp->Size = Size;

	prapArrayProp->aArrayState = malloc_guarded(Size * sizeof(isort_t));
	for (intptr_t i = 0; i < Size; ++i)
		prapArrayProp->aArrayState[i] = 0;

	prapArrayProp->aAttribute = malloc_guarded(Size * sizeof(AvAttribute));
	for (intptr_t i = 0; i < Size; ++i)
		prapArrayProp->aAttribute[i] = AvAttribute_Normal;

	prapArrayProp->bVisible = false;
	prapArrayProp->ValueMin = 0;
	prapArrayProp->ValueMax = 1;

	// Add to tree

	add234(RendererCwc_ptreeGlobalArrayProp, prapArrayProp);

	return;

}

void RendererCwc_RemoveArray(intptr_t ArrayId) {

	RCWC_ARRAYPROP rapFind = { .ArrayId = ArrayId };
	RCWC_ARRAYPROP* prapArrayProp = find234(RendererCwc_ptreeGlobalArrayProp, &rapFind, NULL);

	free(prapArrayProp->aAttribute);
	free(prapArrayProp->aArrayState);

	// Remove from tree

	delpos234(RendererCwc_ptreeGlobalArrayProp, ArrayId);
	free(prapArrayProp);

	return;

}

void RendererCwc_UpdateArray(intptr_t ArrayId, isort_t NewSize, isort_t* aNewArrayState, bool bVisible, isort_t ValueMin, isort_t ValueMax) {

	RCWC_ARRAYPROP rapFind = { .ArrayId = ArrayId };
	RCWC_ARRAYPROP* prapArrayProp = find234(RendererCwc_ptreeGlobalArrayProp, &rapFind, NULL);

	// Clear screen
	
	for (intptr_t i = 0; i < prapArrayProp->Size; ++i) {

		RendererCwc_UpdateItem(
			ArrayId,
			i,
			AV_RENDERER_UPDATEVALUE,
			0,
			0
		);

	}

	prapArrayProp->bVisible = bVisible;
	prapArrayProp->ValueMin = ValueMin;
	prapArrayProp->ValueMax = ValueMax;

	// Handle array resize

	if ((NewSize > 0) && (NewSize != prapArrayProp->Size)) {

		// Realloc arrays

		isort_t* aResizedArrayState = realloc_guarded(
			prapArrayProp->aArrayState,
			NewSize * sizeof(isort_t)
		);

		AvAttribute* aResizedAttribute = realloc_guarded(
			prapArrayProp->aAttribute,
			NewSize * sizeof(AvAttribute)
		);


		intptr_t OldSize = prapArrayProp->Size;
		intptr_t NewPartSize = NewSize - OldSize;

		// Initialize the new part

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aResizedArrayState[OldSize + i] = 0;

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aResizedAttribute[OldSize + i] = AvAttribute_Normal;

		prapArrayProp->aArrayState = aResizedArrayState;
		prapArrayProp->aAttribute = aResizedAttribute;

		prapArrayProp->Size = NewSize;

	}

	isort_t* aArrayState = prapArrayProp->aArrayState;
	intptr_t Size = prapArrayProp->Size;

	// Handle new array state

	if (aNewArrayState)
		for (intptr_t i = 0; i < Size; ++i)
			aArrayState[i] = aNewArrayState[i];

	// Re-render with new props

	// Same attribute
	if (aNewArrayState) {

		for (intptr_t i = 0; i < Size; ++i) {
			RendererCwc_UpdateItem(
				ArrayId,
				i,
				AV_RENDERER_UPDATEVALUE,
				aNewArrayState[i],
				0
			);
		}

	} else {

		for (intptr_t i = 0; i < Size; ++i) {
			RendererCwc_UpdateItem(
				ArrayId,
				i,
				AV_RENDERER_UPDATEVALUE,
				aArrayState[i],
				0
			);
		}

	}

	return;

}

static USHORT RendererCwc_AttrToConAttr(AvAttribute Attribute) {
	USHORT WinConAttrTable[32] = { 0 };
	WinConAttrTable[AvAttribute_Background] = ATTR_WINCON_BACKGROUND;
	WinConAttrTable[AvAttribute_Normal] = ATTR_WINCON_NORMAL;
	WinConAttrTable[AvAttribute_Read] = ATTR_WINCON_READ;
	WinConAttrTable[AvAttribute_Write] = ATTR_WINCON_WRITE;
	WinConAttrTable[AvAttribute_Pointer] = ATTR_WINCON_POINTER;
	WinConAttrTable[AvAttribute_Correct] = ATTR_WINCON_CORRECT;
	WinConAttrTable[AvAttribute_Incorrect] = ATTR_WINCON_INCORRECT;
	return WinConAttrTable[Attribute]; // return 0 on unknown Attr.
}

void RendererCwc_UpdateItem(
	intptr_t ArrayId,
	uintptr_t iPos,
	uint32_t UpdateRequest,
	isort_t NewValue,
	AvAttribute NewAttr
) {

	RCWC_ARRAYPROP rapFind = { .ArrayId = ArrayId };
	RCWC_ARRAYPROP* prapArrayProp = find234(RendererCwc_ptreeGlobalArrayProp, &rapFind, NULL);

	// Choose the correct value & attribute

	isort_t TargetValue = prapArrayProp->aArrayState[iPos];
	if (UpdateRequest & AV_RENDERER_UPDATEVALUE)
		TargetValue = NewValue;

	AvAttribute TargetAttr = prapArrayProp->aAttribute[iPos];
	if (UpdateRequest & AV_RENDERER_UPDATEATTR)
		TargetAttr = NewAttr;

	prapArrayProp->aArrayState[iPos] = TargetValue;
	prapArrayProp->aAttribute[iPos] = TargetAttr;

	isort_t ValueMin = prapArrayProp->ValueMin;
	isort_t ValueMax = prapArrayProp->ValueMax;

	TargetValue -= ValueMin;
	ValueMax -= ValueMin; // Warning: Overflow

	if (TargetValue > ValueMax)
		TargetValue = ValueMax;

	// Scale the value to the corresponding screen height

	double dfHeight = (double)TargetValue * (double)csbiBufferCache.dwSize.Y / (double)ValueMax;
	SHORT FloorHeight = (SHORT)dfHeight;

	// Convert AvAttribute to windows console attribute
	USHORT TargetWinConAttr = RendererCwc_AttrToConAttr(TargetAttr);

	// Initialize aciNewCharCells & update buffer cache

	SHORT TargetConsoleCol = (SHORT)iPos;
	if (TargetConsoleCol >= csbiBufferCache.dwSize.X)
		TargetConsoleCol = csbiBufferCache.dwSize.X - 1;

	{
		intptr_t i = 0;
		for (i; i < (intptr_t)(csbiBufferCache.dwSize.Y - FloorHeight); ++i) {
			// aciBufferCache[csbiBufferCache.dwSize.X * Y + X]
			aciBufferCache[csbiBufferCache.dwSize.X * i + TargetConsoleCol].Attributes = ATTR_WINCON_BACKGROUND;
		}
		for (i; i < csbiBufferCache.dwSize.Y; ++i) {
			aciBufferCache[csbiBufferCache.dwSize.X * i + TargetConsoleCol].Attributes = TargetWinConAttr;
		}
	}

	// Write to console

	SMALL_RECT Rect = (SMALL_RECT){
		TargetConsoleCol,
		0,
		TargetConsoleCol,
		csbiBufferCache.dwSize.Y - 1,
	};

	WriteConsoleOutputW(
		hAltBuffer,
		aciBufferCache,
		csbiBufferCache.dwSize,
		(COORD){ TargetConsoleCol, 0 },
		&Rect
	);

	return;

}
