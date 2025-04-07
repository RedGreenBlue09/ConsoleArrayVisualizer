#pragma once

#include <stdbool.h>

#include "Utils/Atomic.h"
#include "Utils/Machine.h"

typedef atomic bool spinlock;

static inline void SpinLock_Init(spinlock* pLock) {
	atomic_init(pLock, false);
}

static inline void SpinLock_Lock(spinlock* pLock) {
#if defined(MACHINE_ARM32) || (defined(MACHINE_ARM64) && !defined(MACHINE_ARM64_ATOMICS))
	bool bExpected = false;
	while (
		!atomic_compare_exchange_weak_explicit(
			pLock,
			&bExpected,
			true,
			memory_order_relaxed,
			memory_order_relaxed
		)
	)
		while (atomic_load_explicit(pLock, memory_order_relaxed) == true);
	// Really hate this but it has to be done to be compatible with C memory model
	// since fences there only sync with other fences, not operations on variables.
	atomic_thread_fence_light(pLock, memory_order_acquire);
#else
	while (atomic_exchange_explicit(pLock, true, memory_order_relaxed) == true)
		while (atomic_load_explicit(pLock, memory_order_relaxed) == true);
	atomic_thread_fence_light(pLock, memory_order_acquire);
#endif
}

static inline void SpinLock_Unlock(spinlock* pLock) {
	atomic_store_fence_light(pLock, false);
}
