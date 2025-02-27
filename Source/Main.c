
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <string.h>

#include "RunSorts.h"
#include "Utils/GuardedMalloc.h"


int main(int argc, char** argv) {

	if (argc < 4) {
		printf("Usage: %s <Array length> <Sleep multiplier> <Algorithm 1> <Algorithm 2>\n", argv[0]);
		printf("Note: Use algorithm index.\n");
		printf("\n");
		printf("Algorithms:\n");
		for (uintptr_t i = 0; i < RunSorts_nSort; ++i)
			printf("%zu %s\n", i, RunSorts_aSortList[i].sName);

		return 0;
	}

	int ReadChars;

	uintptr_t ArrayLengthUnsigned;
	if (
		sscanf(argv[1], "%tu %n", &ArrayLengthUnsigned, &ReadChars) != 1 ||
		ReadChars != strlen(argv[1]) ||
		ArrayLengthUnsigned == 0
	) {
		printf("Error: Invalid array length \'%s\'\n", argv[1]);
		return 0;
	}
	intptr_t ArrayLength = ArrayLengthUnsigned;

	double fSleepMultiplier;
	if (
		sscanf(argv[2], "%lf %n", &fSleepMultiplier, &ReadChars) != 1 ||
		ReadChars != strlen(argv[2])
	) {
		printf("Error: Invalid sleep multiplier \'%s\'\n", argv[2]);
		return 0;
	}

	visualizer_int* aArray = calloc_guarded(ArrayLength, sizeof(visualizer_int));
	Visualizer_Initialize();
	visualizer_array_handle hArray = Visualizer_AddArray(ArrayLength, NULL, 0, (visualizer_int)ArrayLength - 1);
	Visualizer_SetUserSleepMultiplier(fSleepMultiplier);

	for (int i = 3; i < argc; ++i) {
		uintptr_t iAlgorithm;
		if (
			sscanf(argv[i], "%tu %n", &iAlgorithm, &ReadChars) != 1 ||
			ReadChars != strlen(argv[i]) ||
			iAlgorithm >= RunSorts_nSort
		) {
			printf("Warning: Invalid array index \'%s\'\n", argv[i]);
			continue;
		}
		RunSorts_RunSort(&RunSorts_aSortList[iAlgorithm], hArray, aArray, ArrayLength);
	}

	Visualizer_RemoveArray(hArray);
	Visualizer_Uninitialize();

	free(aArray);
	return 0;

}
