#pragma once

#include <stdint.h>

#include <stdio.h>
#include <malloc.h>

#include <string.h>
#include <stdint.h>
#include <Windows.h>

#include "Sorts.h"

// Random.c

void srand64(uint64_t seed);
uint64_t rand64();

// Clock.c

void utilInitClock();

uint64_t clock64();
uint64_t sleep64(uint64_t time);

// RunSort.c

isort_t* rsCreateSortedArray(uintptr_t n);

void rsShuffle(isort_t* array, uintptr_t n);
void rsCheck(isort_t* array, isort_t* input, uintptr_t n);
void rsRunSort(SORT_INFO* si, isort_t* input, uintptr_t n);

// Console.c

void cnFillStr(HANDLE hBuffer, CHAR* str, USHORT wX, USHORT wY, COORD coordLocation);
void cnFillChar(HANDLE hBuffer, CHAR ch, USHORT wX, USHORT wY, COORD coordLocation);
void cnFillAttr(HANDLE hBuffer, WORD attr, USHORT wX, USHORT wY, COORD coordLocation);
void cnFillAttrs(HANDLE hBuffer, WORD* attrs, USHORT wX, USHORT wY, COORD coordLocation);

void cnWriteStr(HANDLE hBuffer, CHAR* str, COORD coordLocation, ULONG ulLen);
void cnWriteChar(HANDLE hBuffer, CHAR ch, COORD coordLocation, ULONG ulLen);
void cnWriteAttr(HANDLE hBuffer, USHORT attr, COORD coordLocation, ULONG ulLen);

void cnClear(HANDLE hBuffer);
void cnPause();

HANDLE* cnCreateBuffer();
void cnDeleteBuffer(HANDLE hBuffer);

//
