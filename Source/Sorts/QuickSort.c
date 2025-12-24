
#include "Utils/Common.h"
#include "Visualizer.h"

static void partition(usize iThread, visualizer_array arrayHandle, visualizer_int* array, isize low, isize high) {
	isize left;
	isize right;
	isize pivot;
	visualizer_int pivotValue;

begin:
	left = low;
	right = high;
	pivot = low + (high - low + 1) / 2;
	pivotValue = array[pivot];

	visualizer_marker pointer = Visualizer_CreateMarker(iThread, arrayHandle, pivot, Visualizer_MarkerAttribute_Pointer);
	while (left <= right) {
		Visualizer_UpdateRead(iThread, arrayHandle, left, 1.0f);
		while (array[left] < pivotValue) {
			++left;
			Visualizer_UpdateRead(iThread, arrayHandle, left, 1.0f);

		}
		Visualizer_UpdateRead(iThread, arrayHandle, right, 1.0f);
		while (array[right] > pivotValue) {
			--right;
			Visualizer_UpdateRead(iThread, arrayHandle, right, 1.0f);
		}

		if (left <= right) {
			Visualizer_UpdateSwap(iThread, arrayHandle, left, right, 1.0f);
			swap(&array[left], &array[right]);
			++left;
			--right;
		}
	}
	Visualizer_RemoveMarker(iThread, pointer);

	// Call tail optimization
	// Is slower but prevents O(n) call stack in worst case

	isize smallLeft;
	isize smallRight;
	isize bigLeft;
	isize bigRight;
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

	if (smallLeft < smallRight) partition(iThread, arrayHandle, array, smallLeft, smallRight);
	if (bigLeft < bigRight) {
		low = bigLeft;
		high = bigRight;
		goto begin;
	}
}


typedef struct {
	visualizer_array arrayHandle;
	visualizer_int* array;
	isize low;
	isize high;
	thread_pool_wait_group* waitGroup;
	atomic bool parameterRead;
} partition_parameter;

static void partitionParallel(usize iThread, void* parameter) {
	partition_parameter* partitionParameter = parameter;
	visualizer_array arrayHandle = partitionParameter->arrayHandle;
	visualizer_int* array = partitionParameter->array;
	isize low = partitionParameter->low;
	isize high = partitionParameter->high;
	thread_pool_wait_group* waitGroup = partitionParameter->waitGroup;
	atomic_store_explicit(&partitionParameter->parameterRead, true, memory_order_relaxed);

	isize left;
	isize right;
	isize pivot;
	visualizer_int pivotValue;

begin:
	left = low;
	right = high;
	pivot = low + ((high - low + 1) >> 1);
	pivotValue = array[pivot];

	visualizer_marker pointer = Visualizer_CreateMarker(iThread, arrayHandle, pivot, Visualizer_MarkerAttribute_Pointer);
	while (left <= right) {
		Visualizer_UpdateRead(iThread, arrayHandle, left, 1.0f);
		while (array[left] < pivotValue) {
			++left;
			Visualizer_UpdateRead(iThread, arrayHandle, left, 1.0f);

		}
		Visualizer_UpdateRead(iThread, arrayHandle, right, 1.0f);
		while (array[right] > pivotValue) {
			--right;
			Visualizer_UpdateRead(iThread, arrayHandle, right, 1.0f);
		}

		if (left <= right) {
			Visualizer_UpdateSwap(iThread, arrayHandle, left, right, 1.0f);
			swap(&array[left], &array[right]);
			++left;
			--right;
		}
	}
	Visualizer_RemoveMarker(iThread, pointer);

	// Call tail optimization
	// Is slower but prevents O(n) call stack in worst case

	isize smallLeft;
	isize smallRight;
	isize bigLeft;
	isize bigRight;
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
		partition_parameter partitionParameterNew = { arrayHandle, array, smallLeft, smallRight, waitGroup, false };
		thread_pool_job leftJob = ThreadPool_InitJob(partitionParallel, &partitionParameterNew, waitGroup);
		ThreadPool_AddJobRecursive(Visualizer_pThreadPool, &leftJob, iThread);
		while (!atomic_load_explicit(&partitionParameterNew.parameterRead, memory_order_relaxed));
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

void LeftRightQuickSort(usize iThread, visualizer_array arrayHandle, visualizer_int* array, usize n) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(n, 1.0f, Visualizer_SleepScale_NLogN)
	);

	if (n < 2) return;

	partition(iThread, arrayHandle, array, 0, n - 1);
}

//

void LeftRightQuickSortParallel(usize iThread, visualizer_array arrayHandle, visualizer_int* array, usize n) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(n, 1.0f, Visualizer_SleepScale_NLogN)
	);

	if (n < 2) return;

	thread_pool_wait_group waitGroup;
	ThreadPool_WaitGroup_Init(&waitGroup, 1);
	partition_parameter partitionParameterNew = { arrayHandle, array, 0, n - 1, &waitGroup, false };
	thread_pool_job leftJob = ThreadPool_InitJob(partitionParallel, &partitionParameterNew, &waitGroup);
	ThreadPool_AddJob(Visualizer_pThreadPool, &leftJob);
	ThreadPool_WaitGroup_Wait(&waitGroup);
}
