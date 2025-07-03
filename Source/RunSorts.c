
#include "RunSorts.h"

#include <math.h>
#include <string.h>

#include "Utils/Common.h"
#include "Utils/Machine.h"
#include "Utils/Time.h"
#include "Utils/Random.h"

void ShellSortTokuda(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n);
void ShellSortParallel(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n);
void LeftRightQuickSort(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n);
void ImprovedIntroSort(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n);
void BottomUpHeapSort(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n);
void WeaveSortParallel(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n);

sort_info RunSorts_aSort[] = {
	{
		"Shellsort (Tokuda's gaps)",
		ShellSortTokuda,
	},
	{
		"Left/Right Quicksort",
		LeftRightQuickSort,
	},
	{
		"Improved Introsort",
		ImprovedIntroSort,
	},
	{
		"Bottom-up Heapsort",
		BottomUpHeapSort,
	},
	{
		"Parallel Weave Sort",
		WeaveSortParallel,
	},
	{
		"Parallel Shellsort (Tokuda's gaps)",
		ShellSortParallel,
	},
};

uintptr_t RunSorts_nSort = static_arrlen(RunSorts_aSort);

void DistributeLinear(rand64_state, visualizer_array_handle, visualizer_int*, intptr_t);
void VerifyLinear(rand64_state, visualizer_array_handle, const visualizer_int*, intptr_t);
void UnverifyLinear(rand64_state, visualizer_array_handle, const visualizer_int*, intptr_t);

void DistributeRandom(rand64_state, visualizer_array_handle, visualizer_int*, intptr_t);
void VerifyRandom(rand64_state, visualizer_array_handle, const visualizer_int*, intptr_t);
void UnverifyRandom(rand64_state, visualizer_array_handle, const visualizer_int*, intptr_t);

void DistributeGaussian(rand64_state, visualizer_array_handle, visualizer_int*, intptr_t);
void VerifyGaussian(rand64_state, visualizer_array_handle, const visualizer_int*, intptr_t);
void UnverifyGaussian(rand64_state, visualizer_array_handle, const visualizer_int*, intptr_t);

void DistributeSquareRoot(rand64_state, visualizer_array_handle, visualizer_int*, intptr_t);
void VerifySquareRoot(rand64_state, visualizer_array_handle, const visualizer_int*, intptr_t);
void UnverifySquareRoot(rand64_state, visualizer_array_handle, const visualizer_int*, intptr_t);

void DistributeSine(rand64_state, visualizer_array_handle, visualizer_int*, intptr_t);
void VerifySine(rand64_state, visualizer_array_handle, const visualizer_int*, intptr_t);
void UnverifySine(rand64_state, visualizer_array_handle, const visualizer_int*, intptr_t);

distribution_info RunSorts_aDistribution[] = {
	{
		"Linear",
		DistributeLinear,
		VerifyLinear,
		UnverifyLinear,
	},
	{
		"Random",
		DistributeRandom,
		VerifyRandom,
		UnverifyRandom,
	},
	{
		"Gaussian",
		DistributeGaussian,
		VerifyGaussian,
		UnverifyGaussian,
	},
	{
		"Square Root",
		DistributeSquareRoot,
		VerifySquareRoot,
		UnverifySquareRoot,
	},
	{
		"Sine",
		DistributeSine,
		VerifySine,
		UnverifySine,
	},
};

uintptr_t RunSorts_nDistribution = static_arrlen(RunSorts_aDistribution);

void ShuffleRandom(rand64_state, visualizer_array_handle, visualizer_int*, intptr_t);
void ShuffleSorted(rand64_state, visualizer_array_handle, visualizer_int*, intptr_t);
void ShuffleReversed(rand64_state, visualizer_array_handle, visualizer_int*, intptr_t);
void ShuffleFinalMerge(rand64_state, visualizer_array_handle, visualizer_int*, intptr_t);
void ShuffleReversedFinalMerge(rand64_state, visualizer_array_handle, visualizer_int*, intptr_t);
void ShuffleMirrored(rand64_state, visualizer_array_handle, visualizer_int*, intptr_t);
void ShuffleQuickSortAdversary(rand64_state, visualizer_array_handle, visualizer_int*, intptr_t);

shuffle_info RunSorts_aShuffle[] = {
	{
		"Random",
		ShuffleRandom,
	},
	{
		"Reversed",
		ShuffleReversed,
	},
	{
		"Sorted",
		ShuffleSorted,
	},
	{
		"Final Merge",
		ShuffleFinalMerge,
	},
	{
		"Reversed Final Merge",
		ShuffleReversedFinalMerge,
	},
	{
		"Mirrored",
		ShuffleMirrored,
	},
	{
		"Quicksort Adversary",
		ShuffleQuickSortAdversary,
	},
};

uintptr_t RunSorts_nShuffle = static_arrlen(RunSorts_aShuffle);

void RunSorts_RunSort(
	sort_info* pSort,
	distribution_info* pDistribution,
	shuffle_info* pShuffle,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_ResetTimer();
	uint64_t Second = clock64_resolution();

	rand64_state RngState;
	srand64(&RngState, clock64());

	// Distribute

	Visualizer_ClearReadWriteCounter(hArray);

	char sDistributionName[static_strlen("Distribution: ") + static_arrlen(pDistribution->sName)] = "";
	strcat_s(sDistributionName, static_arrlen(sDistributionName), "Distribution: ");
	strcat_s(sDistributionName, static_arrlen(sDistributionName), pDistribution->sName);
	Visualizer_SetAlgorithmName(sDistributionName);

	pDistribution->Distribute(RngState, hArray, aArray, Length);

	// Shuffle
	
	char sShuffleName[static_strlen("Shuffle: ") + static_arrlen(pShuffle->sName)] = "";
	strcat_s(sShuffleName, static_arrlen(sShuffleName), "Shuffle: ");
	strcat_s(sShuffleName, static_arrlen(sShuffleName), pShuffle->sName);
	Visualizer_SetAlgorithmName(sShuffleName);

	pShuffle->Shuffle(RngState, hArray, aArray, Length);

	sleep64(Second);

	// Sort

	Visualizer_ClearReadWriteCounter(hArray);
	Visualizer_SetAlgorithmName(pSort->sName);
	Visualizer_StartTimer();
	pSort->Sort(hArray, aArray, Length);
	Visualizer_StopTimer();

	// Verify

	char sVerifyName[static_strlen("Verify: ") + static_arrlen(pDistribution->sName)] = "";
	strcat_s(sVerifyName, static_arrlen(sVerifyName), "Verify: ");
	strcat_s(sVerifyName, static_arrlen(sVerifyName), pDistribution->sName);
	Visualizer_SetAlgorithmName(sVerifyName);

	pDistribution->Verify(RngState, hArray, aArray, Length);
	sleep64(Second * 3);
	pDistribution->Unverify(RngState, hArray, aArray, Length);
}

