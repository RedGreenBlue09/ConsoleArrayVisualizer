#pragma once

// Aka. Reader-Writer lock

#include <stdbool.h>
#include <stdint.h>

#include "Utils/Atomic.h"
#include "Utils/Machine.h"

// Last bit for exclusive lock, the rest is shared lock count
// Max number of shared locks: 127
typedef atomic uint8_t sharedlock;

static inline void SharedLock_Init(sharedlock* pLock) {
	atomic_init(pLock, 0);
}

static inline void SharedLock_LockExclusive(sharedlock* pLock) {
	// Set exclusive lock bit
	atomic_fetch_or_explicit(pLock, 1 << 7, memory_order_relaxed);
	// Wait for all shared locks
	while (atomic_load_explicit(pLock, memory_order_relaxed) > (1 << 7));
	atomic_thread_fence_light(pLock, memory_order_acquire);
}

static inline void SharedLock_UnlockExclusive(sharedlock* pLock) {
	// Unset exclusive lock bit
	atomic_and_fence_light(pLock, ~(1 << 7), memory_order_release);
}

static inline void SharedLock_LockShared(sharedlock* pLock) {
#if MACHINE_LLSC_ATOMICS
	// CAS version, is slower on x86 but might be faster on ARM
	uint8_t CachedLock;
	do {
		// Wait for exclusive lock
		do {
			CachedLock = atomic_load_explicit(pLock, memory_order_relaxed);
		} while (CachedLock >= (1 << 7));
		// If it's exclusively locked again or increased, retry
		// Else increase shared lock count
	} while (
		!atomic_compare_exchange_weak_explicit(
			pLock,
			&CachedLock,
			CachedLock + 1,
			memory_order_relaxed,
			memory_order_relaxed
		)
	);
	atomic_thread_fence_light(pLock, memory_order_acquire);
#else
	// FAA version
	while (true) {
		// Wait for exclusive lock
		while (atomic_load_explicit(pLock, memory_order_relaxed) >= (1 << 7));

		// Increase shared lock count
		if (atomic_fetch_add_explicit(pLock, 1, memory_order_relaxed) < (1 << 7))
			break;

		// If already exclusively locked, unlock and retry
		atomic_fetch_sub_explicit(pLock, 1, memory_order_relaxed);
	};
	atomic_thread_fence_light(pLock, memory_order_acquire);
#endif
}

static inline void SharedLock_UnlockShared(sharedlock* pLock) {
	// Decrease shared lock count
	atomic_sub_fence_light(pLock, 1, memory_order_release);
}
