
#include <assert.h>
#include "Utils/GuardedMalloc.h"
#include "Utils/MemoryPool.h"

// Source: https://arxiv.org/pdf/2210.16471

void PoolInitialize(pool* pPool, pool_index nBlock, uintptr_t BlockSize) {
	if (BlockSize < sizeof(pool_index)) return;
	pPool->nBlock = nBlock;
	pPool->BlockSize = BlockSize;
	pPool->nFreeBlock = nBlock;
	pPool->nInitializedBlock = 0;
	pPool->pMemory = malloc_guarded(nBlock * BlockSize);
	pPool->NextIndex = 0;
}

void PoolDestroy(pool* pPool) {
	assert(pPool);
	assert(pPool->pMemory);
	free(pPool->pMemory);
	pPool->pMemory = NULL; // Prevent undefined behavior
}

pool_index PoolAllocate(pool* pPool) {

	assert(pPool);
	assert(pPool->nBlock > 0);
	assert(pPool->BlockSize > 0);
	assert(pPool->pMemory);

	if (pPool->nInitializedBlock < pPool->nBlock) {

		pool_index* pFreeBlock = PoolIndexToAddress(pPool, pPool->nInitializedBlock);
		*pFreeBlock = pPool->nInitializedBlock + 1;
		++pPool->nInitializedBlock;

	}

	if (pPool->nFreeBlock > 0) {

		pool_index FreeIndex = pPool->NextIndex;
		--pPool->nFreeBlock;

		// The last free block always points to END.
		//if (pPool->nFreeBlock > 0)
			pPool->NextIndex = *(pool_index*)PoolIndexToAddress(pPool, FreeIndex);
		//else
			//pPool->NextIndex = pPool->nBlock; // END

		return FreeIndex;

	} else
		return pPool->nBlock; // END

}

void PoolDeallocate(pool* pPool, pool_index Index)
{
	assert(pPool);
	assert(pPool->nBlock > 0);
	assert(pPool->BlockSize > 0);
	assert(pPool->nFreeBlock < pPool->nBlock);
	assert(pPool->pMemory);
	assert(Index < pPool->nBlock);

	pool_index* pFreeBlock = PoolIndexToAddress(pPool, Index);
	//if (pPool->NextIndex < pPool->nBlock)
	//{
		*pFreeBlock = pPool->NextIndex;
		pPool->NextIndex = Index;
	//}
	//else
	//{
	//	*pFreeBlock = pPool->nBlock; // Last free block points to END.
	//	pPool->NextIndex = Index;
	//}
	++pPool->nFreeBlock;
}
