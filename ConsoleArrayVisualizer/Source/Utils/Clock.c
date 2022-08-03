
#include <Windows.h>
#include <stdint.h>

uint64_t CLOCK_PER_SEC64 = 0;

void utilInitClock() {
	LARGE_INTEGER LI;
	QueryPerformanceFrequency(&LI);
	CLOCK_PER_SEC64 = LI.QuadPart;
}

uint64_t clock64() {
	LARGE_INTEGER LI;
	QueryPerformanceCounter(&LI);
	return LI.QuadPart;
}

/*
  time: Time to delay in microseconds
*/

void sleep64(uint64_t time) {

	if (CLOCK_PER_SEC64 < 1000000)
		return;

	// With clock64()
	uint64_t target = clock64();
	target += time * (CLOCK_PER_SEC64 / 1000000);
	
	// Spin waiting
	while (clock64() <= time);

	// TODO: With WinAPI Sleep().
	return;
}
