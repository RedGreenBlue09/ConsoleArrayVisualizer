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
	// CAS version, is slower on x86 but might be faster on ARM
	/*
	uint8_t CachedLock;
	do {
		// Wait for exclusive lock
		do {
			CachedLock = *pLock;
		} while (CachedLock >= (1 << 7));
		// If it's exclusively locked again or increased, retry
		// Else increase shared lock count
	} while (!atomic_compare_exchange_weak(pLock, &CachedLock, CachedLock + 1));
	*/

	// FAA version
	while (true) {
		// Wait for exclusive lock
		while (*pLock >= (1 << 7));

		// Increase shared lock count
		++*pLock;

		// If exclusively locked again, unlock and retry
		if (*pLock >= (1 << 7))
			--*pLock;
		else
			break;
	};
}

static inline void sharedlock_unlock_shared(sharedlock* pLock) {
	// Decrease shared lock count
	--*pLock;
}
