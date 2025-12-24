
#include "Utils/ThreadPool.h"

#include "Utils/Common.h"
#include "Utils/GuardedMalloc.h"
#include "Utils/Time.h"

// Backoff

typedef struct {
	uint64_t SpinDuration;
	uint64_t YieldDuration;
	uint64_t StartTime;
} backoff_parameter;

static void BackoffInit(
	backoff_parameter* pParameter,
	uint64_t SpinDurationDivisor,
	uint64_t YieldDurationDivisor
) {
	uint64_t Second = clock64_resolution();
	pParameter->SpinDuration = Second / SpinDurationDivisor;
	pParameter->YieldDuration = Second / YieldDurationDivisor;
}

static void BackoffBegin(backoff_parameter* pParameter) {
	pParameter->StartTime = clock64();
}

static void BackoffSleep(backoff_parameter Parameter) {
	uint64_t FreeDuration = clock64() - Parameter.StartTime;
	if (FreeDuration >= Parameter.YieldDuration) // Force sleep for a minimum duration.
		thrd_sleep(&(struct timespec){ .tv_nsec = 1 }, NULL);
	else if (FreeDuration >= Parameter.SpinDuration)
		thrd_yield();
}

// WaitGroup

void ThreadPool_WaitGroup_Init(thread_pool_wait_group* pWaitGroup, usize ThreadCount) {
	atomic_init(pWaitGroup, ThreadCount);
}

void ThreadPool_WaitGroup_Increase(thread_pool_wait_group* pWaitGroup, usize ThreadCount) {
	// This should be used before AddJob(). As a result, AddJob() creates a fence for us.
	atomic_fetch_add_explicit(pWaitGroup, ThreadCount, memory_order_relaxed);
}

static void ThreadPool_WaitGroup_DecreaseSingle(thread_pool_wait_group* pWaitGroup) {
	atomic_sub_fence_light(pWaitGroup, 1, memory_order_release);
}

void ThreadPool_WaitGroup_Wait(thread_pool_wait_group* pWaitGroup) {
	backoff_parameter BackoffParameter;
	BackoffInit(&BackoffParameter, 2048, 16);
	BackoffBegin(&BackoffParameter);
	while (atomic_load_explicit(pWaitGroup, memory_order_relaxed) > 0)
		BackoffSleep(BackoffParameter);
	atomic_thread_fence_light(pWaitGroup, memory_order_acquire);
}

// Treiber stack
// Not scalable but should give good efficiency (sleep time, latency)
// when contention is low. In practice, the thread pool is rarely the bottleneck.

static thread_pool_stack_entry* StackGetEntry(thread_pool* pThreadPool, uint8_t iEntry) {
	return &pThreadPool->aWorkerThread[iEntry].StackEntry;
}

static void StackPushFreeThread(thread_pool* pThreadPool, uint8_t iEntry) {
	thread_pool_stack_entry* pEntry = StackGetEntry(pThreadPool, iEntry);
	thread_pool_stack_head OldHead = atomic_load_explicit(&pThreadPool->StackHead, memory_order_acquire);
	thread_pool_stack_head NewHead;
	do {
		pEntry->iNextEntry = (uint8_t)OldHead.iEntry;
		NewHead = (thread_pool_stack_head){ iEntry, OldHead.Version };
	} while (
		!atomic_compare_exchange_weak_explicit(
			&pThreadPool->StackHead,
			&OldHead,
			NewHead,
			memory_order_acq_rel,
			memory_order_acquire
		)
	);
}

static uint8_t StackPopFreeThread(thread_pool* pThreadPool) {
	thread_pool_stack_head OldHead = atomic_load_explicit(&pThreadPool->StackHead, memory_order_acquire);
	thread_pool_stack_head NewHead;
	do {
		if (OldHead.iEntry == pThreadPool->ThreadCount)
			return UINT8_MAX;
		thread_pool_stack_entry* pEntry = StackGetEntry(pThreadPool, (uint8_t)OldHead.iEntry);
		NewHead = (thread_pool_stack_head){ pEntry->iNextEntry, OldHead.Version + 1 };
	} while (
		!atomic_compare_exchange_weak_explicit(
			&pThreadPool->StackHead,
			&OldHead,
			NewHead,
			memory_order_acq_rel,
			memory_order_acquire
		)
	);
	return (uint8_t)OldHead.iEntry;
}

//

typedef struct {
	thread_pool* pThreadPool;
	uint8_t iThread;
	atomic bool bParameterReadDone;
} worker_thread_parameter;

static int WorkerThreadFunction(void* Parameter) {
	worker_thread_parameter* pWorkerParameter = Parameter;
	thread_pool* pThreadPool = pWorkerParameter->pThreadPool;
	uint8_t iThread = pWorkerParameter->iThread;
	atomic_store_fence_light(&pWorkerParameter->bParameterReadDone, true);
	pWorkerParameter = NULL;

	backoff_parameter BackoffParameter;
	BackoffInit(&BackoffParameter, 2048, 16);

	thread_pool_worker_thread* pWorkerThread = &pThreadPool->aWorkerThread[iThread];
	thread_pool_job* pJob = &pWorkerThread->Job;

	BackoffBegin(&BackoffParameter);
	while (atomic_load_explicit(&pWorkerThread->bRun, memory_order_relaxed)) {
		if (!atomic_load_explicit(&pWorkerThread->bJobAvailable, memory_order_relaxed)) {
			BackoffSleep(BackoffParameter);
			continue;
		}
		atomic_thread_fence_light(&pWorkerThread->bJobAvailable, memory_order_acquire);

		pJob->pFunction(iThread, pJob->Parameter);
		ThreadPool_WaitGroup_DecreaseSingle(pJob->pWaitGroup);

		atomic_store_explicit(&pWorkerThread->bJobAvailable, false, memory_order_relaxed);
		StackPushFreeThread(pThreadPool, pJob->iStackEntry);
		BackoffBegin(&BackoffParameter);
	}

	return 0;
}

