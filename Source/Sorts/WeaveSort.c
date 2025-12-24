
#include "Utils/Common.h"
#include "Utils/ThreadPool.h"
#include "Visualizer.h"

static void step(usize iThread, visualizer_array arrayHandle, visualizer_int* array, usize x, usize y) {
	Visualizer_UpdateRead2(iThread, arrayHandle, x, y, 1.0f);
	if (array[x] > array[y]) {
		swap(&array[x], &array[y]);
		Visualizer_UpdateSwap(iThread, arrayHandle, x, y, 1.0f);
	}
}

typedef struct {
	visualizer_array arrayHandle;
	visualizer_int* array;
	usize start;
	usize stop;
	usize gap;
} circle_parameter;

static void circle(usize iThread, void* parameter) {
	circle_parameter* circleParameter = parameter;
	visualizer_array arrayHandle = circleParameter->arrayHandle;
	visualizer_int* array = circleParameter->array;
	usize start = circleParameter->start;
	usize stop = circleParameter->stop;
	usize gap = circleParameter->gap;

	if ((stop - start) / gap >= 1) {
		isize left = start, right = stop;
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
	visualizer_array arrayHandle;
	visualizer_int* array;
	usize n;
	usize start;
	usize gap;
} sort_main_parameter;

static void sortMain(usize iThread, void* parameter) {
	sort_main_parameter* sortParameter = parameter;
	visualizer_array arrayHandle = sortParameter->arrayHandle;
	visualizer_int* array = sortParameter->array;
	usize n = sortParameter->n;
	usize start = sortParameter->start;
	usize gap = sortParameter->gap;

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

void WeaveSortParallel(usize iThread, visualizer_array arrayHandle, visualizer_int* array, usize n) {
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
