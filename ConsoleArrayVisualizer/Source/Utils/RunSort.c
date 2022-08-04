
#include "Sorts.h"
#include "Utils.h"

isort_t* rsCreateSortedArray(uintptr_t n) {

	if (n < 2) return NULL;

	isort_t* array = malloc(n * sizeof(isort_t));
	if (!array) return NULL;

	/* Init RNG */
	srand64(clock64());

	/* Random */
	for (uintptr_t i = 0; i < n; ++i)
		array[i] = (isort_t)(rand64() % n);

	/* Sort */
	//NtdllQuickSort(array, n); // i trust this sort :)

	return array;
}

void rsShuffle(isort_t* array, uintptr_t n) {

	/* Init RNG */
	srand64(clock64());

	/* Shuffle */
	for (uintptr_t i = 0; i < n; ++i) {

		uintptr_t RandI = rand64() % n;
		ISORT_SWAP(array[i], array[RandI]);
	}
};

void rsCheck(isort_t* array, isort_t* input, uintptr_t n) {

	if (n < 2) {
		printf("Array is too small.\r\n");
		return;
	}

	int32_t errNum = 0;
	for (uintptr_t i = 0; (i < n) && (errNum < 8); ++i) {

		if (array[i] != input[i]) {

			printf("#%llu is incorrect. Expected: %li, result: %li\r\n", i, input[i], array[i]);
			++errNum;

		}

	}

}

void rsRunSort(SORT_INFO* si, isort_t* input, uintptr_t n) {

	if (n < 2)
		return;
	printf("RUNNING SORT: %48s ----------------\r\n", si->name);
	isort_t* array = malloc(n * sizeof(isort_t));
	if (!array)
		return;

	uint64_t start;
	uint64_t end;

	// Shuffle
	printf("Shuffling array...     ");

	start = clock64();
	memcpy(array, input, n * sizeof(isort_t));
	rsShuffle(array, n);
	end = clock64();

	printf("%12llu us\r\n", (end - start) / 10);

	// Sort
	printf("Sorting...             ");

	start = clock64();
	si->sortFunc(array, n);
	end = clock64();

	printf("%12llu us\r\n", (end - start) / 10);

	// Check
	printf("Checking array...      ");

	start = clock64();
	rsCheck(array, input, n);
	end = clock64();

	printf("%12llu us\r\n", (end - start) / 10);

	//
	printf("\r\n");
	free(array);
	return;
}
