
#include "Sorts.h"
#include "Visualizer.h"

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

int main() {
	utilInitTime();


	return 0;
}
