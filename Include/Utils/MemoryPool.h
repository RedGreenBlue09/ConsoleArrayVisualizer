#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

typedef uintptr_t pool_index;

typedef struct {
	pool_index nBlock;
	uintptr_t BlockSize;
	pool_index nFreeBlock;
	pool_index nInitializedBlock;
	uint8_t* pMemory;
	pool_index NextIndex;
} pool;

#define POOL_INVALID_INDEX UINTPTR_MAX

inline void* Pool_IndexToAddress(pool* pPool, pool_index Index) {
	assert(pPool);
	assert(Index < pPool->nBlock);
	return (pPool->pMemory + (Index * pPool->BlockSize));
}

inline pool_index Pool_AddressToIndex(pool* pPool, void* pAddress) {
	assert(pPool);
	assert((uint8_t*)pAddress >= pPool->pMemory && pAddress <= Pool_IndexToAddress(pPool, pPool->nBlock - 1));
	return ((uint8_t*)pAddress - pPool->pMemory) / pPool->BlockSize; // FIXME: UNDERFLOW?
}

void Pool_Initialize(pool* pPool, pool_index nBlock, uintptr_t BlockSize);
void Pool_Destroy(pool* pPool);
pool_index Pool_Allocate(pool* pPool);
void Pool_Deallocate(pool* pPool, pool_index Index);
void Pool_DeallocateAddress(pool* pPool, void* pBlock);
