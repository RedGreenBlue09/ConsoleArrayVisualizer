#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "Utils/Atomic.h"
#include "Utils/Machine.h"

// Max number of threads: 127
typedef atomic int8_t semaphore;

static inline void Semaphore_Init(semaphore* pSemaphore, int8_t MaxCount) {
	atomic_init(pSemaphore, MaxCount);
}

static inline void Semaphore_AcquireSingle(semaphore* pSemaphore) {
#if defined(MACHINE_ARM32) || (defined(MACHINE_ARM64) && !defined(MACHINE_ARM64_ATOMICS))
	// CAS version, is slower on x86 but might be faster on ARM
	// This version can support more threads if we use unsigned type
	int8_t CachedSemaphore;
	do {
		// Wait for release
		do {
			CachedSemaphore = atomic_load_explicit(pSemaphore, memory_order_relaxed);
		} while (CachedSemaphore <= 0);
		// If not available, retry
		// Else decrease available count
	} while (
		!atomic_compare_exchange_weak_explicit(
			pSemaphore,
			&CachedSemaphore,
			CachedSemaphore - 1,
			memory_order_relaxed,
			memory_order_relaxed
		)
	);
	atomic_thread_fence_light(pSemaphore, memory_order_acquire);
#else
	// FAA version
	while (true) {
		// Wait for release
		while (atomic_load_explicit(pSemaphore, memory_order_relaxed) <= 0);

		// Decrease available count
		if (atomic_fetch_sub_explicit(pSemaphore, 1, memory_order_relaxed) > 0)
			break;

		// If not available, release it
		atomic_fetch_add_explicit(pSemaphore, 1, memory_order_relaxed);
	};
	atomic_thread_fence_light(pSemaphore, memory_order_acquire);
#endif
}

static inline bool Semaphore_TryAcquireSingle(semaphore* pSemaphore) {
#if defined(MACHINE_ARM32) || (defined(MACHINE_ARM64) && !defined(MACHINE_ARM64_ATOMICS))
	// CAS version, is slower on x86 but might be faster on ARM
	// This version can support more threads if we use unsigned type
	int8_t CachedSemaphore = atomic_load_explicit(pSemaphore, memory_order_relaxed);
	if (CachedSemaphore <= 0)
		return false;

	// Decrease available count
	if (
		!atomic_compare_exchange_weak_explicit(
			pSemaphore,
			&CachedSemaphore,
			CachedSemaphore - 1,
			memory_order_relaxed,
			memory_order_relaxed
		)
	)
		return false;
	
	atomic_thread_fence_light(pSemaphore, memory_order_acquire);
	return true;
#else
	if (atomic_load_explicit(pSemaphore, memory_order_relaxed) <= 0)
		return false;

	// Decrease available count
	if (atomic_fetch_sub_explicit(pSemaphore, 1, memory_order_relaxed) > 0) {
		atomic_thread_fence_light(pSemaphore, memory_order_acquire);
		return true;
	}

	// If not available, release it
	atomic_fetch_add_explicit(pSemaphore, 1, memory_order_relaxed);
	return false;
#endif
}

static inline void Semaphore_ReleaseSingle(semaphore* pSemaphore) {
	atomic_add_fence_light(pSemaphore, 1, memory_order_release);
}
