
#include <stdint.h>

#include "Utils/Machine.h"

// Fast log2 for int64_t

#if _MSC_VER

#include <intrin.h>

	#if defined(MACHINE_PTR64)

#pragma intrinsic(_BitScanReverse64)

uint8_t log2_u64(uint64_t X) {
	unsigned long Result;
	_BitScanReverse64(&Result, X);
	return (uint8_t)Result;
}

	#elif defined(MACHINE_PTR32)

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
	 0, 47,  1, 56, 48, 27,  2, 60,
	57, 49, 41, 37, 28, 16,  3, 61,
	54, 58, 35, 52, 50, 42, 21, 44,
	38, 32, 29, 23, 17, 11,  4, 62,
	46, 55, 26, 59, 40, 36, 15, 53,
	34, 51, 20, 43, 31, 22, 10, 45,
	25, 39, 14, 33, 19, 30,  9, 24,
	13, 18,  8, 12,  7,  6,  5, 63
};

uint8_t log2_u64(uint64_t X) {
	X |= X >> 1;
	X |= X >> 2;
	X |= X >> 4;
	X |= X >> 8;
	X |= X >> 16;
	X |= X >> 32;
	return aTab64[(X * 0x03F79D71B4CB0A89) >> 58];
}

#endif

#if defined(MACHINE_PTR64)

uint8_t log2_uptr(uintptr_t X) {
	return log2_u64(X);
}

#elif defined(MACHINE_PTR32)

	#if _MSC_VER

#pragma intrinsic(_BitScanReverse)

uint8_t log2_uptr(uintptr_t X) {
	unsigned long Result;
	_BitScanReverse(&Result, X);
	return (uint8_t)Result;
}

	#elif __GNUC__

uint8_t log2_uptr(uintptr_t X) {
	return (uint8_t)(31 - __builtin_clz(X));
}

	#else

// Source: https://stackoverflow.com/questions/11376288/fast-computing-of-log2-for-64-bit-integers

static const uint8_t aTab32[32] = {
	 0,  9,  1, 10, 13, 21,  2, 29,
	11, 14, 16, 18, 22, 25,  3, 30,
	 8, 12, 20, 28, 15, 17, 24,  7,
	19, 27, 23,  6, 26,  5,  4, 31
};

uint8_t log2_uptr(uintptr_t X) {
	X |= X >> 1;
	X |= X >> 2;
	X |= X >> 4;
	X |= X >> 8;
	X |= X >> 16;
	return aTab32[(X * 0x07C4ACDD) >> 27];
}

	#endif

#endif