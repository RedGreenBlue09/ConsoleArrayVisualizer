#pragma once

#include <stdint.h>
#include <stdatomic.h>

#include "Utils/Common.h"

typedef struct lfring {
	size_t order;
	atomic uintptr_t head;
	atomic intptr_t threshold;
	atomic uintptr_t tail;
	atomic uintptr_t array[];
} concurrent_queue;

// MemberCount > 0
size_t ConcurrentQueue_StructSize(size_t MemberCount);
void ConcurrentQueue_Init(concurrent_queue* pQueue, size_t MemberCount);
void ConcurrentQueue_Push(concurrent_queue* pQueue, size_t Value);
size_t ConcurrentQueue_Pop(concurrent_queue* pQueue);

#define ConcurrentQueue_EmptyValue SIZE_MAX
