
#include <Windows.h>

#include "Sorts.h"
#include "Utils.h"
#include "ArrayRenderer.h"

int main() {
	utilInitClock();

	//const SIZE_T n = 16777216;
	//const uintptr_t n = 1024ULL * 1024ULL * 16ULL;
	const uintptr_t n = 64;

	isort_t* input;

	arInit();
	while (1) {
		input = rsCreateSortedArray(n);
		arUpdateArray(input, n, 0xF0);
		LeftRightQuickSort(input, n);

		free(input);

		input = rsCreateSortedArray(n);
		arUpdateArray(input, n, 0xF0);
		BottomUpHeapSort(input, n);

		free(input);
	}

	cnPause();

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

	free(input);

	return 0;
}
