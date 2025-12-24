#pragma once

#include <threads.h>

#include "Utils/Atomic.h"
#include "Utils/Common.h"

typedef void thread_pool_job_function(usize iThread, void* Parameter);
typedef atomic usize thread_pool_wait_group;

typedef struct {
	thread_pool_job_function* pFunction;
	void* Parameter;
	thread_pool_wait_group* pWaitGroup;
	// Internal stuff
	uint8_t iStackEntry;
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
	atomic bool bRun;
	atomic bool bJobAvailable;
	thread_pool_job Job;
	void* TlsBegin;
} thread_pool_worker_thread;

typedef struct {
	uint8_t ThreadCount;
	uint8_t TlsSlotCount;
	atomic thread_pool_stack_head StackHead;
	thread_pool_worker_thread aWorkerThread[];
} thread_pool;

thread_pool* ThreadPool_Create(usize ThreadCount);
void ThreadPool_Destroy(thread_pool* pThreadPool);

thread_pool_job ThreadPool_InitJob(
	thread_pool_job_function* pFunction,
	void* Parameter,
	thread_pool_wait_group* pWaitGroup
);
void ThreadPool_AddJob(thread_pool* pThreadPool, thread_pool_job* pJob);
void ThreadPool_AddJobRecursive(thread_pool* pThreadPool, thread_pool_job* pJob, usize iThread);

void ThreadPool_WaitGroup_Init(thread_pool_wait_group* pWaitGroup, usize ThreadCount);
void ThreadPool_WaitGroup_Increase(thread_pool_wait_group* pWaitGroup, usize ThreadCount);
void ThreadPool_WaitGroup_Wait(thread_pool_wait_group* pWaitGroup);

// Get the size of each TLS. The TLS is an array of pointers.
usize ThreadPool_TlsSize();
// Get the pointer to the specified slot in the thread's TLS.
void* ThreadPool_TlsGet(thread_pool* pThreadPool, usize iThread, uint8_t Slot);
// Allocate an index to all TLS using arena. Use the index with TlsGet(). 
uint8_t ThreadPool_TlsArenaAlloc(thread_pool* pThreadPool, usize Size);
void ThreadPool_TlsArenaFree(thread_pool* pThreadPool, usize Size);
