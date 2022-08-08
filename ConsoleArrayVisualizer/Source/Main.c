
#include "Sorts.h"
#include "Visualizer.h"
#include "RunSort.h"

#include "Utils.h"
#include <stdio.h>

int main() {
	utilInitTime();

	//const SIZE_T n = 16777216;
	//const intptr_t n = 1024ULL * 1024ULL * 16ULL;
	const intptr_t n = 256;

	isort_t* input;

	arInit();

	
	input = rsCreateSortedArray(n);
	rsShuffle(input, n);
	LeftRightQuickSort(input, n);

	free(input);

	input = rsCreateSortedArray(n);
	rsShuffle(input, n);
	BottomUpHeapSort(input, n);

	free(input);

	input = rsCreateSortedArray(n);
	rsShuffle(input, n);
	ShellSort248(input, n);

	arAddArray(0, input, n, (isort_t)n - 1);
	arUpdateArray(0);
	arRemoveArray(0);

	free(input);

	Sleep(10000000);
	arUninit();

	//rsRunSort(&sortsList[0], input, n);

	//rsRunSort(&sortsList[1], input, n);

	//rsRunSort(&sortsList[2], input, n);

	//rsRunSort(&sortsList[3], input, n);

	//rsRunSort(&sortsList[4], input, n);

	//rsRunSort(&sortsList[5], input, n);

	//rsRunSort(&sortsList[6], input, n);

	//rsRunSort(&sortsList[7], input, n);

	//rsRunSort(&sortsList[8], input, n);

	//rsRunSort(&sortsList[9], input, n);

	//free(input);

	return 0;
}
