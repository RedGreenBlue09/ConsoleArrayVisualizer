#pragma once

#include <stdint.h>

#include <stdio.h>
#include <stdint.h>
#include <Windows.h>

#include "Sorts.h"

// Machine pointer size detection

#ifdef MACHINE_PTR32
#undef MACHINE_PTR32
#endif
#ifdef MACHINE_PTR64
#undef MACHINE_PTR64
#endif

// Try detecting with stdint.h macros
#if (UINTPTR_MAX <= UINT32_MAX)

	#define MACHINE_PTR32

#elif (UINTPTR_MAX <= UINT64_MAX)

	#define MACHINE_PTR64

#else
// The above didn't work.
// Try detecting using compiler-specific macros.
	#ifdef _MSC_VER
		#ifdef _WIN32
			#define MACHINE_PTR32
		#else
			#define MACHINE_PTR64
		#endif
	#else
		// Still not work.
		#define MACHINE_PTRUNKN
	#endif

#endif

// Random.c

void srand64(uint64_t seed);
uint64_t rand64();

// Time.c

void utilInitTime();

uint64_t clock64();
uint64_t sleep64(uint64_t time);

// Threads.c (TODO)
