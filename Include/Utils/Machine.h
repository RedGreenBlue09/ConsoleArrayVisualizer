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

#ifdef _MSC_VER
	#ifdef _M_IX86 
		#define MACHINE_IA32
	#elif _M_X64
		#define MACHINE_AMD64
	#elif _M_ARM
		#define MACHINE_ARM32
	#elif _M_ARM64
		#define MACHINE_ARM64
	#endif
#elif __GNUC__
	#ifdef __i386__
		#define MACHINE_AMD64
	#elif __x86_64__
		#define MACHINE_IA32
	#elif __ARM_ARCH_7A__
		#define MACHINE_ARM32
	#elif __aarch64__
		#define MACHINE_ARM64
	#endif
#endif
