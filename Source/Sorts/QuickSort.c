
#include "Utils/Common.h"
#include "Visualizer.h"

static void partition(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t low, intptr_t high) {
	intptr_t left;
	intptr_t right;
	intptr_t pivot;
	visualizer_int pivotValue;

begin:
	left = low;
	right = high;
	pivot = low + (high - low + 1) / 2;
	pivotValue = array[pivot];

	visualizer_marker pointer = Visualizer_CreateMarker(arrayHandle, pivot, Visualizer_MarkerAttribute_Pointer);
	while (left <= right) {
		Visualizer_UpdateRead(arrayHandle, left, 1.0f);
		while (array[left] < pivotValue) {
			++left;
			Visualizer_UpdateRead(arrayHandle, left, 1.0f);

		}
		Visualizer_UpdateRead(arrayHandle, right, 1.0f);
		while (array[right] > pivotValue) {
			--right;
			Visualizer_UpdateRead(arrayHandle, right, 1.0f);
		}

		if (left <= right) {
			Visualizer_UpdateSwap(arrayHandle, left, right, 1.0f);
			swap(&array[left], &array[right]);
			++left;
			--right;
		}
	}
	Visualizer_RemoveMarker(pointer);

	// Call tail optimization
	// Is slower but prevents O(n) call stack in worst case

	intptr_t smallLeft;
	intptr_t smallRight;
	intptr_t bigLeft;
	intptr_t bigRight;
	if ((right - low) > (high - left)) {
		smallLeft = left;
		smallRight = high;
		bigLeft = low;
		bigRight = right;
	} else {
		smallLeft = low;
		smallRight = right;
		bigLeft = left;
		bigRight = high;
	}

	if (smallLeft < smallRight) partition(arrayHandle, array, smallLeft, smallRight);
	if (bigLeft < bigRight) {
		low = bigLeft;
		high = bigRight;
		goto begin;
	}
}


typedef struct {
	visualizer_array_handle arrayHandle;
	visualizer_int* array;
	intptr_t low;
	intptr_t high;
	thread_pool_wait_group* waitGroup;
} partition_parameter;

static void partitionParallel(uint8_t iThread, void* parameter) {
	partition_parameter* partitionParameter = parameter;
	visualizer_array_handle arrayHandle = partitionParameter->arrayHandle;
	visualizer_int* array = partitionParameter->array;
	intptr_t low = partitionParameter->low;
	intptr_t high = partitionParameter->high;
	thread_pool_wait_group* waitGroup = partitionParameter->waitGroup;
	intptr_t left;
	intptr_t right;
	intptr_t pivot;
	visualizer_int pivotValue;

begin:
	left = low;
	right = high;
	pivot = low + (high - low + 1) / 2;
	pivotValue = array[pivot];

	visualizer_marker pointer = Visualizer_CreateMarker(arrayHandle, pivot, Visualizer_MarkerAttribute_Pointer);
	while (left <= right) {
		Visualizer_UpdateReadT(iThread, arrayHandle, left, 1.0f);
		while (array[left] < pivotValue) {
			++left;
			Visualizer_UpdateReadT(iThread, arrayHandle, left, 1.0f);

		}
		Visualizer_UpdateReadT(iThread, arrayHandle, right, 1.0f);
		while (array[right] > pivotValue) {
			--right;
			Visualizer_UpdateReadT(iThread, arrayHandle, right, 1.0f);
		}

		if (left <= right) {
			Visualizer_UpdateSwapT(iThread, arrayHandle, left, right, 1.0f);
			swap(&array[left], &array[right]);
			++left;
			--right;
		}
	}
	Visualizer_RemoveMarker(pointer);

	// Call tail optimization
	// Is slower but prevents O(n) call stack in worst case

	intptr_t smallLeft;
	intptr_t smallRight;
	intptr_t bigLeft;
	intptr_t bigRight;
	if ((right - low) > (high - left)) {
		smallLeft = left;
		smallRight = high;
		bigLeft = low;
		bigRight = right;
	} else {
		smallLeft = low;
		smallRight = right;
		bigLeft = left;
		bigRight = high;
	}

	if (smallLeft < smallRight) {
		ThreadPool_WaitGroup_Increase(waitGroup, 1);
		partition_parameter partitionParameterNew = { arrayHandle, array, smallLeft, smallRight, waitGroup };
		thread_pool_job leftJob = ThreadPool_InitJob(partitionParallel, &partitionParameterNew, waitGroup);
		ThreadPool_AddJobRecursive(Visualizer_pThreadPool, &leftJob, iThread);
	}
	if (bigLeft < bigRight) {
		low = bigLeft;
		high = bigRight;
		goto begin;
	}
}

// Exports

/*
* ALGORITHM INFORMATION:
* Time complexity (worst case) : O(n ^ 2)
* Time complexity (avg. case)  : O(n * log(n))
* Time complexity (best case)  : O(n * log(n))
* Extra space (worst case)     : O(log(n))
* Extra space (best case)      : O(log(n))
* Type of sort                 : Comparative - Exchange
* Negative integer support     : Yes
*/

void LeftRightQuickSort(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(n, 1.0f, Visualizer_SleepScale_NLogN)
	);

	if (n < 2) return;

	partition(arrayHandle, array, 0, n - 1);
}

//

void LeftRightQuickSortParallel(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(n, 1.0f, Visualizer_SleepScale_NLogN)
	);

	if (n < 2) return;

	thread_pool_wait_group waitGroup;
	ThreadPool_WaitGroup_Init(&waitGroup, 1);
	partition_parameter partitionParameterNew = { arrayHandle, array, 0, n - 1, &waitGroup };
	thread_pool_job leftJob = ThreadPool_InitJob(partitionParallel, &partitionParameterNew, &waitGroup);
	ThreadPool_AddJob(Visualizer_pThreadPool, &leftJob);
	ThreadPool_WaitGroup_Wait(&waitGroup);
}
