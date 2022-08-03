#include "Sorts.h"
#include "Utils.h"

uint32_t sortsCount = 10;

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
		"ShellSort (Unknown gaps #1)",
		ShellSortUnkn1,
	},
	{
		"ntdll.dll QuickSort",
		NtdllQuickSort,
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

