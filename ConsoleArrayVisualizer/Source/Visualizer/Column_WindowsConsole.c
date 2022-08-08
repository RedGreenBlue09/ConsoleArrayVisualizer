
#include "Visualizer.h"
#include <malloc.h>

// Buffer stuff
static HANDLE rendererBuffer = NULL;
static CONSOLE_SCREEN_BUFFER_INFOEX rendererCsbi = { 0 };

// Console attr
// cmd "color /?" explains this very well
static const USHORT conBackgroundAttr = 0x0F;

static const USHORT conNormalAttr = 0xF0;
static const USHORT conReadAttr = 0x10;
static const USHORT conWriteAttr = 0x40;

static const USHORT conPointerAttr = 0x30;

static const USHORT conCorrectAttr = 0x40;
static const USHORT conIncorrectAttr = 0x20;

// For uninitialization
static ULONG oldInputMode = 0;
static HANDLE oldBuffer = NULL;
LONG_PTR oldWindowStyle = 0;

// Array
typedef struct {
	AR_ARRAY* pArArray;

	// Array to keep track of atributes without reading console.
	// Useful in cases where value = 0 or too small to be rendered.
	uint8_t* attrBuffer;
	SHORT attrBufferN;

	// Map array index to console index (used for downscale).
	SHORT* cellMapping; // TODO: Dynamic array for upscale
} ARCNCL_ARRAY;

static ARCNCL_ARRAY arrayList[AR_MAX_ARRAY_COUNT];
// TODO: Linked list to keep track of active (added) items.

void arcnclInit() {
	//

	HWND Window = GetConsoleWindow();
	oldWindowStyle = GetWindowLongPtrW(Window, GWL_STYLE);
	SetWindowLongPtrW(Window, GWL_STYLE, oldWindowStyle & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);
	SetWindowPos(Window, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE);

	//

	rendererBuffer = cnCreateBuffer();

	//

	oldBuffer = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleActiveScreenBuffer(rendererBuffer);

	// Set console IO mode to RAW.

	GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &oldInputMode);
	SetConsoleMode(rendererBuffer, 0);
	SetConsoleMode(rendererBuffer, 0);

	// Set cursor to top left

	rendererCsbi.cbSize = sizeof(rendererCsbi);
	GetConsoleScreenBufferInfoEx(rendererBuffer, &rendererCsbi);
	rendererCsbi.dwCursorPosition = (COORD){ 0, 0 };
	rendererCsbi.wAttributes = conBackgroundAttr;
	SetConsoleScreenBufferInfoEx(rendererBuffer, &rendererCsbi);

	ULONG ul = GetLastError();

	//

	cnClear(rendererBuffer);

	return;
}

void arcnclUninit() {

	SetConsoleActiveScreenBuffer(oldBuffer);
	cnDeleteBuffer(rendererBuffer);

	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 0);

	HWND Window = GetConsoleWindow();
	SetWindowLongPtrW(Window, GWL_STYLE, oldWindowStyle);
	SetWindowPos(Window, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE);

}

void arcnclAddArray(intptr_t id) {
	//
	if (id >= AR_MAX_ARRAY_COUNT)
		return;

	arrayList[id].pArArray = arArrayList + id;

	// TODO: proper exit
	// Init cell mapping.

	intptr_t n = arrayList[id].pArArray->n;
	arrayList[id].cellMapping = malloc(n * sizeof(SHORT));
	if (!arrayList[id].cellMapping)
		exit(ERROR_NOT_ENOUGH_MEMORY);

	// Do scaling (not yet).

	for (intptr_t i = 0; i < n; ++i)
		arrayList[id].cellMapping[i] = (SHORT)i;

	// Init attr buffer

	//arrayList[id].attrBufferN = ((SHORT)n > rendererCsbi.dwSize.X) ? rendererCsbi.dwSize.X : (SHORT)n; // No upscale yet.
	arrayList[id].attrBufferN = (SHORT)n; // No upscale yet.
	arrayList[id].attrBuffer = malloc(arrayList[id].attrBufferN * sizeof(uint8_t));
	if (!arrayList[id].attrBuffer)
		exit(ERROR_NOT_ENOUGH_MEMORY);

	// TODO: scaling

	for (SHORT i = 0; i < arrayList[id].attrBufferN; ++i)
		arrayList[id].attrBuffer[i] = AR_ATTR_BACKGROUND;

	return;
}

// UB if array at id not added.
void arcnclRemoveArray(intptr_t id) {
	if (id >= AR_MAX_ARRAY_COUNT)
		return;

	free(arrayList[id].attrBuffer);
	free(arrayList[id].cellMapping);
	return;
}

static USHORT arcnclAttrToConAttr(uint8_t attr) {
	USHORT conAttrs[256] = { 0 };
	// 0: black background and black text. Make sense :)

	conAttrs[AR_ATTR_BACKGROUND] = conBackgroundAttr;
	conAttrs[AR_ATTR_NORMAL] = conNormalAttr;
	conAttrs[AR_ATTR_READ] = conReadAttr;
	conAttrs[AR_ATTR_WRITE] = conWriteAttr;
	conAttrs[AR_ATTR_POINTER] = conPointerAttr;
	conAttrs[AR_ATTR_CORRECT] = conCorrectAttr;
	conAttrs[AR_ATTR_INCORRECT] = conIncorrectAttr;
	return conAttrs[attr]; // return 0 on unknown attr.
}

void arcnclDrawItem(intptr_t arrayId, uintptr_t pos, isort_t value, uint8_t attr) {

	//printf("%llu\r\n",pos);

	// double for extra range
	isort_t valueMax = arrayList[arrayId].pArArray->valueMax;
	if (value > valueMax)
		value = valueMax;
	double dfHeight = (double)value * (double)rendererCsbi.dwSize.Y / (double)valueMax;
	SHORT height = (SHORT)dfHeight;

	// TODO: scaling: nearest neighbor.
	// TODO: negative values.

	// Save attribute
	arrayList[arrayId].attrBuffer[(SHORT)pos] = attr;
	USHORT conAttr = arcnclAttrToConAttr(attr);

	// Fill the unused cells with background.
	cnFillAttr(
		rendererBuffer,
		conBackgroundAttr,
		1,
		rendererCsbi.dwSize.Y - height,
		(COORD){ arrayList[arrayId].cellMapping[pos], 0 }
	);

	// Fill the used cells with conAttr.
	cnFillAttr(
		rendererBuffer,
		conAttr,
		1,
		height,
		(COORD){ arrayList[arrayId].cellMapping[pos], rendererCsbi.dwSize.Y - height }
	);
}

void arcnclReadItemAttr(intptr_t arrayId, uintptr_t pos, uint8_t* pAttr) {

	// TODO: scaling: nearest neighbor.
	// TODO: negative values.
	*pAttr = arrayList[arrayId].attrBuffer[(SHORT)pos];
	return;

}
