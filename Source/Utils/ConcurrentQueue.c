/* ----------------------------------------------------------------------------
 *
 * Dual 2-BSD/MIT license. Either or both licenses can be used.
 *
 * ----------------------------------------------------------------------------
 *
 * Copyright (c) 2019 Ruslan Nikolaev.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ----------------------------------------------------------------------------
 *
 * Copyright (c) 2019 Ruslan Nikolaev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------------
 */

#include "Utils/ConcurrentQueue.h"

#include <stdbool.h>
#include <stdatomic.h>

#include "Utils/Machine.h"

#define LFRING_EMPTY SIZE_MAX

typedef uintptr_t lfatomic_t;
typedef intptr_t lfsatomic_t;

#define lfring_cmp(x, op, y)	((lfsatomic_t) ((x) - (y)) op 0)

static inline size_t lfring_map(lfatomic_t idx, size_t n) {
	return (size_t)(idx & (n - 1));
}

#define lfring_threshold3(half, n) ((lfsatomic_t) ((half) + (n) - 1))

static inline size_t lfring_pow2(size_t order) {
	return (size_t)1U << order;
}

static inline void lfring_init_empty(struct lfring* q, size_t order) {
	size_t i, n = lfring_pow2(order + 1);

	for (i = 0; i != n; i++)
		atomic_init(&q->array[i], (lfsatomic_t)-1);

	q->order = order;
	atomic_init(&q->head, 0);
	atomic_init(&q->threshold, -1);
	atomic_init(&q->tail, 0);
}

static inline void lfring_init_full(struct lfring* q, size_t order) {
	size_t i, half = lfring_pow2(order), n = half * 2;

	for (i = 0; i != half; i++)
		atomic_init(&q->array[lfring_map(i, n)], n + lfring_map(i, half));
	for (; i != n; i++)
		atomic_init(&q->array[lfring_map(i, n)], (lfsatomic_t)-1);

	q->order = order;
	atomic_init(&q->head, 0);
	atomic_init(&q->threshold, lfring_threshold3(half, n));
	atomic_init(&q->tail, half);
}

static inline void lfring_init_fill(struct lfring* q, size_t s, size_t e, size_t order) {
	size_t i, half = lfring_pow2(order), n = half * 2;

	for (i = 0; i != s; i++)
		atomic_init(&q->array[lfring_map(i, n)], 2 * n - 1);
	for (; i != e; i++)
		atomic_init(&q->array[lfring_map(i, n)], n + i);
	for (; i != n; i++)
		atomic_init(&q->array[lfring_map(i, n)], (lfsatomic_t)-1);

	q->order = order;
	atomic_init(&q->head, s);
	atomic_init(&q->threshold, lfring_threshold3(half, n));
	atomic_init(&q->tail, e);
}

static inline bool lfring_enqueue(struct lfring* q, size_t eidx, bool nonempty) {
	size_t tidx, half = lfring_pow2(q->order), n = half * 2;
	lfatomic_t tail, entry, ecycle, tcycle;

	eidx ^= (n - 1);

	while (1) {
		tail = atomic_fetch_add_explicit(&q->tail, 1, memory_order_acq_rel);
		tcycle = (tail << 1) | (2 * n - 1);
		tidx = lfring_map(tail, n);
		entry = atomic_load_explicit(&q->array[tidx], memory_order_acquire);
	retry:
		ecycle = entry | (2 * n - 1);
		if (lfring_cmp(ecycle, < , tcycle) && ((entry == ecycle) ||
			((entry == (ecycle ^ n)) &&
				lfring_cmp(atomic_load_explicit(&q->head,
					memory_order_acquire), <= , tail)))) {

			if (!atomic_compare_exchange_weak_explicit(&q->array[tidx],
				&entry, tcycle ^ eidx,
				memory_order_acq_rel, memory_order_acquire))
				goto retry;

			if (!nonempty && (atomic_load(&q->threshold) != lfring_threshold3(half, n)))
				atomic_store(&q->threshold, lfring_threshold3(half, n));
			return true;
		}
	}
}

static inline void lfring_catchup(struct lfring* q, lfatomic_t tail, lfatomic_t head) {
	while (!atomic_compare_exchange_weak_explicit(&q->tail, &tail, head,
		memory_order_acq_rel, memory_order_acquire)) {
		head = atomic_load_explicit(&q->head, memory_order_acquire);
		tail = atomic_load_explicit(&q->tail, memory_order_acquire);
		if (lfring_cmp(tail, >= , head))
			break;
	}
}

static inline size_t lfring_dequeue(struct lfring* q, bool nonempty) {
	size_t hidx, n = lfring_pow2(q->order + 1);
	lfatomic_t head, entry, entry_new, ecycle, hcycle, tail;
	size_t attempt;

	if (!nonempty && atomic_load(&q->threshold) < 0) {
		return LFRING_EMPTY;
	}

	while (1) {
		head = atomic_fetch_add_explicit(&q->head, 1, memory_order_acq_rel);
		hcycle = (head << 1) | (2 * n - 1);
		hidx = lfring_map(head, n);
		attempt = 0;
	again:
		entry = atomic_load_explicit(&q->array[hidx], memory_order_acquire);

		do {
			ecycle = entry | (2 * n - 1);
			if (ecycle == hcycle) {
				atomic_fetch_or_explicit(&q->array[hidx], (n - 1),
					memory_order_acq_rel);
				return (size_t)(entry & (n - 1));
			}

			if ((entry | n) != ecycle) {
				entry_new = entry & ~(lfatomic_t)n;
				if (entry == entry_new)
					break;
			} else {
				if (++attempt <= 10000)
					goto again;
				entry_new = hcycle ^ ((~entry) & n);
			}
		} while (lfring_cmp(ecycle, < , hcycle) &&
			!atomic_compare_exchange_weak_explicit(&q->array[hidx],
				&entry, entry_new,
				memory_order_acq_rel, memory_order_acquire));

		if (!nonempty) {
			tail = atomic_load_explicit(&q->tail, memory_order_acquire);
			if (lfring_cmp(tail, <= , head + 1)) {
				lfring_catchup(q, tail, head + 1);
				atomic_fetch_sub_explicit(&q->threshold, 1,
					memory_order_acq_rel);
				return LFRING_EMPTY;
			}

			if (atomic_fetch_sub_explicit(&q->threshold, 1,
				memory_order_acq_rel) <= 0)
				return LFRING_EMPTY;
		}
	}
}

/* vi: set tabstop=4: */

// wrappers

static bool is_pow2(size_t X) {
	return (X & (X - 1)) == 0;
}

size_t ConcurrentQueue_StructSize(size_t MemberCount) {
	size_t Order = log2_uptr(MemberCount) - is_pow2(MemberCount);
	return sizeof(concurrent_queue) + (sizeof(uintptr_t) << (Order + 1));
}

void ConcurrentQueue_Init(concurrent_queue* pQueue, size_t MemberCount) {
	// Order is log2 half the real count of the queue
	size_t Order = log2_uptr(MemberCount) - is_pow2(MemberCount);
	lfring_init_empty(pQueue, Order);
}

void ConcurrentQueue_Push(concurrent_queue* pQueue, size_t Value) {
	lfring_enqueue(pQueue, Value, true);
}

size_t ConcurrentQueue_Pop(concurrent_queue* pQueue) {
	return lfring_dequeue(pQueue, true);
}

