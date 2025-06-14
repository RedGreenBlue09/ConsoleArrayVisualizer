
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <string.h>

#include "RunSorts.h"
#include "Utils/GuardedMalloc.h"
#include "Utils/Time.h"

int main(int argc, char** argv) {

	//sleep64(50000000);
	
	if (argc < 7) {
		printf("Usage:\n");
		printf("	%s\n", argv[0]);
		printf("	<Extra thread count> <Array length> <Sleep multiplier>\n");
		printf("	<Distribution ID> <Shuffle ID> <Algorithm ID> <Algorithm ID> ...\n");
		printf("\n");
		printf("Extra thread count: The number of extra threads for the algorithm. (Max: 125)\n");
		printf("It is recommended to leave a free thread for the renderer.\n");
		printf("\n");
		printf("Distributions:\n");
		for (uintptr_t i = 0; i < RunSorts_nDistribution; ++i)
			printf("%tu - %s\n", i, RunSorts_aDistribution[i].sName);
		printf("\n");
		printf("Shuffles:\n");
		for (uintptr_t i = 0; i < RunSorts_nShuffle; ++i)
			printf("%tu - %s\n", i, RunSorts_aShuffle[i].sName);
		printf("\n");
		printf("Algorithms:\n");
		for (uintptr_t i = 0; i < RunSorts_nSort; ++i)
			printf("%tu - %s\n", i, RunSorts_aSort[i].sName);

		return 0;
	}

	int ReadChars;

	size_t ExtraThreadCount = 0;
	if (
		sscanf(argv[1], "%zu %n", &ExtraThreadCount, &ReadChars) != 1 ||
		ReadChars != strlen(argv[1]) ||
		ExtraThreadCount > 125
	) {
		printf("Error: Invalid extra thread count \'%s\'\n", argv[1]);
		return 0;
	}

	intptr_t ArrayLength = 0;
	if (
		sscanf(argv[2], "%ti %n", &ArrayLength, &ReadChars) != 1 ||
		ReadChars != strlen(argv[2]) ||
		ArrayLength <= 0
	) {
		printf("Error: Invalid array length \'%s\'\n", argv[2]);
		return 0;
	}

	double fSleepMultiplier = 1.0;
	if (
		sscanf(argv[3], "%lf %n", &fSleepMultiplier, &ReadChars) != 1 ||
		ReadChars != strlen(argv[3]) ||
		fSleepMultiplier < 0.0
	) {
		printf("Error: Invalid sleep multiplier \'%s\'\n", argv[3]);
		return 0;
	}

	uintptr_t iDistribution = 0;
	if (
		sscanf(argv[4], "%tu %n", &iDistribution, &ReadChars) != 1 ||
		ReadChars != strlen(argv[4]) ||
		iDistribution >= RunSorts_nDistribution
	) {
		printf("Warning: Invalid distribution ID \'%s\'\n", argv[4]);
		return 0;
	}

	uintptr_t iShuffle = 0;
	if (
		sscanf(argv[5], "%tu %n", &iShuffle, &ReadChars) != 1 ||
		ReadChars != strlen(argv[5]) ||
		iShuffle >= RunSorts_nShuffle
	) {
		printf("Warning: Invalid shuffle ID \'%s\'\n", argv[5]);
		return 0;
	}

	// TODO: Ask user about array distro and shuffle

	visualizer_int* aArray = calloc_guarded(ArrayLength, sizeof(visualizer_int));
	Visualizer_Initialize(ExtraThreadCount);
	visualizer_array_handle hArray = Visualizer_AddArray(ArrayLength, NULL, 0, (visualizer_int)ArrayLength - 1);
	Visualizer_SetUserSleepMultiplier(fSleepMultiplier);

	for (int i = 6; i < argc; ++i) {
		uintptr_t iAlgorithm = 0;
		if (
			sscanf(argv[i], "%tu %n", &iAlgorithm, &ReadChars) != 1 ||
			ReadChars != strlen(argv[i]) ||
			iAlgorithm >= RunSorts_nSort
		) {
			printf("Warning: Invalid algorithm ID \'%s\'\n", argv[i]);
			continue;
		}
		RunSorts_RunSort(
			&RunSorts_aSort[iAlgorithm],
			&RunSorts_aDistribution[iDistribution],
			&RunSorts_aShuffle[iShuffle],
			hArray,
			aArray,
			ArrayLength
		);
	}

	Visualizer_RemoveArray(hArray);
	Visualizer_Uninitialize();

	free(aArray);
	return 0;

}
