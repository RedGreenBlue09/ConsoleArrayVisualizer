#pragma once

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

inline void* PoolGetAddress(pool* pPool, pool_index Index) {
	return (pPool->pMemory + (Index * pPool->BlockSize));
}

void PoolInitialize(pool* pPool, pool_index nBlock, uintptr_t BlockSize);
void PoolDestroy(pool* pPool);
pool_index PoolAllocate(pool* pPool);
void PoolDeallocate(pool* pPool, pool_index Index);
