
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

usize RunSorts_nSort = static_arrlen(RunSorts_aSort);

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

usize RunSorts_nDistribution = static_arrlen(RunSorts_aDistribution);

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

usize RunSorts_nShuffle = static_arrlen(RunSorts_aShuffle);

typedef struct {
	sort_info* pSort;
	distribution_info* pDistribution;
	shuffle_info* pShuffle;
	visualizer_array hArray;
	visualizer_int* aArray;
	usize Length;
} run_sorts_worker_parameter;

static void RunSorts_RunSortWorker(usize iThread, void* Parameter) {
	run_sorts_worker_parameter* pParameter = Parameter;
	sort_info* pSort                 = pParameter->pSort;
	distribution_info* pDistribution = pParameter->pDistribution;
	shuffle_info* pShuffle           = pParameter->pShuffle;
	visualizer_array hArray          = pParameter->hArray;
	visualizer_int* aArray           = pParameter->aArray;
	usize Length                     = pParameter->Length;

	Visualizer_ResetTimer();
	uint64_t Second = clock64_resolution();

	randptr_state RngState;
	srandptr(&RngState, (usize)clock64());

	// Distribute

	Visualizer_ClearReadWriteCounter();

	char sDistributionName[static_strlen("Distribution: ") + static_arrlen(pDistribution->sName)] = "";
	strcat_s(sDistributionName, static_arrlen(sDistributionName), "Distribution: ");
	strcat_s(sDistributionName, static_arrlen(sDistributionName), pDistribution->sName);
	Visualizer_SetAlgorithmName(sDistributionName);

	pDistribution->pDistribute(iThread, hArray, aArray, Length, RngState);

	// Shuffle

	char sShuffleName[static_strlen("Shuffle: ") + static_arrlen(pShuffle->sName)] = "";
	strcat_s(sShuffleName, static_arrlen(sShuffleName), "Shuffle: ");
	strcat_s(sShuffleName, static_arrlen(sShuffleName), pShuffle->sName);
	Visualizer_SetAlgorithmName(sShuffleName);

	pShuffle->pShuffle(iThread, hArray, aArray, Length, RngState);

	sleep64(Second);

	// Sort

	Visualizer_ClearReadWriteCounter();
	Visualizer_SetAlgorithmName(pSort->sName);
	Visualizer_StartTimer();
	pSort->pSort(iThread, hArray, aArray, Length);
	Visualizer_StopTimer();

	// Verify

	char sVerifyName[static_strlen("Verify: ") + static_arrlen(pDistribution->sName)] = "";
	strcat_s(sVerifyName, static_arrlen(sVerifyName), "Verify: ");
	strcat_s(sVerifyName, static_arrlen(sVerifyName), pDistribution->sName);
	Visualizer_SetAlgorithmName(sVerifyName);

	pDistribution->pVerify(iThread, hArray, aArray, Length, RngState);
	sleep64(Second * 3);
	pDistribution->pUnverify(iThread, hArray, aArray, Length, RngState);

}

void RunSorts_RunSort(
	sort_info* pSort,
	distribution_info* pDistribution,
	shuffle_info* pShuffle,
	visualizer_array hArray,
	visualizer_int* aArray,
	usize Length
) {
	run_sorts_worker_parameter Parameter = { pSort, pDistribution, pShuffle, hArray, aArray, Length };
	thread_pool_wait_group WaitGroup;
	ThreadPool_WaitGroup_Init(&WaitGroup, 1);
	thread_pool_job Job = ThreadPool_InitJob(RunSorts_RunSortWorker, &Parameter, &WaitGroup);
	ThreadPool_AddJob(Visualizer_pThreadPool, &Job);
	ThreadPool_WaitGroup_Wait(&WaitGroup);
}

