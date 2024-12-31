
#include <Windows.h>
#include <stdint.h>

static uint64_t ClockRes = 0;

uint64_t clock64() {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
}

uint64_t clock64_resolution() {
	if (ClockRes == 0) {
		LARGE_INTEGER li;
		QueryPerformanceFrequency(&li);
		ClockRes = li.QuadPart;
	}
	return ClockRes;
}

NTSYSAPI NTSTATUS NTAPI NtQueryTimerResolution(
	OUT PULONG MinimumResolution,
	OUT PULONG MaximumResolution,
	OUT PULONG CurrentResolution
);

static ULONG MinTickRes = 0;

static void waitabletimer(int64_t Time) {

	HANDLE hTimer = CreateWaitableTimerW(NULL, TRUE, NULL);
	if (!hTimer) return;

	LARGE_INTEGER liTime;
	liTime.QuadPart = -Time;
	SetWaitableTimer(hTimer, &liTime, 0, NULL, NULL, FALSE);

	WaitForSingleObject(hTimer, INFINITE);
	CloseHandle(hTimer);
	return;

}

/*
  time: Time to delay in microseconds
*/

void sleep64(uint64_t time) {

	uint64_t StartTime = clock64();

	if (ClockRes == 0) {
		LARGE_INTEGER li;
		QueryPerformanceFrequency(&li);
		ClockRes = li.QuadPart;
	}
	
	if (MinTickRes == 0) {
		ULONG Unused, Unused2;
		NtQueryTimerResolution(
			&MinTickRes,
			&Unused,
			&Unused2
		);
	}
	
	int64_t WaitableTimerTime = ((int64_t)time * 10 / (int64_t)MinTickRes - 1) * (int64_t)MinTickRes;
	if (WaitableTimerTime > 0)
		waitabletimer(WaitableTimerTime);
	
	// FIXME: Potential overflow
	uint64_t TargetClockTime = StartTime + (time * ClockRes / 1000000);
	while (clock64() < TargetClockTime);

	return;
}
