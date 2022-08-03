
#include "Utils.h"
#include "Sorts.h"

#include <Windows.h>

const uint64_t defaultSleepTime = 1000;
isort_t valueMax = 32768;

//
HANDLE rendererBuffer = NULL;
CONSOLE_SCREEN_BUFFER_INFOEX rendererCsbi = { 0 };

//
WORD nothingAttr = 0x0F;

// For uninitialization
static ULONG oldInputMode = 0;
static HANDLE oldBuffer = NULL;

void arSleep(double multiplier) {
	sleep64((uint64_t)((double)defaultSleepTime * multiplier));
	return;
}

void arInit() {

	//

	if (rendererBuffer == NULL)
		rendererBuffer = cnCreateBuffer();

	//

	oldBuffer = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleActiveScreenBuffer(rendererBuffer);

	// Set console IO mode to RAW.

	GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &oldInputMode);
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 0);
	SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), 0);

	// Set cursor to top left

	GetConsoleScreenBufferInfoEx(rendererBuffer, &rendererCsbi);
	rendererCsbi.dwCursorPosition = (COORD){ 0, 0 };
	rendererCsbi.wAttributes = nothingAttr; // cmd "color /?" explains this very well
	SetConsoleScreenBufferInfoEx(rendererBuffer, &rendererCsbi);

	//

	cnClear(rendererBuffer);

	return;
}

void arUninit() {

	SetConsoleActiveScreenBuffer(oldBuffer);
	cnDeleteBuffer(rendererBuffer);

	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 0);

	return;
}

void arDrawCol(isort_t* array, uintptr_t n, uintptr_t pos, USHORT attr) {

	isort_t value = array[pos];

	// double for extra range
	double dfHeight = (double)value * (double)rendererCsbi.dwSize.Y / (double)valueMax;
	USHORT height = (USHORT)dfHeight;

	// TODO: scaling: convert pos, height to buffer position.
	// TODO: negative value.
	cnFillAttr(
		rendererBuffer,
		attr,
		1,
		height,
		(COORD){ pos, rendererCsbi.dwSize.Y - 1 }
	);

	// Fill the rest with nothingAttr.
	cnFillAttr(
		rendererBuffer,
		nothingAttr,
		1,
		rendererCsbi.dwSize.Y - height,
		(COORD) {
		pos, rendererCsbi.dwSize.Y - 1
	}
	);
}


