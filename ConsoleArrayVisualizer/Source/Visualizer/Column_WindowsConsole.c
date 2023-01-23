
#include "Visualizer.h"
#include <malloc.h>

#ifndef VISUALIZER_DISABLED

// Buffer stuff
static HANDLE rendererBuffer = NULL;
static CONSOLE_SCREEN_BUFFER_INFOEX rendererCsbi = { 0 };

// Console attr
// cmd "color /?" explains this very well
static const USHORT AttrBackground = 0x0F;

static const USHORT AttrNormal = 0xF0;
static const USHORT AttrRead = 0x10;
static const USHORT AttrWrite = 0x40;

static const USHORT AttrPointer = 0x30;

static const USHORT AttrCorrect = 0x40;
static const USHORT AttrIncorrect = 0x20;

// For uninitialization
static ULONG OldInputMode = 0;
static HANDLE hOldBuffer = NULL;
static LONG_PTR OldWindowStyle = 0;

// Array
typedef struct {
	V_ARRAY VArrayList;

	// Array to keep track of atributes without reading console.
	// Useful in cases where value = 0 or too small to be rendered.
	uint8_t* aAttribute;
	int16_t nAttribute;

	// TODO: Horizontal scaling

} ARCNCL_ARRAY;

static ARCNCL_ARRAY arrayList[AR_MAX_ARRAY_COUNT];
// TODO: Linked list to keep track of active (added) items.

void arcnclInit() {
	//

	HWND Window = GetConsoleWindow();
	OldWindowStyle = GetWindowLongPtrW(Window, GWL_STYLE);
	SetWindowLongPtrW(Window, GWL_STYLE, OldWindowStyle & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);
	SetWindowPos(Window, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE);

	//

	rendererBuffer = WinConsole_CreateBuffer();

	//

	hOldBuffer = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleActiveScreenBuffer(rendererBuffer);

	// Set console IO mode to RAW.

	GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &OldInputMode);
	SetConsoleMode(rendererBuffer, 0);
	SetConsoleMode(rendererBuffer, 0);

	// Set cursor to top left

	rendererCsbi.cbSize = sizeof(rendererCsbi);
	GetConsoleScreenBufferInfoEx(rendererBuffer, &rendererCsbi);
	rendererCsbi.dwCursorPosition = (COORD){ 0, 0 };
	rendererCsbi.wAttributes = AttrBackground;
	SetConsoleScreenBufferInfoEx(rendererBuffer, &rendererCsbi);

	ULONG ul = GetLastError();

	//

	WinConsole_Clear(rendererBuffer);

	return;
}

void arcnclUninit() {

	SetConsoleActiveScreenBuffer(hOldBuffer);
	WinConsole_FreeBuffer(rendererBuffer);

	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 0);

	HWND Window = GetConsoleWindow();
	SetWindowLongPtrW(Window, GWL_STYLE, OldWindowStyle);
	SetWindowPos(Window, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE);

}

void arcnclAddArray(V_ARRAY* parArray, intptr_t id) {
	//
	if (id >= AR_MAX_ARRAY_COUNT)
		return;

	arrayList[id].arArray = *parArray;

	// TODO: scaling
	intptr_t n = arrayList[id].arArray.n;

	// Init attr buffer

	arrayList[id].nAttribute = (SHORT)n; // No upscale yet.
	arrayList[id].aAttribute = malloc(n * sizeof(uint8_t));
	if (!arrayList[id].aAttribute)
		exit(ERROR_NOT_ENOUGH_MEMORY);

	// TODO: scaling

	for (SHORT i = 0; i < n; ++i)
		arrayList[id].aAttribute[i] = AR_ATTR_BACKGROUND;

	return;
}

// UB if array at id not added.
void arcnclRemoveArray(intptr_t id) {
	if (id >= AR_MAX_ARRAY_COUNT)
		return;

	free(arrayList[id].aAttribute);
	return;
}

USHORT conAttrs[256] = { 0 };
static USHORT arcnclAttrToConAttr(uint8_t attr) {
	// 0: black background and black text. Make sense :)

	conAttrs[AR_ATTR_BACKGROUND] = AttrBackground;
	conAttrs[AR_ATTR_NORMAL] = AttrNormal;
	conAttrs[AR_ATTR_READ] = AttrRead;
	conAttrs[AR_ATTR_WRITE] = AttrWrite;
	conAttrs[AR_ATTR_POINTER] = AttrPointer;
	conAttrs[AR_ATTR_CORRECT] = AttrCorrect;
	conAttrs[AR_ATTR_INCORRECT] = AttrIncorrect;
	return conAttrs[attr]; // return 0 on unknown attr.
}

void arcnclDrawItem(intptr_t arrayId, uintptr_t pos, isort_t value, uint8_t attr) {

	//printf("%llu\r\n",pos);

	// double for extra range
	isort_t valueMax = arrayList[arrayId].arArray.valueMax;
	if (value > valueMax)
		value = valueMax;
	double dfHeight = (double)value * (double)rendererCsbi.dwSize.Y / (double)valueMax;
	SHORT height = (SHORT)dfHeight;

	// TODO: scaling: nearest neighbor.
	// TODO: negative values.

	// Save attribute
	arrayList[arrayId].aAttribute[(SHORT)pos] = attr;
	USHORT conAttr = arcnclAttrToConAttr(attr);

	// Fill the unused cells with background.
	WinConsole_FillAttr(
		rendererBuffer,
		AttrBackground,
		1,
		rendererCsbi.dwSize.Y - height,
		(COORD){ pos, 0 }
	);

	// Fill the used cells with conAttr.
	WinConsole_FillAttr(
		rendererBuffer,
		conAttr,
		1,
		height,
		(COORD){ pos, rendererCsbi.dwSize.Y - height }
	);

	return;
}

void arcnclReadItemAttr(intptr_t arrayId, uintptr_t pos, uint8_t* pAttr) {

	// TODO: scaling: nearest neighbor.
	// TODO: negative values.
	*pAttr = arrayList[arrayId].aAttribute[(SHORT)pos];
	return;

}

#else
#endif
