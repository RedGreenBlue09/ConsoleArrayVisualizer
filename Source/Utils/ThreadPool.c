
#include "Utils/ThreadPool.h"

#include "Utils/GuardedMalloc.h"
#include "Utils/Time.h"

typedef struct {
	thread_pool* pThreadPool;
	size_t iThread;
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

static int WorkerThreadFunction(void* Parameter) {
	worker_thread_parameter* pWorkerParameter = Parameter;
	thread_pool* pThreadPool = pWorkerParameter->pThreadPool;
	size_t iThread = pWorkerParameter->iThread;
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
		atomic_store_fence_light(&pJob->bFinished, true);

		atomic_store_explicit(&pWorkerThread->pJob, NULL, memory_order_relaxed);
		atomic_thread_fence(memory_order_release); // FIXME: Prevent the reorder of the above line
		ConcurrentQueue_Push(pThreadPool->pThreadQueue, iThread);
		Semaphore_ReleaseSingle(&pThreadPool->StatusSemaphore);

		BackoffBegin(&BackoffParameter);
	}

	return 0;
}

thread_pool* ThreadPool_Create(size_t ThreadCount) {
	// Alloc
	// TODO: Separate this so that the user can allocate it on the stack

	thread_pool* pThreadPool;
	size_t QueueStructSize = ConcurrentQueue_StructSize(ThreadCount + (ThreadCount == 0));
	size_t StructSize =
		sizeof(thread_pool) + QueueStructSize +
		sizeof(pThreadPool->aWorkerThread[0]) * ThreadCount;
	
	pThreadPool = malloc_guarded(StructSize);

	// Init

	// Init pointers
	pThreadPool->pThreadQueue = (void*)&pThreadPool->Data[0];
	pThreadPool->aWorkerThread = (void*)&pThreadPool->Data[QueueStructSize];

	// Init values
	pThreadPool->ThreadCount = ThreadCount;
	Semaphore_Init(&pThreadPool->StatusSemaphore, (int8_t)ThreadCount);
	ConcurrentQueue_Init(pThreadPool->pThreadQueue, ThreadCount + (ThreadCount == 0));

	for (size_t i = 0; i < ThreadCount; ++i)
		ConcurrentQueue_Push(pThreadPool->pThreadQueue, i);

	for (size_t i = 0; i < ThreadCount; ++i) {
		atomic_store_explicit(&pThreadPool->aWorkerThread[i].pJob, NULL, memory_order_relaxed);
		atomic_store_explicit(&pThreadPool->aWorkerThread[i].bRun, true, memory_order_relaxed);

		worker_thread_parameter WorkerParameter = { pThreadPool, i, false };
		thrd_create(&pThreadPool->aWorkerThread[i].Thread, WorkerThreadFunction, &WorkerParameter);
		while (!atomic_load_explicit(&WorkerParameter.bParameterReadDone, memory_order_relaxed));
		atomic_thread_fence_light(&WorkerParameter.bParameterReadDone, memory_order_acquire);
	}

	return pThreadPool;
}

void ThreadPool_Destroy(thread_pool* pThreadPool) {
	for (size_t i = 0; i < pThreadPool->ThreadCount; ++i)
		atomic_store_explicit(&pThreadPool->aWorkerThread[i].bRun, false, memory_order_relaxed);
	for (size_t i = 0; i < pThreadPool->ThreadCount; ++i)
		thrd_join(pThreadPool->aWorkerThread[i].Thread, NULL);

	free(pThreadPool);
}

thread_pool_job ThreadPool_InitJob(thrd_start_t pFunction, void* Parameter) {
	thread_pool_job Job;
	Job.pFunction = pFunction;
	Job.Parameter = Parameter;
	return Job;
}

void ThreadPool_AddJob(thread_pool* ThreadPool, thread_pool_job* pJob) {
	backoff_parameter BackoffParameter;
	BackoffInit(&BackoffParameter, 2048, 16);
	BackoffBegin(&BackoffParameter);
	while (!Semaphore_TryAcquireSingle(&ThreadPool->StatusSemaphore)) 
		BackoffSleep(BackoffParameter);

	size_t iThread = ConcurrentQueue_Pop(ThreadPool->pThreadQueue);

	// This relaxed store is safe unless someone run WaitForJob() in another thread.
	atomic_store_explicit(&pJob->bFinished, false, memory_order_relaxed);
	atomic_store_fence_light(&ThreadPool->aWorkerThread[iThread].pJob, pJob);
}

void ThreadPool_AddJobRecursive(thread_pool* ThreadPool, thread_pool_job* pJob) {
	if (Semaphore_TryAcquireSingle(&ThreadPool->StatusSemaphore)) {
		size_t iThread = ConcurrentQueue_Pop(ThreadPool->pThreadQueue);

		atomic_store_explicit(&pJob->bFinished, false, memory_order_relaxed);
		atomic_store_fence_light(&ThreadPool->aWorkerThread[iThread].pJob, pJob);
	} else {
		// Get the job done on the calling thread to prevent dead lock
		// This is less efficient but has low memory and complexity
		pJob->StatusCode = pJob->pFunction(pJob->Parameter);
		atomic_store_fence_light(&pJob->bFinished, true);
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
