
#include <tgmath.h>

#include "Visualizer.h"
#include "Utils/Random.h"

typedef struct {
	floatptr_t sum;
	floatptr_t cs;
	floatptr_t ccs;
} sum_state;

floatptr_t KahanBabushkaKleinSum(sum_state* s, floatptr_t next) {
	floatptr_t t = s->sum + next;
	floatptr_t c;
	if (fabs(s->sum) >= fabs(next))
		c = (s->sum - t) + next;
	else
		c = (next - t) + s->sum;
	s->sum = t;

	t = s->cs + c;
	floatptr_t cc;
	if (fabs(s->cs) >= fabs(c))
		cc = (s->cs - t) + c;
	else
		cc = (c - t) + s->cs;
	s->cs = t;

	s->ccs = s->ccs + cc;
	return s->sum + (s->cs + s->ccs);
}

void DistributeRandom(
	usize iThread,
	visualizer_array hArray,
	visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125f, Visualizer_SleepScale_N)
	);

	sum_state SumState = { 0 };
	for (usize i = 0; i < Length; ++i) {
		floatptr_t fRandom = ext_max(randfptr(&RngState), FPTR_EPSILON * 0.5f);
		floatptr_t fValue = KahanBabushkaKleinSum(&SumState, -log(fRandom));
		visualizer_int Value = (visualizer_int)round(fValue);
		Visualizer_UpdateWrite(iThread, hArray, i, Value, 1.0f);
		aArray[i] = Value;
	}
}

void VerifyRandom(
	usize iThread,
	visualizer_array hArray,
	const visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625f, Visualizer_SleepScale_N)
	);

	sum_state SumState = { 0 };
	for (usize i = 0; i < Length; ++i) {
		floatptr_t fRandom = ext_max(randfptr(&RngState), FPTR_EPSILON * 0.5f);
		floatptr_t fValue = KahanBabushkaKleinSum(&SumState, -log(fRandom));
		visualizer_int Value = (visualizer_int)round(fValue);
		Visualizer_UpdateCorrectness(iThread, hArray, i, aArray[i] == Value, 1.0f);
	}
}

void UnverifyRandom(
	usize iThread,
	visualizer_array hArray,
	const visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	sum_state SumState = { 0 };
	for (usize i = 0; i < Length; ++i) {
		floatptr_t fRandom = ext_max(randfptr(&RngState), FPTR_EPSILON * 0.5f);
		floatptr_t fValue = KahanBabushkaKleinSum(&SumState, -log(fRandom));
		visualizer_int Value = (visualizer_int)round(fValue);
		Visualizer_ClearCorrectness(iThread, hArray, i, aArray[i] == Value);
	}
}
