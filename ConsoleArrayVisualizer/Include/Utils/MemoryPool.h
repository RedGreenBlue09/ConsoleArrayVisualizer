#pragma once

#include <assert.h>
#include <stdint.h>

typedef uintptr_t pool_index;

typedef struct {
	pool_index nBlock;
	uintptr_t BlockSize;
	pool_index nFreeBlock;
	pool_index nInitializedBlock;
	uint8_t* pMemory;
	pool_index NextIndex;
} pool;

inline void* PoolIndexToAddress(pool* pPool, pool_index Index) {
	return (pPool->pMemory + (Index * pPool->BlockSize));
}
inline pool_index PoolAddressToIndex(pool* pPool, void* pAddress) {
	assert(pAddress >= pPool->pMemory);
	return ((uint8_t*)pAddress - pPool->pMemory) / pPool->BlockSize; // FIXME: UNDERFLOW?
}

void PoolInitialize(pool* pPool, pool_index nBlock, uintptr_t BlockSize);
void PoolDestroy(pool* pPool);
pool_index PoolAllocate(pool* pPool);
void PoolDeallocate(pool* pPool, pool_index Index);
