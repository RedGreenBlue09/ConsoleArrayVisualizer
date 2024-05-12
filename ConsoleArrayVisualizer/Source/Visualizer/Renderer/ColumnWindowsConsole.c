
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

	pool_index                  ArrayIndex;
	intptr_t                    Size;
	isort_t*                    aState;
	Visualizer_MarkerAttribute* aAttribute;

	isort_t      ValueMin;
	isort_t      ValueMax;

	// TODO: Horizontal scaling
} RendererCwc_ArrayProp;

static intptr_t RendererCwc_nArrayProp = 0;
static RendererCwc_ArrayProp* RendererCwc_aArrayProp = NULL;

void RendererCwc_Initialize(intptr_t nMaxArray) {

	// Initialize RendererCwc_aArrayProp

	RendererCwc_nArrayProp = nMaxArray;
	RendererCwc_aArrayProp = malloc_guarded(nMaxArray * sizeof(*RendererCwc_aArrayProp));

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

	// Uninitialize RendererCwc_aArrayProp

	free(RendererCwc_aArrayProp);

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

void RendererCwc_AddArray(
	pool_index ArrayIndex,
	intptr_t Size,
	isort_t* aArrayState,
	isort_t ValueMin,
	isort_t ValueMax
) {

	RendererCwc_ArrayProp* pArrayProp = RendererCwc_aArrayProp + (uintptr_t)ArrayIndex;

	pArrayProp->ArrayIndex = ArrayIndex;
	pArrayProp->Size = Size;

	pArrayProp->aState = malloc_guarded(Size * sizeof(isort_t));
	if (aArrayState)
		for (intptr_t i = 0; i < Size; ++i)
			pArrayProp->aState[i] = aArrayState[i];
	else
		for (intptr_t i = 0; i < Size; ++i)
			pArrayProp->aState[i] = 0;

	pArrayProp->aAttribute = malloc_guarded(Size * sizeof(Visualizer_MarkerAttribute));
	for (intptr_t i = 0; i < Size; ++i)
		pArrayProp->aAttribute[i] = Visualizer_MarkerAttribute_Normal;

	pArrayProp->ValueMin = ValueMin;
	pArrayProp->ValueMax = ValueMax;

	return;

}

void RendererCwc_RemoveArray(pool_index ArrayIndex) {

	RendererCwc_ArrayProp* pArrayProp = RendererCwc_aArrayProp + (uintptr_t)ArrayIndex;

	free(pArrayProp->aAttribute);
	free(pArrayProp->aState);

	return;

}

void RendererCwc_UpdateArray(
	pool_index ArrayIndex,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
) {

	RendererCwc_ArrayProp* pArrayProp = RendererCwc_aArrayProp + (uintptr_t)ArrayIndex;

	// Clear screen

	int32_t Written;
	FillConsoleOutputAttribute(
		hAltBuffer,
		ATTR_WINCON_BACKGROUND,
		csbiBufferCache.dwSize.X * csbiBufferCache.dwSize.Y,
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

	intptr_t Size = pArrayProp->Size;
	for (intptr_t i = 0; i < Size; ++i) {
		RendererCwc_UpdateItem(
			ArrayIndex,
			i,
			AV_RENDERER_NOUPDATE,
			0,
			0
		);
	}

	return;

}

static USHORT RendererCwc_AttrToConAttr(Visualizer_MarkerAttribute Attribute) {
	USHORT WinConAttrTable[32] = { 0 };
	WinConAttrTable[Visualizer_MarkerAttribute_Background] = ATTR_WINCON_BACKGROUND;
	WinConAttrTable[Visualizer_MarkerAttribute_Normal] = ATTR_WINCON_NORMAL;
	WinConAttrTable[Visualizer_MarkerAttribute_Read] = ATTR_WINCON_READ;
	WinConAttrTable[Visualizer_MarkerAttribute_Write] = ATTR_WINCON_WRITE;
	WinConAttrTable[Visualizer_MarkerAttribute_Pointer] = ATTR_WINCON_POINTER;
	WinConAttrTable[Visualizer_MarkerAttribute_Correct] = ATTR_WINCON_CORRECT;
	WinConAttrTable[Visualizer_MarkerAttribute_Incorrect] = ATTR_WINCON_INCORRECT;
	return WinConAttrTable[Attribute]; // return 0 on unknown Attr.
}

void RendererCwc_UpdateItem(
	pool_index ArrayIndex,
	intptr_t iPosition,
	uint32_t UpdateRequest,
	isort_t NewValue,
	Visualizer_MarkerAttribute NewAttr
) {

	RendererCwc_ArrayProp* pArrayProp = RendererCwc_aArrayProp + (uintptr_t)ArrayIndex;

	// Choose the correct value & attribute

	isort_t TargetValue;
	if (UpdateRequest & AV_RENDERER_UPDATEVALUE)
		TargetValue = NewValue;
	else
		TargetValue = pArrayProp->aState[iPosition];

	Visualizer_MarkerAttribute TargetAttr;
	if (UpdateRequest & AV_RENDERER_UPDATEATTR)
		TargetAttr = NewAttr;
	else
		TargetAttr = pArrayProp->aAttribute[iPosition];

	pArrayProp->aState[iPosition] = TargetValue;
	pArrayProp->aAttribute[iPosition] = TargetAttr;

	isort_t ValueMin = pArrayProp->ValueMin;
	isort_t ValueMax = pArrayProp->ValueMax;

	TargetValue -= ValueMin;
	ValueMax -= ValueMin; // FIXME: Underflow

	if (TargetValue < 0) // TODO: Negative
		TargetValue = 0;
	if (TargetValue > ValueMax)
		TargetValue = ValueMax;

	// Scale the value to the corresponding screen height

	double HeightFloat = (double)TargetValue * (double)csbiBufferCache.dwSize.Y / (double)ValueMax;
	SHORT FloorHeight = (SHORT)HeightFloat;

	// Convert Visualizer_MarkerAttribute to windows console attribute

	USHORT TargetWinConAttr = RendererCwc_AttrToConAttr(TargetAttr);

	// Initialize update buffer cache

	SHORT TargetConsoleCol = (SHORT)iPosition;
	if (TargetConsoleCol >= csbiBufferCache.dwSize.X)
		TargetConsoleCol = csbiBufferCache.dwSize.X - 1;

	{
		intptr_t i = 0;
		for (i; i < (intptr_t)(csbiBufferCache.dwSize.Y - FloorHeight); ++i) {
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
