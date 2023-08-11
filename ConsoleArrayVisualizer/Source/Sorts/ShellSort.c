
#include "Sorts.h"
#include "Visualizer/Visualizer.h"

/*
* ALGORITHM INFORMATION:
* Time complexity          : O(n * log2(n))
* Extra space              : No
* Type of sort             : Comparative - Insert
* Negative integer support : Yes
*/

// TODO: 32-bit intptr_t support

intptr_t gapsTokuda[] = {
	1, 4, 9, 20, 46, 103, 233, 525, 1182, 2660, 5985, 13467, 30301,
	68178, 153401, 345152, 776591, 1747331, 3931496, 8845866, 19903198,
	44782196, 100759940, 226709866, 510097200, 1147718700, 2582367076,
	5810325920, 13073233321, 29414774973, 66183243690, 148912298303,
	335052671183, 753868510162, 1696204147864, 3816459332694, 8587033498562,
	19320825371765, 43471857086472, 97811678444563, 220076276500268,
	495171622125603, 1114136149782608, 2506806337010869, 5640314258274455,
	12690707081117525, 28554090932514431, 64246704598157469, 144555085345854306,
	325248942028172190, 731810119563387427, 1646572769017621711, 3704788730289648850,
	8335774643151709914
};

intptr_t gapsCiura[] = {
	1, 4, 10, 23, 57, 132, 301, 701, 1750, 3937, 8859,
	19933, 44850, 100913, 227056, 510876, 1149471, 2586310,
	5819199, 13093198, 29459696, 66284316, 149139712, 335564353
};

intptr_t gapsPrimeMean[] = {
	1, 4, 9, 23, 57, 138, 326, 749, 1695, 3785, 8359,
	18298, 39744, 85764, 184011, 392925, 835387, 1769455,
	3735498, 7862761, 16506016, 34568606, 72240147, 150668836,
	313682636
};

intptr_t gaps248[] = {
	1, 3, 7, 16, 38, 94, 233, 577, 1431, 3549, 8801,
	21826, 54128, 134237, 332908, 825611, 2047515, 5077835,
	12593031, 31230716, 77452175, 192081393, 476361855
};

intptr_t gapsPigeon[] = {
	1, 2, 4, 8, 21, 56, 149, 404, 1098, 2982, 8104, 22027,
	59875, 162756, 442414, 1202605, 3269018, 8886112, 24154954,
	65659970, 178482302, 485165196, 1318815735, 3584912847, 9744803447,
	26489122131, 72004899338, 195729609430
};

intptr_t gapsSedgewick1986[] = {
	1, 5, 19, 41, 109, 209, 505, 929, 2161, 3905, 8929, 16001,
	36289, 64769, 146305, 260609, 587521, 1045505, 2354689, 4188161,
	9427969, 16764929, 37730305, 67084289, 150958081, 268386305, 603906049,
	1073643521, 2415771649, 4294770689, 9663381505, 17179475969
};

intptr_t gapsCbrt16[] = {
	1, 3, 7, 16, 41, 102, 256, 646, 1626, 4096, 10322, 26008,
	65536, 165141, 416128, 1048576, 2642246, 6658043, 16777216,
	42275936, 106528682, 268435456, 676414965
};

intptr_t gapsCbrt16p1[] = {
	1, 4, 8, 17, 42, 103, 257, 647, 1627, 4097, 10323, 26009,
	65537, 165142, 416129, 1048577, 2642247, 6658044, 16777217,
	42275937, 106528683, 268435457, 676414966
};

void SHS_ShellSort(isort_t* array, intptr_t n, intptr_t* gaps, intptr_t nGaps) {

	if (n < 2) return;

	intptr_t pass = nGaps - 1;
	while (gaps[pass] > n) --pass;
	if (pass) --pass;

	while (pass >= 0) {

		intptr_t gap = gaps[pass];

		for (intptr_t i = gap; i < n; ++i) {
			isort_t temp = array[i];
			Visualizer_UpdatePointer(0, 0, i);
			intptr_t j;

			Visualizer_UpdateRead(0, i - gap, 0.25);
			for (j = i; (j >= gap) && (array[j - gap] > temp); j -= gap) {
				Visualizer_UpdateRead(0, j - gap, 0.25);
				Visualizer_UpdateWrite(0, j, array[j - gap], 0.24);
				array[j] = array[j - gap];
			}
			Visualizer_UpdateWrite(0, j, temp, 0.25);
			array[j] = temp;
		}
		Visualizer_RemovePointer(0, 0);
		--pass;
	}

	return;

}

// Exports:

void ShellSortTokuda(isort_t* array, intptr_t n) {
	SHS_ShellSort(array, n, gapsTokuda, sizeof(gapsTokuda) / sizeof(*gapsTokuda));
	return;
}

void ShellSortCiura(isort_t* array, intptr_t n) {
	SHS_ShellSort(array, n, gapsCiura, sizeof(gapsCiura) / sizeof(*gapsCiura));
	return;
}

void ShellSortPrimeMean(isort_t* array, intptr_t n) {
	SHS_ShellSort(array, n, gapsPrimeMean, sizeof(gapsPrimeMean) / sizeof(*gapsPrimeMean));
	return;
}

void ShellSort248(isort_t* array, intptr_t n) {
	SHS_ShellSort(array, n, gaps248, sizeof(gaps248) / sizeof(*gaps248));
	return;
}

void ShellSortPigeon(isort_t* array, intptr_t n) {
	SHS_ShellSort(array, n, gapsPigeon, sizeof(gapsPigeon) / sizeof(*gapsPigeon));
	return;
}

void ShellSortSedgewick1986(isort_t* array, intptr_t n) {
	SHS_ShellSort(array, n, gapsSedgewick1986, sizeof(gapsSedgewick1986) / sizeof(*gapsSedgewick1986));
	return;
}

void ShellSortCbrt16(isort_t* array, intptr_t n) {
	SHS_ShellSort(array, n, gapsCbrt16, sizeof(gapsCbrt16) / sizeof(*gapsCbrt16));
	return;
}

void ShellSortCbrt16p1(isort_t* array, intptr_t n) {
	SHS_ShellSort(array, n, gapsCbrt16p1, sizeof(gapsCbrt16p1) / sizeof(*gapsCbrt16p1));
	return;
}
