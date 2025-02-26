#pragma once
#include <inttypes.h>

// Time.c

// Resolution: 1 microsecond

uint64_t clock64();
uint64_t clock64_resolution();

uint64_t sleep64(uint64_t time);
