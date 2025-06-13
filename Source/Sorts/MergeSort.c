
#include "Utils/Common.h"
#include "Visualizer.h"

#include "Utils/GuardedMalloc.h"

void BinaryInsertion(visualizer_int* array, intptr_t start, intptr_t end);

static void merge(visualizer_int* c, visualizer_int* d, size_t lt, size_t md, size_t rt) {

	// Merge c[lt:md] and c[md+1:rt] to d[lt:rt]
	size_t i = lt;  // cursor for first segment
	size_t j = md;  // cursor for second
	size_t k = lt;  // cursor for result

	// merge until i or j exits its segment
	while ((i < md) && (j < rt)) {

		if (c[i] <= c[j])
			d[k++] = c[i++];
		else
			d[k++] = c[j++];

	}

	// take care of left overs
	// tjr code: only one while loop actually runs
	while (i < md)
		d[k++] = c[i++];

	while (j < rt)
		d[k++] = c[j++];

}

/**
* Perform one pass through the two arrays, invoking Merge() above
*/
static void mergePass(visualizer_int* x, visualizer_int* y, size_t s, size_t n) {

	// Merge adjacent segments of size s.
	size_t i = 0;

	// Merge two adjacent segments of size s
	size_t s2 = s * 2;
	while (i <= n - s2) {
		merge(x, y, i, i + s, i + s2);
		i += s2;
	}

	// fewer than 2s elements remain
	if (i + s < n) {
		merge(x, y, i, i + s, n);
		return;
	}

	// copy last segment to y
	for (size_t j = i; j < n; ++j)
		y[j] = x[j];

}

