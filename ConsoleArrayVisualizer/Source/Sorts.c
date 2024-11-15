
#include "Sorts.h"

uintptr_t Sorts_nSort = sizeof(Sorts_aSortList) / sizeof(*Sorts_aSortList);

SORT_INFO Sorts_aSortList[128] = {
	{
		"ShellSort (Tokuda's gaps)",
		ShellSortTokuda,
	},
	{
		"ShellSort (Ciura's gaps)",
		ShellSortCiura,
	},
	{
		"Left/Right QuickSort",
		LeftRightQuickSort,
	},
	{
		"Iterative MergeSort",
		IterativeMergeSort,
	},
	{
		"Weak HeapSort",
		WeakHeapSort,
	},
	{
		"Bottom-up HeapSort",
		BottomUpHeapSort,
	},
};

