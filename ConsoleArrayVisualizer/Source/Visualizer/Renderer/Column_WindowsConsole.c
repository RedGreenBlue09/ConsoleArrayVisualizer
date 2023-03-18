
#include "Visualizer.h"
#include <malloc.h>

#ifndef VISUALIZER_DISABLED

// Buffer stuff
static HANDLE hRendererBuffer = NULL;
static CONSOLE_SCREEN_BUFFER_INFOEX csbiRenderer = { 0 };

// Console Attr
// cmd "color /?" explains this very well.
static const USHORT WinConAttrBackground = 0x0F;

static const USHORT WinConAttrNormal = 0xF0;
static const USHORT WinConAttrRead = 0x10;
static const USHORT WinConAttrWrite = 0x40;

static const USHORT WinConAttrPointer = 0x30;

static const USHORT WinConAttrCorrect = 0x40;
static const USHORT WinConAttrIncorrect = 0x20;

// For uninitialization
static ULONG OldInputMode = 0;
static HANDLE hOldBuffer = NULL;
static LONG_PTR OldWindowStyle = 0;

// Array
typedef struct {

	V_ARRAY* pVArray;

	// TODO: Horizontal scaling

} WCC_ARRAY;

static WCC_ARRAY aWccArrayList[AR_MAX_ARRAY_COUNT];
// TODO: Linked list to keep track of active (added) items.

void RendererWcc_Initialize() {
	//

	HWND hWindow = GetConsoleWindow();
	OldWindowStyle = GetWindowLongPtrW(hWindow, GWL_STYLE);
	SetWindowLongPtrW(hWindow, GWL_STYLE, OldWindowStyle & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);
	SetWindowPos(hWindow, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE);

	//

	hRendererBuffer = WinConsole_CreateBuffer();

	//

	hOldBuffer = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleActiveScreenBuffer(hRendererBuffer);

	// Set console IO mode to RAW.

	GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &OldInputMode);
	SetConsoleMode(hRendererBuffer, 0);
	SetConsoleMode(hRendererBuffer, 0);

	// Set cursor to top left

	csbiRenderer.cbSize = sizeof(csbiRenderer);
	GetConsoleScreenBufferInfoEx(hRendererBuffer, &csbiRenderer);
	csbiRenderer.dwCursorPosition = (COORD){ 0, 0 };
	csbiRenderer.wAttributes = WinConAttrBackground;
	SetConsoleScreenBufferInfoEx(hRendererBuffer, &csbiRenderer);

	GetConsoleScreenBufferInfoEx(hRendererBuffer, &csbiRenderer);

	ULONG ul = GetLastError();

	//

	WinConsole_Clear(hRendererBuffer);

	return;
}

void RendererWcc_Uninitialize() {

	SetConsoleActiveScreenBuffer(hOldBuffer);
	WinConsole_FreeBuffer(hRendererBuffer);

	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 0);

	HWND hWindow = GetConsoleWindow();
	SetWindowLongPtrW(hWindow, GWL_STYLE, OldWindowStyle);
	SetWindowPos(hWindow, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE);

}




void RendererWcc_AddArray(intptr_t ArrayId, V_ARRAY* pVArray) {

	// VArray structure is shared between Visualizer.c and the renderer.

	aWccArrayList[ArrayId].pVArray = pVArray;

	// TODO: scaling

	return;
}

void RendererWcc_RemoveArray(intptr_t ArrayId) {

	aWccArrayList[ArrayId].pVArray = NULL;

	return;

}

void RendererWcc_UpdateArray(intptr_t ArrayId, isort_t* aNewArrayState, int32_t bVisible, isort_t ValueMin, isort_t ValueMax) {

	intptr_t Size = aWccArrayList[ArrayId].pVArray->Size;

	for (intptr_t i = 0; i < Size; ++i) {
		uint8_t Attribute = aWccArrayList[ArrayId].pVArray->aAttribute[i];
		RendererWcc_DrawItem(ArrayId, i, aNewArrayState[i], Attribute);
	}

	return;

}




USHORT WinConAttrTable[256] = { 0 }; // 0: black background and black text.
static USHORT AttrToConAttr(uint8_t Attr) {

	WinConAttrTable[AR_ATTR_BACKGROUND] = WinConAttrBackground;
	WinConAttrTable[AR_ATTR_NORMAL] = WinConAttrNormal;
	WinConAttrTable[AR_ATTR_READ] = WinConAttrRead;
	WinConAttrTable[AR_ATTR_WRITE] = WinConAttrWrite;
	WinConAttrTable[AR_ATTR_POINTER] = WinConAttrPointer;
	WinConAttrTable[AR_ATTR_CORRECT] = WinConAttrCorrect;
	WinConAttrTable[AR_ATTR_INCORRECT] = WinConAttrIncorrect;
	return WinConAttrTable[Attr]; // return 0 on unknown Attr.

}

void RendererWcc_DrawItem(intptr_t ArrayId, uintptr_t iPos, isort_t Value, uint8_t Attr) {

	if (ArrayId > 0) {
		return;
		// TOOD: Multiple array render
	}

	isort_t ValueMin = aWccArrayList[ArrayId].pVArray->ValueMin;
	isort_t ValueMax = aWccArrayList[ArrayId].pVArray->ValueMax;

	Value -= ValueMin;
	ValueMax -= ValueMin; // Warning: Overflow

	if (Value > ValueMax) {
		Value = ValueMax;
	}

	double dfHeight = (double)Value * (double)csbiRenderer.dwSize.Y / (double)ValueMax;
	SHORT FloorHeight = (SHORT)dfHeight;

	//
	USHORT WinConAttr = AttrToConAttr(Attr);

	// Fill the unused cells with background.
	WinConsole_FillAttr(
		hRendererBuffer,
		&csbiRenderer,
		WinConAttrBackground,
		1,
		csbiRenderer.dwSize.Y - FloorHeight,
		(COORD){ (SHORT)iPos, 0 }
	);
	// Fill the used cells with WinConAttr.
	WinConsole_FillAttr(
		hRendererBuffer,
		&csbiRenderer,
		WinConAttr,
		1,
		FloorHeight,
		(COORD){ (SHORT)iPos, csbiRenderer.dwSize.Y - FloorHeight }
	);

	return;
}

#else
#endif
