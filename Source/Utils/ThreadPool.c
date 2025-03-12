
#include "Utils/ThreadPool.h"

#include "Utils/GuardedMalloc.h"

typedef struct {
	thread_pool* pThreadPool;
	size_t iThread;
	atomic bool bParameterReadDone;
} worker_thread_parameter;

static int WorkerThread(void* Parameter) {
	worker_thread_parameter* pWorkerParameter = Parameter;
	thread_pool* pThreadPool = pWorkerParameter->pThreadPool;
	size_t iThread = pWorkerParameter->iThread;
	pWorkerParameter->bParameterReadDone = true;
	pWorkerParameter = NULL;

	thread_pool_worker_thread* pWorkerThread = &pThreadPool->aThread[iThread];

	while (pWorkerThread->bRun) {
		if (!pWorkerThread->pJob) // TODO: Sleep
			continue;
		thread_pool_job* pJob = pWorkerThread->pJob;

		pJob->StatusCode = pJob->pFunction(pJob->Parameter);
		pJob->bFinished = true;

		pWorkerThread->pJob = NULL;
		ConcurrentQueue_Push(pThreadPool->pThreadQueue, iThread);
		semaphore_release_single(&pThreadPool->StatusSemaphore);
	}

	return 0;
}

thread_pool* ThreadPool_Create(size_t ThreadCount) {
	// Alloc
	// TODO: Separate this so that the user can allocate it on the stack

	thread_pool* pThreadPool;
	size_t QueueStructSize = ConcurrentQueue_StructSize(ThreadCount);
	size_t StructSize =
		sizeof(thread_pool) + QueueStructSize +
		sizeof(pThreadPool->aThread[0]) * ThreadCount;
	
	pThreadPool = malloc_guarded(StructSize);

	// Init

	// Init pointers
	pThreadPool->pThreadQueue = (void*)&pThreadPool->Data[0];
	pThreadPool->aThread = (void*)&pThreadPool->Data[QueueStructSize];

	// Init values
	pThreadPool->ThreadCount = ThreadCount;
	semaphore_init(&pThreadPool->StatusSemaphore, (int8_t)ThreadCount);
	ConcurrentQueue_Init(pThreadPool->pThreadQueue, ThreadCount);

	for (size_t i = 0; i < ThreadCount; ++i)
		ConcurrentQueue_Push(pThreadPool->pThreadQueue, i);

	for (size_t i = 0; i < ThreadCount; ++i) {
		pThreadPool->aThread[i].pJob = NULL;
		pThreadPool->aThread[i].bRun = true;

		worker_thread_parameter WorkerParameter = { pThreadPool, i, false };
		thrd_create(&pThreadPool->aThread[i].Thread, WorkerThread, &WorkerParameter);
		while (!WorkerParameter.bParameterReadDone);
	}

	return pThreadPool;
}

void ThreadPool_Destroy(thread_pool* pThreadPool) {
	for (size_t i = 0; i < pThreadPool->ThreadCount; ++i)
		pThreadPool->aThread[i].bRun = false;
	for (size_t i = 0; i < pThreadPool->ThreadCount; ++i)
		thrd_join(pThreadPool->aThread[i].Thread, NULL);

	free(pThreadPool);
}

thread_pool_job ThreadPool_InitJob(thrd_start_t pFunction, void* Parameter) {
	thread_pool_job Job;
	Job.pFunction = pFunction;
	Job.Parameter = Parameter;
	return Job;
}

void ThreadPool_AddJob(thread_pool* ThreadPool, thread_pool_job* pJob) {
	if (semaphore_try_acquire_single(&ThreadPool->StatusSemaphore)) {
		size_t iThread = ConcurrentQueue_Pop(ThreadPool->pThreadQueue);

		pJob->bFinished = false;
		ThreadPool->aThread[iThread].pJob = pJob;
	} else {
		// Get the job done to prevent dead lock
		// This is less efficient but saves memory
		// TODO: Find another way to address this
		pJob->StatusCode = pJob->pFunction(pJob->Parameter);
		pJob->bFinished = true;
	}

}

void ThreadPool_WaitForJob(thread_pool_job* pJob) {
	while (!pJob->bFinished); // TODO: Sleep
}
