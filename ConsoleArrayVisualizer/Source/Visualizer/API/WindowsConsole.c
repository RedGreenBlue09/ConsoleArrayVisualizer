
#include <Windows.h>
#include <malloc.h>
#include <stdio.h>

/*
* Console.c:
* Simplifies some Windows Console API calls
* and implements more functions.
*/

// TODO: Change code style

/*
* 
* Fill functions (by block)
* All cells outside of the console buffer will be ignored
* 
*/

void WinConsole_FillStr(HANDLE hBuffer, CHAR* str, SHORT wX, SHORT wY, COORD coordLocation) {

	CONSOLE_SCREEN_BUFFER_INFO CSBI;
	GetConsoleScreenBufferInfo(hBuffer, &CSBI);

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

	CHAR_INFO* ciBlock;
	LONG blockN = wX * wY;
	ciBlock = malloc(blockN * sizeof(CHAR_INFO));
	if (ciBlock == NULL)
		return;

	SMALL_RECT rect = (SMALL_RECT){
		coordLocation.X,
		coordLocation.Y,
		coordLocation.X + wX,
		coordLocation.Y + wY,
	};
	ReadConsoleOutputA(
		hBuffer,
		ciBlock,
		(COORD){ wX, wY },
		(COORD){ 0, 0 },
		&rect
	);

	for (LONG i = 0; i < wY; ++i) {
		for (int j = 0; j < wX; ++j)
			ciBlock[i * wX + j].Char.AsciiChar = str[i * originalX + j];
	}

	WriteConsoleOutputA(
		hBuffer,
		ciBlock,
		(COORD){ wX, wY },
		(COORD){ 0, 0 },
		&rect
	);

	free(ciBlock);
	return;
}

void WinConsole_FillChar(HANDLE hBuffer, CHAR ch, SHORT wX, SHORT wY, COORD coordLocation) {

	CONSOLE_SCREEN_BUFFER_INFO CSBI;
	GetConsoleScreenBufferInfo(hBuffer, &CSBI);

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

	CHAR_INFO* ciBlock;
	LONG blockN = wX * wY;
	ciBlock = malloc(blockN * sizeof(CHAR_INFO));
	if (ciBlock == NULL)
		return;

	SMALL_RECT rect = (SMALL_RECT){
		coordLocation.X,
		coordLocation.Y,
		coordLocation.X + wX,
		coordLocation.Y + wY,
	};
	ReadConsoleOutputA(
		hBuffer,
		ciBlock,
		(COORD){ wX, wY },
		(COORD){ 0, 0 },
		&rect
	);

	for (LONG i = 0; i < wY; ++i) {
		for (int j = 0; j < wX; ++j)
			ciBlock[i * wX + j].Char.AsciiChar = ch;
	}

	WriteConsoleOutputA(
		hBuffer,
		ciBlock,
		(COORD){ wX, wY },
		(COORD){ 0, 0 },
		&rect
	);

	free(ciBlock);
	return;
}

void WinConsole_FillAttr(HANDLE hBuffer, CONSOLE_SCREEN_BUFFER_INFOEX* pCSBI, WORD attr, SHORT wX, SHORT wY, COORD coordLocation) {

	// Dummy code
	// TODO: Cache console buffer to eliminate slow ReadConsoleOutputW()

	CONSOLE_SCREEN_BUFFER_INFOEX CSBI = *pCSBI;
	// apparently GetConsoleScreenBufferInfo() is turtle slow
	//GetConsoleScreenBufferInfo(hBuffer, &CSBI);

	if (
		coordLocation.X >= CSBI.dwSize.X ||
		coordLocation.Y >= CSBI.dwSize.Y
		) {
		return;
	}

	SHORT originalX = wX;
	SHORT originalY = wY;

	if ((coordLocation.X + wX) >= CSBI.dwSize.X)
		wX = CSBI.dwSize.X - coordLocation.X + 1;
	if ((coordLocation.Y + wY) >= CSBI.dwSize.Y)
		wY = CSBI.dwSize.Y - coordLocation.Y + 1;

	CHAR_INFO* ciBlock;
	LONG blockN = wX * wY;
	ciBlock = malloc(blockN * sizeof(CHAR_INFO));
	if (ciBlock == NULL)
		return;

	SMALL_RECT rect = (SMALL_RECT){
		coordLocation.X,
		coordLocation.Y,
		coordLocation.X + wX,
		coordLocation.Y + wY,
	};
	ReadConsoleOutputW(
		hBuffer,
		ciBlock,
		(COORD){ wX, wY },
		(COORD){ 0, 0 },
		&rect
	);

	for (LONG i = 0; i < wY; ++i) {
		for (int j = 0; j < wX; ++j) {
			ciBlock[i * wX + j].Attributes = attr;
		}
	}

	WriteConsoleOutputW(
		hBuffer,
		ciBlock,
		(COORD){ wX, wY },
		(COORD){ 0, 0 },
		&rect
	);

	free(ciBlock);
	return;
}

void WinConsole_FillAttrs(HANDLE hBuffer, WORD* attrs, SHORT wX, SHORT wY, COORD coordLocation) {

	CONSOLE_SCREEN_BUFFER_INFO CSBI;
	GetConsoleScreenBufferInfo(hBuffer, &CSBI);

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

	CHAR_INFO* ciBlock;
	LONG blockN = wX * wY;
	ciBlock = malloc(blockN * sizeof(CHAR_INFO));
	if (ciBlock == NULL)
		return;

	SMALL_RECT rect = (SMALL_RECT){
		coordLocation.X,
		coordLocation.Y,
		coordLocation.X + wX,
		coordLocation.Y + wY,
	};
	ReadConsoleOutputW(
		hBuffer,
		ciBlock,
		(COORD){ wX, wY },
		(COORD){ 0, 0 },
		&rect
	);

	for (LONG i = 0; i < wY; ++i) {
		for (int j = 0; j < wX; ++j)
			ciBlock[i * wX + j].Attributes = attrs[i * originalX + j];
	}

	WriteConsoleOutputW(
		hBuffer,
		ciBlock,
		(COORD){ wX, wY },
		(COORD){ 0, 0 },
		&rect
	);

	free(ciBlock);
	return;
}

/*
* 
* Fill functions(by line)
* 
*/

void WinConsole_WriteStr(HANDLE hBuffer, CHAR* str, COORD coordLocation, ULONG ulLen) {

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

void WinConsole_WriteChar(HANDLE hBuffer, CHAR ch, COORD coordLocation, ULONG ulLen) {

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

void WinConsole_WriteAttr(HANDLE hBuffer, USHORT attr, COORD coordLocation, ULONG ulLen) {

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

HANDLE* WinConsole_CreateBuffer() {
	HANDLE hBuffer = CreateConsoleScreenBuffer(
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		CONSOLE_TEXTMODE_BUFFER,
		NULL
	);
	return hBuffer;
}

void WinConsole_FreeBuffer(HANDLE hBuffer) {
	CloseHandle(hBuffer);
	return;
}

/*
* Extra functions
*/

// Similar to cmd "clear" command.
void WinConsole_Clear(HANDLE hBuffer) {

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
void WinConsole_Pause() {

	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);

	Sleep(250);
	FlushConsoleInputBuffer(hStdIn);
	INPUT_RECORD InputRecord;
	ULONG W;
	do {
		ReadConsoleInputW(hStdIn, &InputRecord, 1, &W);
	} while (InputRecord.EventType == KEY_EVENT);

	return;
}