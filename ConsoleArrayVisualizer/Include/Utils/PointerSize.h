#pragma once

#include <stdint.h>

// Machine pointer size detection

#ifdef MACHINE_PTR32
#undef MACHINE_PTR32
#endif
#ifdef MACHINE_PTR64
#undef MACHINE_PTR64
#endif

// Try detecting with stdint.h macros
#if (UINTPTR_MAX <= UINT32_MAX)

	#define MACHINE_PTR32 1

#elif (UINTPTR_MAX <= UINT64_MAX)

	#define MACHINE_PTR64 1

#endif

#ifdef MACHINE_PTR32
	#define POINTER_SIZE 4
#elif MACHINE_PTR64
	#define POINTER_SIZE 8
#endif