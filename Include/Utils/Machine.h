#pragma once

#include <stdint.h>

// Machine pointer size detection

#if (UINTPTR_MAX <= UINT32_MAX)
	#define MACHINE_PTR32 1
#elif (UINTPTR_MAX <= UINT64_MAX)
	#define MACHINE_PTR64 1
#endif

// Machine architecture detection

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
		#define MACHINE_IA32
	#elif __x86_64__
		#define MACHINE_AMD64
	#elif __ARM_ARCH_7A__
		#define MACHINE_ARM32
	#elif __aarch64__
		#define MACHINE_ARM64
	#endif
#endif
