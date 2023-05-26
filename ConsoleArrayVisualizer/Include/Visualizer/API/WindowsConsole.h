#pragma once
#include <Windows.h>

// WindowsConsole.c

void WinConsole_FillStr(HANDLE hBuffer, CHAR* str, SHORT wX, SHORT wY, COORD coordLocation);
void WinConsole_FillChar(HANDLE hBuffer, CHAR ch, SHORT wX, SHORT wY, COORD coordLocation);
void WinConsole_FillAttr(HANDLE hBuffer, CONSOLE_SCREEN_BUFFER_INFOEX* pCSBI, WORD Attr, SHORT wX, SHORT wY, COORD coordLocation);
void WinConsole_FillAttrs(HANDLE hBuffer, WORD* attrs, SHORT wX, SHORT wY, COORD coordLocation);

void WinConsole_WriteStr(HANDLE hBuffer, CHAR* str, COORD coordLocation, ULONG ulLen);
void WinConsole_WriteChar(HANDLE hBuffer, CHAR ch, COORD coordLocation, ULONG ulLen);
void WinConsole_WriteAttr(HANDLE hBuffer, USHORT Attr, COORD coordLocation, ULONG ulLen);

void WinConsole_Clear(HANDLE hBuffer);
void WinConsole_Pause();

HANDLE* WinConsole_CreateBuffer();
void WinConsole_FreeBuffer(HANDLE hBuffer);
