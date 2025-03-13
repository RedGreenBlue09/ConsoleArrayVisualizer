
#include "Utils/Common.h"
#include "Utils/ThreadPool.h"
#include "Visualizer.h"

static void step(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t x, intptr_t y) {
	Visualizer_UpdateRead2(arrayHandle, x, y, 0.5);
	if (array[x] > array[y]) {
		swap(&array[x], &array[y]);
		Visualizer_UpdateSwap(arrayHandle, x, y, 0.5);
	}
}

typedef struct {
	visualizer_array_handle arrayHandle;
	visualizer_int* array;
	intptr_t start;
	intptr_t stop;
	intptr_t gap;
	thread_pool* threadPool;
} circle_parameter;

static int circle(void* parameter) {
	circle_parameter* circleParameter = parameter;
	visualizer_array_handle arrayHandle = circleParameter->arrayHandle;
	visualizer_int* array = circleParameter->array;
	intptr_t start = circleParameter->start;
	intptr_t stop = circleParameter->stop;
	intptr_t gap = circleParameter->gap;
	thread_pool* threadPool = circleParameter->threadPool;

	if ((stop - start) / gap >= 1) {
		intptr_t left = start, right = stop;
		while (left < right) {
			step(arrayHandle, array, left, right);
			left += gap;
			right -= gap;
		}

		circle_parameter leftParameter = { arrayHandle, array, start, right, gap, threadPool };
		circle_parameter rightParameter = { arrayHandle, array, left, stop, gap, threadPool };
		thread_pool_job leftJob = ThreadPool_InitJob(circle, &leftParameter);
		thread_pool_job rightJob = ThreadPool_InitJob(circle, &rightParameter);

		ThreadPool_AddJobRecursive(threadPool, &leftJob);
		ThreadPool_AddJobRecursive(threadPool, &rightJob);

		ThreadPool_WaitForJob(&leftJob);
		ThreadPool_WaitForJob(&rightJob);
	}

	return 0;
}

typedef struct {
	visualizer_array_handle arrayHandle;
	visualizer_int* array;
	intptr_t n;
	intptr_t start;
	intptr_t gap;
	thread_pool* threadPool;
} sort_main_parameter;

static int sortMain(void* parameter) {
	sort_main_parameter* sortParameter = parameter;
	visualizer_array_handle arrayHandle = sortParameter->arrayHandle;
	visualizer_int* array = sortParameter->array;
	intptr_t n = sortParameter->n;
	intptr_t start = sortParameter->start;
	intptr_t gap = sortParameter->gap;
	thread_pool* threadPool = sortParameter->threadPool;

	if (gap < n) {
		sort_main_parameter leftParameter = { arrayHandle, array, n, start, gap * 2, threadPool };
		sort_main_parameter rightParameter = { arrayHandle, array, n, start + gap, gap * 2, threadPool };
		thread_pool_job leftJob = ThreadPool_InitJob(sortMain, &leftParameter);
		thread_pool_job rightJob = ThreadPool_InitJob(sortMain, &rightParameter);

		ThreadPool_AddJobRecursive(threadPool, &leftJob);
		ThreadPool_AddJobRecursive(threadPool, &rightJob);

		ThreadPool_WaitForJob(&leftJob);
		ThreadPool_WaitForJob(&rightJob);

		circle_parameter circleParameter = { arrayHandle, array, start, n - gap + start, gap, threadPool };
		circle(&circleParameter);
	}

	return 0;
}

void WeaveSortParallel(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(n, 1.0, Visualizer_SleepScale_N) // TODO FIXME
	);
	thread_pool* threadPool = ThreadPool_Create(4); // TODO FIXME
	sort_main_parameter parameter = { arrayHandle, array, n, 0, 1, threadPool };
	sortMain(&parameter);
	ThreadPool_Destroy(threadPool);
}
