#pragma once

#include <stdatomic.h>
#include <stdbool.h>

typedef _Atomic bool spinlock;

static inline void spinlock_init(spinlock* pLock) {
	*pLock = false;
}

static inline void spinlock_lock(spinlock* pLock) {
	while (atomic_exchange(pLock, true) == true)
		while (*pLock == true);
}

static inline void spinlock_unlock(spinlock* pLock) {
	*pLock = false;
}
