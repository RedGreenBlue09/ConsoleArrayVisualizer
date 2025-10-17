
#include "Visualizer.h"
#include "Utils/Common.h"
#include "Utils/GuardedMalloc.h"
#include "Utils/Machine.h"
#include "Utils/ThreadPool.h"

/*
* ALGORITHM INFORMATION:
* Time complexity          : O(n * log(n))
* Extra space              : No
* Type of sort             : Comparative - Insert
* Negative integer support : Yes
*/

// TODO: 32-bit intptr_t support

static const intptr_t gapsTokuda[] = {
	1, 4, 9, 20, 46, 103, 233, 525, 1182, 2660, 5985, 13467, 30301,
	68178, 153401, 345152, 776591, 1747331, 3931496, 8845866, 19903198,
	44782196, 100759940, 226709866, 510097200, 1147718700,
#ifdef MACHINE_PTR64
	2582367076, 5810325920, 13073233321, 29414774973, 66183243690, 148912298303,
	335052671183, 753868510162, 1696204147864, 3816459332694, 8587033498562,
	19320825371765, 43471857086472, 97811678444563, 220076276500268,
	495171622125603, 1114136149782608, 2506806337010869, 5640314258274455,
	12690707081117525, 28554090932514431, 64246704598157469, 144555085345854306,
	325248942028172190, 731810119563387427, 1646572769017621711, 3704788730289648850,
	8335774643151709914
#endif
};

static void shellSort(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n, const intptr_t* gaps, intptr_t nGaps) {

	if (n < 2) return;

	intptr_t pass = nGaps - 1;
	while (gaps[pass] > n) --pass;

	for (--pass; pass >= 0; --pass) {

		intptr_t gap = gaps[pass];

		visualizer_marker pointer = Visualizer_CreateMarker(arrayHandle, gap, Visualizer_MarkerAttribute_Pointer);
		for (intptr_t i = gap; i < n; ++i) {
			visualizer_int temp = array[i];
			Visualizer_UpdateRead(arrayHandle, i, 1.0f);
			Visualizer_MoveMarker(&pointer, i);

			intptr_t j;
			for (j = i; j >= gap; j -= gap) {
				Visualizer_UpdateRead(arrayHandle, j - gap, 1.0f);
				if (array[j - gap] <= temp)
					break;
				Visualizer_UpdateWrite(arrayHandle, j, array[j - gap], 1.0f);
				array[j] = array[j - gap];
			}
			Visualizer_UpdateWrite(arrayHandle, j, temp, 1.0f);
			array[j] = temp;
		}
		Visualizer_RemoveMarker(pointer);
	}

	return;

}

void ShellSortTokuda(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(n, 0.25f, Visualizer_SleepScale_NLogN)
	);
	shellSort(arrayHandle, array, n, gapsTokuda, static_arrlen(gapsTokuda));
	return;
}

// Parallel

typedef struct {
	visualizer_array_handle arrayHandle;
	visualizer_int* array;
	intptr_t start;
	intptr_t end;
	intptr_t gap;
	atomic bool parameterRead;
} insertion_parameter;

static void gappedInsertion(uint8_t iThread, void* parameter) {
	insertion_parameter* insertionParameter = parameter;
	visualizer_array_handle arrayHandle = insertionParameter->arrayHandle;
	visualizer_int* array = insertionParameter->array;
	intptr_t start = insertionParameter->start;
	intptr_t end = insertionParameter->end;
	intptr_t gap = insertionParameter->gap;
	atomic_store_fence_light(&insertionParameter->parameterRead, true);
	insertionParameter = NULL;

	visualizer_marker pointer = Visualizer_CreateMarker(arrayHandle, start + gap, Visualizer_MarkerAttribute_Pointer);
	for (intptr_t i = start + gap; i < end; i += gap) {
		visualizer_int temp = array[i];
		Visualizer_UpdateReadT(iThread, arrayHandle, i, 1.0f);
		Visualizer_MoveMarker(&pointer, i);

		intptr_t j;
		visualizer_int temp2;
		for (j = i; j >= start + gap; j -= gap) {
			Visualizer_UpdateReadT(iThread, arrayHandle, j - gap, 1.0f);
			temp2 = array[j - gap];
			if (temp2 <= temp)
				break;
			Visualizer_UpdateWriteT(iThread, arrayHandle, j, temp2, 1.0f);
			array[j] = temp2;
		}
		Visualizer_UpdateWriteT(iThread, arrayHandle, j, temp, 1.0f);
		array[j] = temp;
	}
	Visualizer_RemoveMarker(pointer);

	return;
}
void ShellSortParallel(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(n, 0.25f, Visualizer_SleepScale_N) // TODO FIXME
	);

	intptr_t nGaps = static_arrlen(gapsTokuda);
	const intptr_t* gaps = gapsTokuda;

	intptr_t pass = nGaps - 1;
	while (gaps[pass] > n) --pass;

	for (--pass; pass >= 0; --pass) {
		intptr_t gap = gaps[pass];
		intptr_t nIteration = min2(gap, n - gap); // Worst case: n / 2

		thread_pool_wait_group waitGroup;
		ThreadPool_WaitGroup_Init(&waitGroup, nIteration);
		
		for (intptr_t i = 0; i < nIteration; ++i) {
			insertion_parameter parameter = { arrayHandle, array, i, n, gap, false };
			thread_pool_job Job = ThreadPool_InitJob(gappedInsertion, &parameter, &waitGroup);
			ThreadPool_AddJob(Visualizer_pThreadPool, &Job);

			while (!atomic_load_explicit(&parameter.parameterRead, memory_order_relaxed));
			atomic_thread_fence_light(&parameter.parameterRead, memory_order_acquire);
		}

		ThreadPool_WaitGroup_Wait(&waitGroup);
	}
}
