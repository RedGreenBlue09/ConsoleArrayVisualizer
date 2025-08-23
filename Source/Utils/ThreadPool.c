
#include "Utils/ThreadPool.h"

#include "Utils/GuardedMalloc.h"
#include "Utils/Time.h"

typedef struct {
	thread_pool* pThreadPool;
	uint8_t iThread;
	atomic bool bParameterReadDone;
} worker_thread_parameter;

// Ideally, a stack would be better than a queue as it allows
// threads to sleep more oftenly, but I'm unable to find code for
// a scalable concurrent stack.

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

// Treiber stack
// Not scalable but should give good efficiency (sleep time, latency)
// when contention is low.

static void PushFreeThread(thread_pool* pThreadPool, uint8_t iEntry) {
	thread_pool_stack_entry* pEntry = &pThreadPool->aWorkerThread[iEntry].StackEntry;
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

static uint8_t PopFreeThread(thread_pool* pThreadPool) {
	thread_pool_stack_head OldHead = atomic_load_explicit(&pThreadPool->StackHead, memory_order_acquire);
	thread_pool_stack_head NewHead;
	do {
		if (OldHead.iEntry == pThreadPool->ThreadCount)
			return UINT8_MAX;
		thread_pool_stack_entry* pEntry = &pThreadPool->aWorkerThread[OldHead.iEntry].StackEntry;
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

static int WorkerThreadFunction(void* Parameter) {
	worker_thread_parameter* pWorkerParameter = Parameter;
	thread_pool* pThreadPool = pWorkerParameter->pThreadPool;
	uint8_t iThread = pWorkerParameter->iThread;
	atomic_store_fence_light(&pWorkerParameter->bParameterReadDone, true);
	pWorkerParameter = NULL;


	backoff_parameter BackoffParameter;
	BackoffInit(&BackoffParameter, 2048, 16);

	thread_pool_worker_thread* pWorkerThread = &pThreadPool->aWorkerThread[iThread];
	BackoffBegin(&BackoffParameter);
	while (atomic_load_explicit(&pWorkerThread->bRun, memory_order_relaxed)) {
		thread_pool_job* pJob = atomic_load_explicit(&pWorkerThread->pJob, memory_order_relaxed);
		if (!pJob) {
			BackoffSleep(BackoffParameter);
			continue;
		}
		atomic_thread_fence_light(&pWorkerThread->pJob, memory_order_acquire);
		
		pJob->StatusCode = pJob->pFunction(pJob->Parameter);
		
		uint8_t iStackEntry = pJob->iStackEntry;
		atomic_store_fence_light(&pJob->bFinished, true);
		atomic_store_explicit(&pWorkerThread->pJob, NULL, memory_order_relaxed);
		PushFreeThread(pThreadPool, iStackEntry);

		BackoffBegin(&BackoffParameter);
	}

	return 0;
}

thread_pool* ThreadPool_Create(size_t ThreadCount) {
	// Alloc
	// TODO: Separate this so that the user can allocate it on the stack

	thread_pool* pThreadPool;
	size_t StructSize = sizeof(*pThreadPool) + sizeof(pThreadPool->aWorkerThread[0]) * ThreadCount;
	
	pThreadPool = malloc_guarded(StructSize);

	// Init

	pThreadPool->ThreadCount = (uint8_t)ThreadCount;
	atomic_init(&pThreadPool->StackHead, (thread_pool_stack_head){ 0, 0 });

	for (uint8_t i = 0; i < ThreadCount; ++i) {
		pThreadPool->aWorkerThread[i].StackEntry = (thread_pool_stack_entry){ i, i + 1 };
		atomic_init(&pThreadPool->aWorkerThread[i].pJob, NULL);
		atomic_init(&pThreadPool->aWorkerThread[i].bRun, true);

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

thread_pool_job ThreadPool_InitJob(thrd_start_t pFunction, void* Parameter) {
	thread_pool_job Job;
	Job.pFunction = pFunction;
	Job.Parameter = Parameter;
	return Job;
}

void ThreadPool_AddJob(thread_pool* pThreadPool, thread_pool_job* pJob) {
	backoff_parameter BackoffParameter;
	BackoffInit(&BackoffParameter, 2048, 16);
	BackoffBegin(&BackoffParameter);
	while ((pJob->iStackEntry = PopFreeThread(pThreadPool)) == UINT8_MAX)
		BackoffSleep(BackoffParameter);

	uint8_t iThread = pThreadPool->aWorkerThread[pJob->iStackEntry].StackEntry.iThread;
	// This relaxed store is safe unless someone run WaitForJob() in another thread.
	atomic_store_explicit(&pJob->bFinished, false, memory_order_relaxed);
	atomic_store_fence_light(&pThreadPool->aWorkerThread[iThread].pJob, pJob);
}

void ThreadPool_AddJobRecursive(thread_pool* pThreadPool, thread_pool_job* pJob) {
	pJob->iStackEntry = PopFreeThread(pThreadPool);
	if (pJob->iStackEntry == UINT8_MAX) {
		// Get the job done on the calling thread to prevent dead lock
		// This is less efficient but has low memory and complexity
		pJob->StatusCode = pJob->pFunction(pJob->Parameter);
		atomic_store_fence_light(&pJob->bFinished, true);
	} else {
		uint8_t iThread = pThreadPool->aWorkerThread[pJob->iStackEntry].StackEntry.iThread;
		atomic_store_explicit(&pJob->bFinished, false, memory_order_relaxed);
		atomic_store_fence_light(&pThreadPool->aWorkerThread[iThread].pJob, pJob);
	}
}

void ThreadPool_WaitForJob(thread_pool_job* pJob) {
	backoff_parameter BackoffParameter;
	BackoffInit(&BackoffParameter, 2048, 16);
	BackoffBegin(&BackoffParameter);
	while (!atomic_load_explicit(&pJob->bFinished, memory_order_relaxed))
		BackoffSleep(BackoffParameter);
	atomic_thread_fence_light(&pJob->bFinished, memory_order_acquire);
}
