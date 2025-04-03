#pragma once

#include <stdatomic.h>
#include <stdbool.h>

#include "Utils/Common.h"
#include "Utils/Machine.h"

typedef atomic bool spinlock;

static inline void SpinLock_Init(spinlock* pLock) {
	atomic_init(pLock, false);
}

static inline void SpinLock_Lock(spinlock* pLock) {
#ifdef MACHINE_ARM32 // TODO: Test if this is faster on ARM32
	bool bExpected = true;
	while (
		!atomic_compare_exchange_weak_explicit(
			pLock,
			&bExpected,
			true,
			memory_order_acquire,
			memory_order_relaxed
		)
	)
		while (atomic_load_explicit(pLock, memory_order_relaxed) == true);
#else
	while (atomic_exchange_explicit(pLock, true, memory_order_relaxed) == true)
		while (atomic_load_explicit(pLock, memory_order_relaxed) == true);
	atomic_load_explicit(pLock, memory_order_acquire);
#endif
}

static inline void SpinLock_Unlock(spinlock* pLock) {
	atomic_store_explicit(pLock, false, memory_order_release);
}
