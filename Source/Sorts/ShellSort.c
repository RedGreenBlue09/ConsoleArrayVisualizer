
#include "Visualizer.h"
#include "Utils/Common.h"
#include "Utils/Machine.h"

/*
* ALGORITHM INFORMATION:
* Time complexity          : O(n * log(n))
* Extra space              : No
* Type of sort             : Comparative - Insert
* Negative integer support : Yes
*/

// TODO: 32-bit intptr_t support

intptr_t gapsTokuda[] = {
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

static void shellSort(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n, intptr_t* gaps, intptr_t nGaps) {

	if (n < 2) return;

	intptr_t pass = nGaps - 1;
	while (gaps[pass] > n) --pass;
	if (pass) --pass;

	while (pass >= 0) {

		intptr_t gap = gaps[pass];

		visualizer_marker pointer = Visualizer_CreateMarker(arrayHandle, gap, visualizer_marker_attribute_Pointer);
		for (intptr_t i = gap; i < n; ++i) {
			visualizer_int temp = array[i];
			Visualizer_UpdateRead(arrayHandle, i, 0.25);
			Visualizer_MoveMarker(&pointer, i);
			intptr_t j;

			visualizer_int temp2;
			for (j = i; (j >= gap) && ((temp2 = array[j - gap]) > temp); j -= gap) {
				Visualizer_UpdateRead(arrayHandle, j - gap, 0.25);
				Visualizer_UpdateWrite(arrayHandle, j, temp2, 0.25);
				array[j] = temp2;
			}
			Visualizer_UpdateWrite(arrayHandle, j, temp, 0.25);
			array[j] = temp;
		}
		Visualizer_RemoveMarker(pointer);
		--pass;
	}

	return;

}

// Exports:

void ShellSortTokuda(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n) {
	shellSort(arrayHandle, array, n, gapsTokuda, static_arrlen(gapsTokuda));
	return;
}

