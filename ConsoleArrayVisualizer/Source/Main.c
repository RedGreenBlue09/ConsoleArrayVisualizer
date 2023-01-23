
#include "Sorts.h"
#include "Visualizer.h"
#include "RunSort.h"

#define _USE_MATH_DEFINES

#include "Utils.h"
#include <stdio.h>
#include <math.h>

double myKahanSum(double* ax, size_t n) {
	double sum = 0.0;
	double c = 0.0;
	for (size_t i = 0; i < n; ++i) {
		double y = ax[i] - c;
		double t = sum + y;
		c = (t - sum) - y;
		sum = t;
	}
	return sum;
}


double log2fact(double x) {
	double x1 = x + 1.0;
	if (x <= 128.0) {
		double gamma = tgamma(x1);
		return log2(gamma);
	} else {
		double a[8];
		a[0] = x1 * log(x1);
		a[1] = -x1;
		a[2] = -0.5 * log(x1);
		a[3] = 0.5 * log(2 * M_PI);
		a[4] = 1.0 / (12 * x1);
		a[5] = -1.0 / (360 * pow(x1, 3.0));
		a[6] = 1.0 / (1260 * pow(x1, 5.0));
		a[7] = 0.0;
		return (myKahanSum(a, 8) / M_LN2);
	}
}

// ShellSort.c
extern uint64_t nCompare;
extern uint64_t nWrite;

int main() {
	utilInitTime();

	//const intptr_t n = 1ll << 27ll;
	//const intptr_t n = 256;

	isort_t* input;
	/*

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

	arUninit();
	*/

	FILE* file;
	if (fopen_s(&file, "Results.txt", "w") == EINVAL) {
		printf("Unable to open file.\r\n");
		return 0;
	}

	for (uintptr_t j = 0; j < 1; ++j) {

		double dfLimit = 27.0;

		printf("Running: %s\r\n", sortsList[j].name);
		fprintf(file, "\r\n%s\r\n", sortsList[j].name);

		for (double dfi = 1.0; dfi <= dfLimit; dfi += 0.25) {

			intptr_t n;
			uint64_t nIteration;

			uint64_t sumCompare;
			uint64_t sumWrite;

			n = (intptr_t)exp2(dfi);
			//nIteration = (uint64_t)round(exp2(dfLimit - dfi
			nIteration = (1ull << 20ull) / n;

			if (nIteration > (1ull << 20ull))
				nIteration = (1ull << 20ull);
			else if (nIteration < 2)
				nIteration = 2;

			sumCompare = 0;
			sumWrite = 0;

			for (uint64_t i = 0; i < nIteration; ++i) {

				input = rsCreateSortedArray(n);
				rsShuffle(input, n);

				sortsList[j].sortFunc(input, n);

				free(input);

				sumCompare += nCompare;
				sumWrite += nWrite;

			}
			fprintf(
				file,
				"%g||%.4f|%.4f||%.4f|%.4f\r\n",
				dfi,                                        // 2^x
				(double)sumCompare / (double)nIteration,    // avg comparisions
				(double)sumWrite / (double)nIteration,      // avg writes
				log2fact((double)n),                        // log2 n!
				(double)n * log2((double)n)                 // n log n
			);

		}
	}

	fclose(file);
	return 0;
}
