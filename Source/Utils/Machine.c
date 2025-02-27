
#include <stdint.h>

#include "Utils/Machine.h"

// Fast log2 for int64_t

#if _MSC_VER

#include <intrin.h>

	#ifdef MACHINE_PTR64

#pragma intrinsic(_BitScanReverse64)

uint8_t log2_u64(uint64_t X) {
	unsigned long Result;
	_BitScanReverse64(&Result, X);
	return (uint8_t)Result;
}

	#else

#pragma intrinsic(_BitScanReverse)

uint8_t log2_u64(uint64_t X) {
	unsigned long Result;
	if (_BitScanReverse(&Result, X >> 32))
		return (uint8_t)Result + 32;
	_BitScanReverse(&Result, (uint32_t)X);
	return (uint8_t)Result;
}

	#endif

#elif __GNUC__

uint8_t log2_u64(uint64_t X) {
	return (uint8_t)(63 - __builtin_clzll(X));
}

#else

// Source: https://www.chessprogramming.org/BitScan#De_Bruijn_Multiplication_2

static const uint8_t aTab64[64] = {
	63,  0, 58,  1, 59, 47, 53,  2,
	60, 39, 48, 27, 54, 33, 42,  3,
	61, 51, 37, 40, 49, 18, 28, 20,
	55, 30, 34, 11, 43, 14, 22,  4,
	62, 57, 46, 52, 38, 26, 32, 41,
	50, 36, 17, 19, 29, 10, 13, 21,
	56, 45, 25, 31, 35, 16,  9, 12,
	44, 24, 15,  8, 23,  7,  6,  5
};

uint8_t log2_u64(uint64_t X) {
	X |= X >> 1;
	X |= X >> 2;
	X |= X >> 4;
	X |= X >> 8;
	X |= X >> 16;
	X |= X >> 32;
	return aTab64[((uint64_t)((X - (X >> 1)) * 0x07EDD5E59A4E28C2)) >> 58];
}

#endif
