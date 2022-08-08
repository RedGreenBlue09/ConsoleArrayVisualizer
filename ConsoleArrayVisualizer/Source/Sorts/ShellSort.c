
#include "Sorts.h"
#include "Visualizer.h"

/*
* ALGORITHM INFORMATION:
* Time complexity          : O(n * log2(n))
* Extra space              : No
* Type of sort             : Comparative - Insert
* Negative integer support : Yes
*/

intptr_t gapsTokuda[54] = {
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

intptr_t gapsCiura[24] = {
	1, 4, 10, 23, 57, 132, 301, 701, 1750, 3937, 8859,
	19933, 44850, 100913, 227056, 510876, 1149471, 2586310,
	5819199, 13093198, 29459696, 66284316, 149139712, 335564353
};

intptr_t gapsPrimeMean[25] = {
	1, 4, 9, 23, 57, 138, 326, 749, 1695, 3785, 8359,
	18298, 39744, 85764, 184011, 392925, 835387, 1769455,
	3735498, 7862761, 16506016, 34568606, 72240147, 150668836,
	313682636
};

intptr_t gaps248[23] = {
	1, 3, 7, 16, 38, 94, 233, 577, 1431, 3549, 8801,
	21826, 54128, 134237, 332908, 825611, 2047515, 5077835,
	12593031, 31230716, 77452175, 192081393, 476361855
};

intptr_t gapsUnkn1[55] = {
	1, 3, 7, 16, 37, 83, 187, 419, 937, 2099,
	4693, 10499, 23479, 52501, 117391, 262495,
	586961, 1312481, 2934793, 6562397, 14673961,
	32811973, 73369801, 164059859, 366848983,
	820299269, 1834244921, 4101496331, 9171224603,
	20507481647, 45856123009, 102537408229, 229280615033,
	512687041133, 1146403075157, 2563435205663, 5732015375783,
	12817176028331, 28660076878933, 64085880141667, 143300384394667,
	320429400708323, 716501921973329, 1602147003541613, 3582509609866643,
	8010735017708063, 17912548049333207, 40053675088540303, 89562740246666023,
	200268375442701509, 447813701233330109, 1001341877213507537, 2239068506166650537,
	5006709386067537661, 11195342530833252689
};

void SHS_ShellSort(isort_t* array, intptr_t n, intptr_t* gaps, intptr_t nGaps) {

	arAddArray(0, array, n, (isort_t)n - 1);
	arUpdateArray(0);

	if (n < 2) return;

	intptr_t pass = nGaps - 1;
	while (gaps[pass] > n) --pass;
	if (pass) --pass;

	while (pass >= 0) {

		intptr_t gap = gaps[pass];

		for (intptr_t i = gap; i < n; ++i) {
			isort_t temp = array[i];
			arUpdatePointer(0, 0, i, 0.0);
			intptr_t j;

			arUpdateRead(0, i - gap, 25.0);
			for (j = i; (j >= gap) && (array[j - gap] > temp); j -= gap) {

				arUpdateRead(0, j - gap, 25.0);
				arUpdateWrite(0, j, array[j - gap], 25.0);
				array[j] = array[j - gap];
			}
			arUpdateWrite(0, j, temp, 25.0);
			array[j] = temp;
		}
		arRemovePointer(0, 0);
		--pass;
	}
	arRemoveArray(0);
	return;
}

// Exports:

void ShellSortTokuda(isort_t* array, intptr_t n) {
	SHS_ShellSort(array, n, gapsTokuda, 54);
	return;
}

void ShellSortCiura(isort_t* array, intptr_t n) {
	SHS_ShellSort(array, n, gapsCiura, 24);
	return;
}

void ShellSortPrimeMean(isort_t* array, intptr_t n) {
	SHS_ShellSort(array, n, gapsPrimeMean, 25);
	return;
}

void ShellSort248(isort_t* array, intptr_t n) {
	SHS_ShellSort(array, n, gaps248, 23);
	return;
}

void ShellSortUnkn1(isort_t* array, intptr_t n) {
	SHS_ShellSort(array, n, gapsUnkn1, 55);
	return;
}
