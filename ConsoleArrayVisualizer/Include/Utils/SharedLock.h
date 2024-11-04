#pragma once

// Aka. Reader-Writer lock

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

// Last bit for exclusive lock, the rest is shared lock count
// Max number of shared locks: 128
typedef _Atomic uint8_t sharedlock;

static inline void sharedlock_init(sharedlock* pLock) {
	pLock = 0;
}

static inline void sharedlock_lock_exclusive(sharedlock* pLock) {
	*pLock |= (1 << 7); // Set exclusive lock bit
	while (*pLock > (1 << 7)); // Wait for all shared locks
}

static inline void sharedlock_unlock_exclusive(sharedlock* pLock) {
	*pLock &= ~(1 << 7); // Unset exclusive lock bit
}

static inline void sharedlock_lock_shared(sharedlock* pLock) {
	uint8_t CachedLock;
	do {
		// Wait for exclusive lock
		do {
			CachedLock = *pLock;
		} while (CachedLock >= (1 << 7));
		// If it's exclusively locked again or increased, retry
		// Else increase shared lock count
	} while (!atomic_compare_exchange_weak(pLock, &CachedLock, CachedLock + 1));
}

static inline void sharedlock_unlock_shared(sharedlock* pLock) {
	--*pLock;
}
