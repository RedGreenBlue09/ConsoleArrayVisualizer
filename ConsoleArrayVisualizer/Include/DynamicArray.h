#pragma once

#include <stdint.h>

typedef struct {
	void* array;
	size_t itemSize;
	size_t memSize;
	size_t start;
	size_t size;
} DYNAMIC_ARRAY;

void DaCreate(DYNAMIC_ARRAY* pDa, size_t itemSize);

void DaDelete(DYNAMIC_ARRAY* pDa);

size_t DaSize(DYNAMIC_ARRAY* pDa);

void* DaIndex(DYNAMIC_ARRAY* pDa, size_t index);

void DaResize(DYNAMIC_ARRAY* pDa, size_t newMemSize);

void DaPushEnd(DYNAMIC_ARRAY* pDa, void* pItem);

void DaPushBegin(DYNAMIC_ARRAY* pDa, void* pItem);

void DaPopEnd(DYNAMIC_ARRAY* pDa, void* pItem);

void DaPopBegin(DYNAMIC_ARRAY* pDa, void* pItem);
