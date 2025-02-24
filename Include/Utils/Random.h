#pragma once
#include <inttypes.h>

// Random.c

void srand64(uint64_t seed);
uint64_t rand64();
uint64_t rand64_bounded(uint64_t range);
