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
	thread_pool_job* atomic pJob;
	atomic bool bRun;
} thread_pool_worker_thread;

typedef struct {
	size_t ThreadCount;
	semaphore StatusSemaphore;
	concurrent_queue* pThreadQueue;
	thread_pool_worker_thread* aThread;
	uint8_t Data[];
} thread_pool;

thread_pool* ThreadPool_Create(size_t ThreadCount);
void ThreadPool_Destroy(thread_pool* pThreadPool);

thread_pool_job ThreadPool_InitJob(thrd_start_t pFunction, void* Parameter);
void ThreadPool_AddJob(thread_pool* ThreadPool, thread_pool_job* pJob);
void ThreadPool_WaitForJob(thread_pool_job* pJob);
