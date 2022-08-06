
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


// Array
typedef struct {
	AR_ARRAY* arArray;

	// Array to keep track of atributes without reading console.
	// Useful in cases where value = 0 or too small to be rendered.
	uint8_t* attrBuffer;
	intptr_t attrBufferN;

	// Map array index to console index (used for downscale).
	SHORT* cellMapping; // TODO: Dynamic array for upscale
} ARCNCL_ARRAY_ITEM;

static ARCNCL_ARRAY_ITEM arrayList[AR_MAX_ARRAY_COUNT];
// TODO: Linked list to keep track of active (added) items.

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


	return;
}

void arcnclAddArray(intptr_t id, isort_t* array, intptr_t n) {
	//
	if (id >= AR_MAX_ARRAY_COUNT)
		return;
	arrayList[id].array = array;
	arrayList[id].n = n;

	// TODO: proper exit
	// Init cell mapping buffer.
	arrayList[id].cellMapping = malloc(n * sizeof(SHORT));
	if (!arrayList[id].cellMapping)
		exit(ERROR_NOT_ENOUGH_MEMORY);

	// Do scaling (not yet).
	for (intptr_t i = 0; i < n; ++i)
		arrayList[id].cellMapping[i] = (SHORT)i;

	arrayList[id].attrBufferN = (n > rendererCsbi.dwSize.X) ? rendererCsbi.dwSize.X : n; // No upscale yet.
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

void arcnclUninit() {

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
	conAttrs[AR_ATTR_CORRECT] = conCorrectAttr;
	conAttrs[AR_ATTR_INCORRECT] = conIncorrectAttr;
	return conAttrs[attr]; // return 0 on unknown attr.
}

void arcnclDrawItem(intptr_t arrayId, isort_t value, uintptr_t pos, uint8_t attr) {

	// double for extra range
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
