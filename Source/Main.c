
#include "RunSorts.h"
#include "Utils/GuardedMalloc.h"

int main() {
	//sleep64(5000000);

	//intptr_t N = 512;
	intptr_t N = 1 << 22;
	visualizer_int* aArray = calloc_guarded(N, sizeof(visualizer_int));

	Visualizer_Initialize();
	visualizer_array_handle hArray = Visualizer_AddArray(N, NULL, 0, (visualizer_int)N - 1);

	//
	RunSorts_RunSort(RunSorts_aSortList + 3, hArray, aArray, N);
	RunSorts_RunSort(RunSorts_aSortList + 1, hArray, aArray, N);
	RunSorts_RunSort(RunSorts_aSortList + 0, hArray, aArray, N);

	//
	Visualizer_RemoveArray(hArray);
	Visualizer_Uninitialize();

	free(aArray);
	return 0;

}
