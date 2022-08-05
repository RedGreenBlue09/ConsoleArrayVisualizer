#pragma once

#include <stdint.h>

#include <stdio.h>
#include <stdint.h>
#include <Windows.h>

#include "Sorts.h"

// Random.c

void srand64(uint64_t seed);
uint64_t rand64();

// Time.c

void utilInitTime();

uint64_t clock64();
uint64_t sleep64(uint64_t time);

// Threads.c (TODO)
