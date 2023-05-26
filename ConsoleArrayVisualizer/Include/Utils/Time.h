#pragma once
#include <inttypes.h>

// Time.c

void utilInitTime();

uint64_t clock64();
uint64_t sleep64(uint64_t time);
