#pragma once

#include <threads.h>

#include "Utils/ConcurrentQueue.h"
#include "Utils/Semaphore.h"

typedef struct {
	thrd_start_t pFunction;
	void* Parameter;
	int StatusCode;
	atomic bool bFinished;
} thread_pool_job;

typedef struct {
	thrd_t Thread;
	thread_pool_job* pJob;
	atomic bool bRun;
} thread_pool_worker_thread;

typedef struct {
	size_t ThreadCount;
	semaphore StatusSemaphore;
	concurrent_queue* pThreadQueue;
	thread_pool_worker_thread* aThread;
	uint8_t Data[];
} thread_pool;
