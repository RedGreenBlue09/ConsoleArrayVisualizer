
#include <tgmath.h>

#include "Visualizer.h"
#include "Utils/Random.h"

typedef struct {
	floatptr_t sum;
	floatptr_t cs;
	floatptr_t ccs;
} sum_state;

visualizer_int KahanBabushkaKleinSum(sum_state* s, floatptr_t next) {
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

	if (fabs(s->sum) >= 2.0f / FPTR_EPSILON)
		return (visualizer_int)s->sum + (visualizer_int)round(s->cs + s->ccs);
	else
		return (visualizer_int)round(s->sum + (s->cs + s->ccs));
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
		visualizer_int Value = KahanBabushkaKleinSum(&SumState, -log(fRandom));
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
		visualizer_int Value = KahanBabushkaKleinSum(&SumState, -log(fRandom));
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
		visualizer_int Value = KahanBabushkaKleinSum(&SumState, -log(fRandom));
		Visualizer_ClearCorrectness(iThread, hArray, i, aArray[i] == Value);
	}
}
