
#include <Windows.h>
#include <stdint.h>

NTSYSAPI NTSTATUS NTAPI NtQueryTimerResolution(
	OUT PULONG MinimumResolution,
	OUT PULONG MaximumResolution,
	OUT PULONG CurrentResolution
);

uint64_t CLOCK_PER_SEC64 = 0;
ULONG tickResolution = 0;

void utilInitTime() {
	LARGE_INTEGER LI;
	QueryPerformanceFrequency(&LI);
	CLOCK_PER_SEC64 = LI.QuadPart;

	ULONG tickMinRes;
	ULONG tickMaxRes;
	NtQueryTimerResolution(&tickMinRes, &tickMaxRes, &tickResolution);

	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), 31); // THREAD_PRIORITY_TIME_CRITICAL

}

uint64_t clock64() {
	LARGE_INTEGER LI;
	QueryPerformanceCounter(&LI);
	return LI.QuadPart;
}

static void _sleep64_waitabletimer(uint64_t time) {

	HANDLE hTimer = CreateWaitableTimerExW(
		NULL,
		NULL,
		CREATE_WAITABLE_TIMER_MANUAL_RESET | CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
		TIMER_ALL_ACCESS
	);
	if (!hTimer) return; // if fail then continue waiting with QPC

	LARGE_INTEGER liTime = { 0 };
	liTime.QuadPart = 0 - (int64_t)time;
	SetWaitableTimer(hTimer, &liTime, 0, NULL, NULL, FALSE);

	WaitForSingleObject(hTimer, INFINITE);
	CloseHandle(hTimer);
	return;
}

/*
  time: Time to delay in microseconds
*/

void sleep64(uint64_t time) {

	uint64_t startTime = clock64();
	int64_t waitableTimerTime = (time * 10 / tickResolution - 1) * tickResolution;
	if (waitableTimerTime > 0)
		_sleep64_waitabletimer(waitableTimerTime);
	// if fail then continue waiting with QPC
	
	// Wait with QueryPerformanceCounter
	uint64_t qpcTarget = startTime + (time * (CLOCK_PER_SEC64 / 1000000));
	while (clock64() <= qpcTarget);

	// TODO: handle resolution change.
	return;
}
