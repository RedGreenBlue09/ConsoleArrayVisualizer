#pragma once

#include <stdatomic.h>
#include <stdbool.h>

#include "Utils/Machine.h"

typedef _Atomic bool spinlock;

static inline void spinlock_init(spinlock* pLock) {
	*pLock = false;
}

static inline void spinlock_lock(spinlock* pLock) {
#ifdef MACHINE_ARM32 // TODO: Test if this is faster on ARM32
	bool bExpected = true;
	while (!atomic_compare_exchange_weak(pLock, &bExpected, true))
		while (*pLock == true);
#else
	while (atomic_exchange(pLock, true) == true)
		while (*pLock == true);
#endif
}

static inline void spinlock_unlock(spinlock* pLock) {
	*pLock = false;
}
