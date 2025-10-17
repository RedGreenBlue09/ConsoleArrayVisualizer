
#include "Utils/Common.h"
#include "Utils/ThreadPool.h"
#include "Visualizer.h"

static void step(uint8_t iThread, visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t x, intptr_t y) {
	Visualizer_UpdateRead2T(iThread, arrayHandle, x, y, 1.0f);
	if (array[x] > array[y]) {
		swap(&array[x], &array[y]);
		Visualizer_UpdateSwapT(iThread, arrayHandle, x, y, 1.0f);
	}
}

typedef struct {
	visualizer_array_handle arrayHandle;
	visualizer_int* array;
	intptr_t start;
	intptr_t stop;
	intptr_t gap;
} circle_parameter;

static void circle(uint8_t iThread, void* parameter) {
	circle_parameter* circleParameter = parameter;
	visualizer_array_handle arrayHandle = circleParameter->arrayHandle;
	visualizer_int* array = circleParameter->array;
	intptr_t start = circleParameter->start;
	intptr_t stop = circleParameter->stop;
	intptr_t gap = circleParameter->gap;

	if ((stop - start) / gap >= 1) {
		intptr_t left = start, right = stop;
		while (left < right) {
			step(iThread, arrayHandle, array, left, right);
			left += gap;
			right -= gap;
		}

		thread_pool_wait_group waitGroup;
		ThreadPool_WaitGroup_Init(&waitGroup, 2);
		circle_parameter leftParameter = { arrayHandle, array, start, right, gap };
		circle_parameter rightParameter = { arrayHandle, array, left, stop, gap };
		thread_pool_job leftJob = ThreadPool_InitJob(circle, &leftParameter, &waitGroup);
		thread_pool_job rightJob = ThreadPool_InitJob(circle, &rightParameter, &waitGroup);

		ThreadPool_AddJobRecursive(Visualizer_pThreadPool, &leftJob, iThread);
		ThreadPool_AddJobRecursive(Visualizer_pThreadPool, &rightJob, iThread);

		ThreadPool_WaitGroup_Wait(&waitGroup);
	}

	return;
}

typedef struct {
	visualizer_array_handle arrayHandle;
	visualizer_int* array;
	intptr_t n;
	intptr_t start;
	intptr_t gap;
} sort_main_parameter;

static void sortMain(uint8_t iThread, void* parameter) {
	sort_main_parameter* sortParameter = parameter;
	visualizer_array_handle arrayHandle = sortParameter->arrayHandle;
	visualizer_int* array = sortParameter->array;
	intptr_t n = sortParameter->n;
	intptr_t start = sortParameter->start;
	intptr_t gap = sortParameter->gap;

	if (gap < n) {
		thread_pool_wait_group waitGroup;
		ThreadPool_WaitGroup_Init(&waitGroup, 2);
		sort_main_parameter leftParameter = { arrayHandle, array, n, start, gap * 2 };
		sort_main_parameter rightParameter = { arrayHandle, array, n, start + gap, gap * 2 };
		thread_pool_job leftJob = ThreadPool_InitJob(sortMain, &leftParameter, &waitGroup);
		thread_pool_job rightJob = ThreadPool_InitJob(sortMain, &rightParameter, &waitGroup);
		
		ThreadPool_AddJobRecursive(Visualizer_pThreadPool, &leftJob, iThread);
		ThreadPool_AddJobRecursive(Visualizer_pThreadPool, &rightJob, iThread);

		ThreadPool_WaitGroup_Wait(&waitGroup);

		circle_parameter circleParameter = { arrayHandle, array, start, n - gap + start, gap };
		circle(iThread, &circleParameter);
	}

	return;
}

void WeaveSortParallel(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(n, 2.0f, Visualizer_SleepScale_NLogNLogN)
	);
	thread_pool_wait_group waitGroup;
	ThreadPool_WaitGroup_Init(&waitGroup, 1);
	sort_main_parameter parameter = { arrayHandle, array, n, 0, 1 };
	thread_pool_job job = ThreadPool_InitJob(sortMain, &parameter, &waitGroup);
	ThreadPool_AddJob(Visualizer_pThreadPool, &job);
	ThreadPool_WaitGroup_Wait(&waitGroup);
}
