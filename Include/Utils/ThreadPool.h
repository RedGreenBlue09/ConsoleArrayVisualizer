#pragma once

#include <stddef.h>
#include <threads.h>

#include "Utils/Atomic.h"

typedef void thread_pool_job_function(uint8_t iThread, void* Parameter);
typedef atomic size_t thread_pool_wait_group;

// TODO: Wait groups allow waiting for jobs without an array of jobs. Maybe I should do that instead?
typedef struct {
	thread_pool_job_function* pFunction;
	void* Parameter;
	thread_pool_wait_group* pWaitGroup;
	// Internal stuff
	uint8_t iStackEntry;
	atomic bool bJobRead;
} thread_pool_job;

typedef struct {
	uint64_t iEntry : 7;
	// Unless the pop thread stops for years while others are running,
	// we shouldn't get the ABA problem.
	uint64_t Version : 57;
} thread_pool_stack_head;

typedef struct {
	uint8_t iThread;
	uint8_t iNextEntry;
} thread_pool_stack_entry;

typedef struct {
	// Align first element to leave some space for TLS and separate the cachelines
	alignas(sizeof(void*) * 16) thread_pool_stack_entry StackEntry;
	thrd_t Thread;
	thread_pool_job* atomic pJob;
	atomic bool bRun;
	void* TlsBegin;
} thread_pool_worker_thread;

typedef struct {
	uint8_t ThreadCount;
	atomic thread_pool_stack_head StackHead;
	thread_pool_worker_thread aWorkerThread[];
} thread_pool;

thread_pool* ThreadPool_Create(uint8_t ThreadCount);
void ThreadPool_Destroy(thread_pool* pThreadPool);

thread_pool_job ThreadPool_InitJob(
	thread_pool_job_function* pFunction,
	void* Parameter,
	thread_pool_wait_group* pWaitGroup
);
void ThreadPool_AddJob(thread_pool* pThreadPool, thread_pool_job* pJob);
void ThreadPool_AddJobRecursive(thread_pool* pThreadPool, thread_pool_job* pJob, uint8_t iThread);

void ThreadPool_WaitGroup_Init(thread_pool_wait_group* pWaitGroup, size_t ThreadCount);
void ThreadPool_WaitGroup_Increase(thread_pool_wait_group* pWaitGroup, size_t ThreadCount);
void ThreadPool_WaitGroup_Wait(thread_pool_wait_group* pWaitGroup);

size_t ThreadPool_TlsSize();
void* ThreadPool_TlsGet(thread_pool* pThreadPool, uint8_t iThread);
