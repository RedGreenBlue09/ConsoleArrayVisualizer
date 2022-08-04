
#include "ArrayRenderer.h"
#include <malloc.h>

//
HANDLE rendererBuffer = NULL;
CONSOLE_SCREEN_BUFFER_INFOEX rendererCsbi = { 0 };

// Console attr
// cmd "color /?" explains this very well
static const USHORT conBackgroundAttr = 0x0F;
static const USHORT conNormalAttr = 0xF0;
static const USHORT conReadAttr = 0x10;
static const USHORT conWriteAttr = 0x40;
static const USHORT conPointerAttr = 0x20;

// For uninitialization
static ULONG oldInputMode = 0;
static HANDLE oldBuffer = NULL;

// Array to keep track of atributes without reading console.
// Useful in cases where value = 0 or too small to be rendered.
static SHORT internalAttrBufferN;
static uint8_t* internalAttrBuffer;

void arcnclInit() {
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

	// TODO: proper exit
	internalAttrBufferN = rendererCsbi.dwSize.X;
	internalAttrBuffer = malloc(internalAttrBufferN * sizeof(uint8_t));
	if (!internalAttrBuffer)
		exit(ERROR_NOT_ENOUGH_MEMORY);
	for (SHORT i = 0; i < internalAttrBufferN; ++i)
		internalAttrBuffer[i] = AR_ATTR_BACKGROUND;

	return;
}

void arcnclUninit() {

	free(internalAttrBuffer);

	SetConsoleActiveScreenBuffer(oldBuffer);
	cnDeleteBuffer(rendererBuffer);

	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 0);

}

static USHORT arcnclAttrToConAttr(uint8_t attr) {
	USHORT conAttrs[256] = { 0 }; // 0: black background and black text :)
	conAttrs[AR_ATTR_BACKGROUND] = conBackgroundAttr;
	conAttrs[AR_ATTR_NORMAL] = conNormalAttr;
	conAttrs[AR_ATTR_READ] = conReadAttr;
	conAttrs[AR_ATTR_WRITE] = conWriteAttr;
	conAttrs[AR_ATTR_POINTER] = conPointerAttr;
	return conAttrs[attr]; // return 0 on unknown attr.
}

void arcnclDrawItem(isort_t value, uintptr_t n, uintptr_t pos, uint8_t attr) {

	// double for extra range
	double dfHeight = (double)value * (double)rendererCsbi.dwSize.Y / (double)valueMax;
	SHORT height = (SHORT)dfHeight;

	// TODO: scaling: nearest neighbor.
	// TODO: negative values.

	// Save attribute
	internalAttrBuffer[(SHORT)pos] = attr;
	USHORT conAttr = arcnclAttrToConAttr(attr);

	// Fill the unused cells with background.
	cnFillAttr(
		rendererBuffer,
		conBackgroundAttr,
		1,
		rendererCsbi.dwSize.Y - height,
		(COORD){ (SHORT)pos, 0 }
	);

	// Fill the used cells with conAttr.
	cnFillAttr(
		rendererBuffer,
		conAttr,
		1,
		height,
		(COORD){ (SHORT)pos, rendererCsbi.dwSize.Y - height }
	);
}

void arcnclReadItemAttr(isort_t value, uintptr_t n, uintptr_t pos, uint8_t* pAttr) {

	// TODO: scaling: nearest neighbor.
	// TODO: negative values.
	*pAttr = internalAttrBuffer[(SHORT)pos];
	return;

}
