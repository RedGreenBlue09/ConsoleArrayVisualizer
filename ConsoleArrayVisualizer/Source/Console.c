
#include <Windows.h>
#include <malloc.h>
#include <stdio.h>

/*
* Console.c:
* Simplifies some Windows Console API calls
* and implements more functions.
*/

/*
* 
* Fill functions (by block)
* All cells outside of the console buffer will be ignored
* 
*/

void cnFillStr(HANDLE hBuffer, CHAR* str, SHORT wX, SHORT wY, COORD coordLocation) {

	CONSOLE_SCREEN_BUFFER_INFO CSBI;
	GetConsoleScreenBufferInfo(hBuffer, &CSBI);

	ULONG W;

	if (
		(coordLocation.X >= CSBI.dwSize.X) ||
		(coordLocation.Y >= CSBI.dwSize.Y)
		) {
		return;
	}
	
	SHORT originalX = wX;

	if ((coordLocation.X + wX) >= CSBI.dwSize.X)
		wX = CSBI.dwSize.X - coordLocation.X + 1;
	if ((coordLocation.Y + wY) >= CSBI.dwSize.Y)
		wY = CSBI.dwSize.Y - coordLocation.Y + 1;

	for (LONG i = 0; i < wY; ++i) {
		WriteConsoleOutputCharacterA(
			hBuffer,
			str,
			wX,
			coordLocation,
			&W
		);
		coordLocation.Y += 1;
		str += originalX;
	}

	return;
}

void cnFillChar(HANDLE hBuffer, CHAR ch, SHORT wX, SHORT wY, COORD coordLocation) {

	CONSOLE_SCREEN_BUFFER_INFO CSBI;
	GetConsoleScreenBufferInfo(hBuffer, &CSBI);

	ULONG W;

	if (
		coordLocation.X >= CSBI.dwSize.X ||
		coordLocation.Y >= CSBI.dwSize.Y
		) {
		return;
	}

	if ((coordLocation.X + wX) >= CSBI.dwSize.X)
		wX = CSBI.dwSize.X - coordLocation.X + 1;
	if ((coordLocation.Y + wY) >= CSBI.dwSize.Y)
		wY = CSBI.dwSize.Y - coordLocation.Y + 1;

	for (LONG i = 0; i < wY; ++i) {
		FillConsoleOutputCharacterA(
			hBuffer,
			ch,
			wX,
			coordLocation,
			&W
		);
		coordLocation.Y += 1;
	}

	return;
}

void cnFillAttr(HANDLE hBuffer, WORD attr, SHORT wX, SHORT wY, COORD coordLocation) {

	CONSOLE_SCREEN_BUFFER_INFO CSBI;
	GetConsoleScreenBufferInfo(hBuffer, &CSBI);

	ULONG W;

	if (
		coordLocation.X >= CSBI.dwSize.X ||
		coordLocation.Y >= CSBI.dwSize.Y
		) {
		return;
	}

	if ((coordLocation.X + wX) >= CSBI.dwSize.X)
		wX = CSBI.dwSize.X - coordLocation.X + 1;
	if ((coordLocation.Y + wY) >= CSBI.dwSize.Y)
		wY = CSBI.dwSize.Y - coordLocation.Y + 1;

	for (LONG i = 0; i < wY; ++i) {
		FillConsoleOutputAttribute(
			hBuffer,
			attr,
			wX,
			coordLocation,
			&W
		);
		coordLocation.Y += 1;
	}

	return;
}

void cnFillAttrs(HANDLE hBuffer, WORD* attrs, SHORT wX, SHORT wY, COORD coordLocation) {

	CONSOLE_SCREEN_BUFFER_INFO CSBI;
	GetConsoleScreenBufferInfo(hBuffer, &CSBI);

	ULONG W;

	if (
		coordLocation.X >= CSBI.dwSize.X ||
		coordLocation.Y >= CSBI.dwSize.Y
		) {
		return;
	}

	SHORT originalX = wX;

	if ((coordLocation.X + wX) >= CSBI.dwSize.X)
		wX = CSBI.dwSize.X - coordLocation.X + 1;
	if ((coordLocation.Y + wY) >= CSBI.dwSize.Y)
		wY = CSBI.dwSize.Y - coordLocation.Y + 1;

	for (LONG i = 0; i < wY; ++i) {
		WriteConsoleOutputAttribute(
			hBuffer,
			attrs,
			wX,
			coordLocation,
			&W
		);
		coordLocation.Y += 1;
		attrs += originalX;
	}

	return;
}

/*
* 
* Fill functions(by line)
* 
*/

void cnWriteStr(HANDLE hBuffer, CHAR* str, COORD coordLocation, ULONG ulLen) {

	ULONG W;
	WriteConsoleA(
		hBuffer,
		str,
		ulLen,
		&W,
		NULL
	);
	return;
}

void cnWriteChar(HANDLE hBuffer, CHAR ch, COORD coordLocation, ULONG ulLen) {

	ULONG W;
	FillConsoleOutputCharacterA(
		hBuffer,
		ch,
		ulLen,
		coordLocation,
		&W
	);
	return;
}

void cnWriteAttr(HANDLE hBuffer, USHORT attr, COORD coordLocation, ULONG ulLen) {

	ULONG W;
	FillConsoleOutputAttribute(
		hBuffer,
		attr,
		ulLen,
		coordLocation,
		&W
	);
	return;
}

/*
* Buffer functions
*/

HANDLE* cnCreateBuffer() {
	HANDLE hBuffer = CreateConsoleScreenBuffer(
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		CONSOLE_TEXTMODE_BUFFER,
		NULL
	);
	return hBuffer;
}

void cnDeleteBuffer(HANDLE hBuffer) {
	CloseHandle(hBuffer);
	return;
}

/*
* Extra functions
*/

// Similar to cmd "clear" command.
void cnClear(HANDLE hBuffer) {

	CONSOLE_SCREEN_BUFFER_INFO CSBI;
	GetConsoleScreenBufferInfo(hBuffer, &CSBI);

	ULONG W;
	FillConsoleOutputCharacterA(
		hBuffer,
		' ',
		CSBI.dwSize.X * CSBI.dwSize.Y,
		(COORD){ 0, 0 },
		& W
	);

	FillConsoleOutputAttribute(
		hBuffer,
		CSBI.wAttributes,
		CSBI.dwSize.X * CSBI.dwSize.Y,
		(COORD){ 0, 0 },
		& W
	);

	return;
}

// Similar to cmd "pause" command, no print.
void cnPause() {

	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);

	Sleep(250);
	FlushConsoleInputBuffer(hStdIn);
	INPUT_RECORD InputRecord;
	ULONG W;
	do {
		ReadConsoleInputA(hStdIn, &InputRecord, 1, &W);
	} while (InputRecord.EventType == KEY_EVENT);

	return;
}