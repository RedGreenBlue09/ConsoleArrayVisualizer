#pragma once

#include <stdint.h>

// Machine pointer size detection

#if (UINTPTR_MAX == UINT32_MAX)
	#define MACHINE_PTR32 1
#elif (UINTPTR_MAX == UINT64_MAX)
	#define MACHINE_PTR64 1
#endif

// Machine architecture detection

#ifdef _MSC_VER
	#ifdef _M_IX86 
		#define MACHINE_IA32 1
	#elif _M_X64
		#define MACHINE_AMD64 1
	#elif _M_ARM
		#define MACHINE_ARM32 1
	#elif _M_ARM64
		#define MACHINE_ARM64 1
		#if __ARM_ARCH >= 801
			#define MACHINE_ARM64_ATOMICS 1
		#endif
	#endif
#elif __GNUC__
	#ifdef __i386__
		#define MACHINE_IA32 1
	#elif __x86_64__
		#define MACHINE_AMD64 1
	#elif __arm__
		#define MACHINE_ARM32 1
	#elif __aarch64__
		#define MACHINE_ARM64 1
		#if defined(__ARM_FEATURE_ATOMICS)
			#define MACHINE_ARM64_ATOMICS 1
		#endif
	#endif
#endif

uint8_t log2_uptr(uintptr_t X);
uint8_t log2_u64(uint64_t X);
