#pragma once

#include <stdint.h>

#include "Utils/Atomic.h"
#include "Utils/Common.h"

typedef struct lfring {
	usize order;
	atomic usize head;
	atomic isize threshold;
	atomic usize tail;
	atomic usize array[];
} concurrent_queue;

// MemberCount > 0
usize ConcurrentQueue_StructSize(usize MemberCount);
void ConcurrentQueue_Init(concurrent_queue* pQueue, usize MemberCount);
void ConcurrentQueue_Push(concurrent_queue* pQueue, usize Value);
usize ConcurrentQueue_Pop(concurrent_queue* pQueue);

#define ConcurrentQueue_EmptyValue SIZE_MAX
