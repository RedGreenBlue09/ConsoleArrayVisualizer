
#include "Utils/GuardedMalloc.h"
#include "Utils/MemoryPool.h"

// Source: https://arxiv.org/pdf/2210.16471

void Pool_Initialize(pool* pPool, pool_index nBlock, uintptr_t BlockSize) {
	if (BlockSize < sizeof(pool_index)) return;
	pPool->nBlock = nBlock;
	pPool->BlockSize = BlockSize;
	pPool->nFreeBlock = nBlock;
	pPool->nInitializedBlock = 0;
	pPool->pMemory = malloc_guarded(nBlock * BlockSize);
	pPool->NextIndex = 0;
}

void Pool_Destroy(pool* pPool) {
	assert(pPool);
	assert(pPool->pMemory);
	free(pPool->pMemory);
	pPool->pMemory = NULL; // Prevent undefined behavior
}

pool_index Pool_Allocate(pool* pPool) {

	assert(pPool);
	assert(pPool->nBlock > 0);
	assert(pPool->BlockSize > 0);
	assert(pPool->pMemory);

	if (pPool->nInitializedBlock < pPool->nBlock) {

		pool_index* pFreeBlock = Pool_IndexToAddress(pPool, pPool->nInitializedBlock);
		*pFreeBlock = pPool->nInitializedBlock + 1;
		++pPool->nInitializedBlock;

	}

	if (pPool->nFreeBlock > 0) {

		pool_index FreeIndex = pPool->NextIndex;
		--pPool->nFreeBlock;
		pPool->NextIndex = *(pool_index*)Pool_IndexToAddress(pPool, FreeIndex);
		return FreeIndex;

	} else
		return POOL_INVALID_INDEX;

}

void Pool_Deallocate(pool* pPool, pool_index Index)
{
	assert(pPool);
	assert(pPool->nBlock > 0);
	assert(pPool->BlockSize > 0);
	assert(pPool->nFreeBlock < pPool->nBlock);
	assert(pPool->pMemory);
	assert(Index < pPool->nBlock);

	pool_index* pFreeBlock = Pool_IndexToAddress(pPool, Index);
	*pFreeBlock = pPool->NextIndex;
	pPool->NextIndex = Index;
	++pPool->nFreeBlock;
}

void Pool_DeallocateAddress(pool* pPool, void* pBlock)
{
	assert(pPool);
	assert(pPool->nBlock > 0);
	assert(pPool->BlockSize > 0);
	assert(pPool->nFreeBlock < pPool->nBlock);
	assert(pPool->pMemory);
	assert((uint8_t*)pBlock >= pPool->pMemory && pBlock <= Pool_IndexToAddress(pPool, pPool->nBlock - 1));

	*(pool_index*)pBlock = pPool->NextIndex;
	pPool->NextIndex = Pool_AddressToIndex(pPool, pBlock);
	++pPool->nFreeBlock;
}
