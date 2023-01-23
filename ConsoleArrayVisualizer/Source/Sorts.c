
#include "Sorts.h"

#include "Utils.h"

uintptr_t sortsCount = sizeof(sortsList) / sizeof(*sortsList);

SORT_INFO sortsList[128] = {
	{
		"ShellSort (Tokuda's gaps)",
		ShellSortTokuda,
	},
	{
		"ShellSort (Ciura's gaps)",
		ShellSortCiura,
	},
	{
		"ShellSort (Prime-mean gaps)",
		ShellSortPrimeMean,
	},
	{
		"ShellSort (2.48 gaps)",
		ShellSort248,
	},
	{
		"ShellSort (Pigeon's gaps)",
		ShellSortPigeon,
	},
	{
		"ShellSort (Sedgewick's gaps)",
		ShellSortSedgewick1986,
	},
	{
		"ShellSort (Cbrt16 gaps)",
		ShellSortCbrt16,
	},
	{
		"ShellSort (Cbrt16-1 gaps)",
		ShellSortCbrt16p1,
	},
	{
		"stdlib QuickSort",
		StdlibQuickSort,
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

