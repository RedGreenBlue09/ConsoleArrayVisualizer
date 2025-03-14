#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "Utils/Common.h"
#include "Utils/Machine.h"

// Max number of threads: 127
typedef atomic int8_t semaphore;

static inline void Semaphore_Init(semaphore* pSemaphore, int8_t MaxCount) {
	*pSemaphore = MaxCount;
}

static inline void Semaphore_AcquireSingle(semaphore* pSemaphore) {
#if defined(MACHINE_ARM32) || defined(MACHINE_ARM64) /* TODO: Handle ARMv8.1 */
	// CAS version, is slower on x86 but might be faster on ARM
	// This version can support more threads if we use unsigned type
	int8_t CachedSemaphore;
	do {
		// Wait for release
		do {
			CachedSemaphore = *pSemaphore;
		} while (CachedSemaphore <= 0);
		// If not available, retry
		// Else decrease available count
	} while (!atomic_compare_exchange_weak(pSemaphore, &CachedSemaphore, CachedSemaphore - 1));
#else
	// FAA version
	while (true) {
		// Wait for release
		while (*pSemaphore <= 0);

		// Decrease available count
		if ((*pSemaphore)-- > 0)
			break;

		// If not available, release it
		++*pSemaphore;
	};
#endif
}

static inline bool Semaphore_TryAcquireSingle(semaphore* pSemaphore) {
#if defined(MACHINE_ARM32) || defined(MACHINE_ARM64) /* TODO: Handle ARMv8.1 */
	// CAS version, is slower on x86 but might be faster on ARM
	// This version can support more threads if we use unsigned type
	int8_t CachedSemaphore = *pSemaphore;
	if (CachedSemaphore <= 0)
		return false;

	// Decrease available count
	return atomic_compare_exchange_weak(pSemaphore, &CachedSemaphore, CachedSemaphore - 1);
#else
	if (*pSemaphore <= 0)
		return false;

	// Decrease available count
	if ((*pSemaphore)-- > 0)
		return true;

	// If not available, release it
	++*pSemaphore;
	return false;
#endif
}

static inline void Semaphore_ReleaseSingle(semaphore* pSemaphore) {
	++*pSemaphore;
}
