
#include "RunSorts.h"

#include <math.h>
#include <string.h>

#include "Utils/Common.h"
#include "Utils/Machine.h"
#include "Utils/Time.h"
#include "Utils/Random.h"

sort_function ShellSortTokuda;
sort_function LeftRightQuickSort;
sort_function ImprovedIntroSort;
sort_function BottomUpHeapSort;
sort_function WeaveSortParallel;
sort_function ShellSortParallel;
sort_function LeftRightQuickSortParallel;

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
	{
		"Parallel Left/Right Quicksort",
		LeftRightQuickSortParallel,
	},
};

uintptr_t RunSorts_nSort = static_arrlen(RunSorts_aSort);

distribute_function DistributeLinear;
verify_function VerifyLinear;
unverify_function UnverifyLinear;

distribute_function DistributeRandom;
verify_function VerifyRandom;
unverify_function UnverifyRandom;

distribute_function DistributeGaussian;
verify_function VerifyGaussian;
unverify_function UnverifyGaussian;

distribute_function DistributeSquareRoot;
verify_function VerifySquareRoot;
unverify_function UnverifySquareRoot;

distribute_function DistributeSine;
verify_function VerifySine;
unverify_function UnverifySine;

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

shuffle_function ShuffleRandom;
shuffle_function ShuffleSorted;
shuffle_function ShuffleReversed;
shuffle_function ShuffleFinalMerge;
shuffle_function ShuffleReversedFinalMerge;
shuffle_function ShuffleMirrored;
shuffle_function ShuffleQuickSortAdversary;

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

	randptr_state RngState;
	srandptr(&RngState, (uintptr_t)clock64());

	// Distribute

	Visualizer_ClearReadWriteCounter();

	char sDistributionName[static_strlen("Distribution: ") + static_arrlen(pDistribution->sName)] = "";
	strcat_s(sDistributionName, static_arrlen(sDistributionName), "Distribution: ");
	strcat_s(sDistributionName, static_arrlen(sDistributionName), pDistribution->sName);
	Visualizer_SetAlgorithmName(sDistributionName);

	pDistribution->pDistribute(RngState, hArray, aArray, Length);

	// Shuffle
	
	char sShuffleName[static_strlen("Shuffle: ") + static_arrlen(pShuffle->sName)] = "";
	strcat_s(sShuffleName, static_arrlen(sShuffleName), "Shuffle: ");
	strcat_s(sShuffleName, static_arrlen(sShuffleName), pShuffle->sName);
	Visualizer_SetAlgorithmName(sShuffleName);

	pShuffle->pShuffle(RngState, hArray, aArray, Length);

	sleep64(Second);

	// Sort

	Visualizer_ClearReadWriteCounter();
	Visualizer_SetAlgorithmName(pSort->sName);
	Visualizer_StartTimer();
	pSort->pSort(hArray, aArray, Length);
	Visualizer_StopTimer();

	// Verify

	char sVerifyName[static_strlen("Verify: ") + static_arrlen(pDistribution->sName)] = "";
	strcat_s(sVerifyName, static_arrlen(sVerifyName), "Verify: ");
	strcat_s(sVerifyName, static_arrlen(sVerifyName), pDistribution->sName);
	Visualizer_SetAlgorithmName(sVerifyName);

	pDistribution->pVerify(RngState, hArray, aArray, Length);
	sleep64(Second * 3);
	pDistribution->pUnverify(RngState, hArray, aArray, Length);
}

