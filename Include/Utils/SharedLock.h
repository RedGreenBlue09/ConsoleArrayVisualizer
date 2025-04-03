#pragma once

// Aka. Reader-Writer lock

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "Utils/Common.h"
#include "Utils/Machine.h"

// Last bit for exclusive lock, the rest is shared lock count
// Max number of shared locks: 127
typedef atomic uint8_t sharedlock;

static inline void SharedLock_Init(sharedlock* pLock) {
	atomic_init(pLock, 0);
}

static inline void SharedLock_LockExclusive(sharedlock* pLock) {
	*pLock |= (1 << 7); // Set exclusive lock bit
	while (*pLock > (1 << 7)); // Wait for all shared locks
}

static inline void SharedLock_UnlockExclusive(sharedlock* pLock) {
	*pLock &= ~(1 << 7); // Unset exclusive lock bit
}

static inline void SharedLock_LockShared(sharedlock* pLock) {
#if defined(MACHINE_ARM32) || defined(MACHINE_ARM64) /* TODO: Handle ARMv8.1 */
	// CAS version, is slower on x86 but might be faster on ARM
	uint8_t CachedLock;
	do {
		// Wait for exclusive lock
		do {
			CachedLock = *pLock;
		} while (CachedLock >= (1 << 7));
		// If it's exclusively locked again or increased, retry
		// Else increase shared lock count
	} while (!atomic_compare_exchange_weak(pLock, &CachedLock, CachedLock + 1));
#else
	// FAA version
	while (true) {
		// Wait for exclusive lock
		while (*pLock >= (1 << 7));

		// Increase shared lock count
		if ((*pLock)++ < (1 << 7))
			break;

		// If already exclusively locked, unlock and retry
		--(*pLock);
	};
#endif
}

static inline void SharedLock_UnlockShared(sharedlock* pLock) {
	// Decrease shared lock count
	--*pLock;
}