// Create & delete

thread_pool* ThreadPool_Create(usize ThreadCount) {
	// Alloc
	// TODO: Separate this so that the user can allocate it on the stack

	thread_pool* pThreadPool;
	usize StructSize = sizeof(*pThreadPool) + sizeof(pThreadPool->aWorkerThread[0]) * ThreadCount;

	pThreadPool = malloc_guarded(StructSize);

	// Init

	pThreadPool->ThreadCount = (uint8_t)ThreadCount;
	pThreadPool->TlsSlotCount = 0;
	atomic_init(&pThreadPool->StackHead, (thread_pool_stack_head){ 0, 0 });

	for (uint8_t i = 0; i < ThreadCount; ++i) {
		pThreadPool->aWorkerThread[i].StackEntry = (thread_pool_stack_entry){ i, i + 1 };
		atomic_init(&pThreadPool->aWorkerThread[i].bRun, true);
		atomic_init(&pThreadPool->aWorkerThread[i].bJobAvailable, false);

		worker_thread_parameter WorkerParameter = { pThreadPool, i, false };
		thrd_create(&pThreadPool->aWorkerThread[i].Thread, WorkerThreadFunction, &WorkerParameter);
		while (!atomic_load_explicit(&WorkerParameter.bParameterReadDone, memory_order_relaxed));
		atomic_thread_fence_light(&WorkerParameter.bParameterReadDone, memory_order_acquire);
	}

	return pThreadPool;
}

void ThreadPool_Destroy(thread_pool* pThreadPool) {
	for (uint8_t i = 0; i < pThreadPool->ThreadCount; ++i)
		atomic_store_explicit(&pThreadPool->aWorkerThread[i].bRun, false, memory_order_relaxed);
	for (uint8_t i = 0; i < pThreadPool->ThreadCount; ++i)
		thrd_join(pThreadPool->aWorkerThread[i].Thread, NULL);

	free(pThreadPool);
}

// Job

thread_pool_job ThreadPool_InitJob(
	thread_pool_job_function* pFunction,
	void* Parameter,
	thread_pool_wait_group* pWaitGroup
) {
	thread_pool_job Job;
	Job.pFunction = pFunction;
	Job.Parameter = Parameter;
	Job.pWaitGroup = pWaitGroup;
	return Job;
}

void ThreadPool_AddJob(thread_pool* pThreadPool, thread_pool_job* pJob) {
	backoff_parameter BackoffParameter;
	BackoffInit(&BackoffParameter, 2048, 16);
	BackoffBegin(&BackoffParameter);
	while ((pJob->iStackEntry = StackPopFreeThread(pThreadPool)) == UINT8_MAX)
		BackoffSleep(BackoffParameter);

	uint8_t iThread = StackGetEntry(pThreadPool, pJob->iStackEntry)->iThread;
	pThreadPool->aWorkerThread[iThread].Job = *pJob;
	atomic_store_fence_light(&pThreadPool->aWorkerThread[iThread].bJobAvailable, true);
}

void ThreadPool_AddJobRecursive(thread_pool* pThreadPool, thread_pool_job* pJob, usize iCurrentThread) {
	pJob->iStackEntry = StackPopFreeThread(pThreadPool);
	if (pJob->iStackEntry == UINT8_MAX) {
		// Get the job done on the calling thread to prevent dead lock
		// This is less efficient but has low memory and complexity
		pJob->pFunction(iCurrentThread, pJob->Parameter);
		ThreadPool_WaitGroup_DecreaseSingle(pJob->pWaitGroup);
	} else {
		uint8_t iThread = StackGetEntry(pThreadPool, pJob->iStackEntry)->iThread;
		pThreadPool->aWorkerThread[iThread].Job = *pJob;
		atomic_store_fence_light(&pThreadPool->aWorkerThread[iThread].bJobAvailable, true);
	}
}

// TLS

usize ThreadPool_TlsSize() {
	return sizeof(thread_pool_worker_thread) - offsetof(thread_pool_worker_thread, TlsBegin);
}

void* ThreadPool_TlsGet(thread_pool* pThreadPool, usize iThread, uint8_t Slot) {
	return (void*)&pThreadPool->aWorkerThread[iThread].TlsBegin;
}

uint8_t ThreadPool_TlsArenaAlloc(thread_pool* pThreadPool, usize Size) {
	assert(Size <= ThreadPool_TlsSize() - (pThreadPool->TlsSlotCount * sizeof(void*)));
	uint8_t Result = pThreadPool->TlsSlotCount;
	pThreadPool->TlsSlotCount += (uint8_t)(Size / sizeof(void*) + (Size % sizeof(void*) > 0));
	return Result;
}

void ThreadPool_TlsArenaFree(thread_pool* pThreadPool, usize Size) {
	assert(Size <= pThreadPool->TlsSlotCount * sizeof(void*));
	pThreadPool->TlsSlotCount -= (uint8_t)(Size / sizeof(void*) + (Size % sizeof(void*) > 0));
}
