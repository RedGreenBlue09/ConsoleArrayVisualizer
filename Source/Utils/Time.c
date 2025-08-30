
#include <Windows.h>
#include <stdint.h>

#include "Utils/IntMath.h"

static uint64_t ClockRes = 0; // Fixme: Multi-thread

uint64_t clock64() {
	LARGE_INTEGER TimeStruct;
	QueryPerformanceCounter(&TimeStruct);
	return TimeStruct.QuadPart;
}

uint64_t clock64_resolution() {
	if (ClockRes == 0) {
		LARGE_INTEGER TimeStruct;
		QueryPerformanceFrequency(&TimeStruct);
		ClockRes = TimeStruct.QuadPart;
	}
	return ClockRes;
}

NTSYSAPI NTSTATUS NTAPI NtQueryTimerResolution(
	OUT PULONG MinimumResolution,
	OUT PULONG MaximumResolution,
	OUT PULONG CurrentResolution
);

static ULONG TimerResPeriod = 0;

static void WaitableTimerSleep(int64_t Duration) {
	if (Duration <= 0)
		return; // Avoid overhead
	HANDLE hTimer = CreateWaitableTimerW(NULL, TRUE, NULL);
	if (!hTimer)
		return;

	LARGE_INTEGER DurationStruct = { .QuadPart = -Duration };
	SetWaitableTimer(hTimer, &DurationStruct, 0, NULL, NULL, FALSE);

	WaitForSingleObject(hTimer, INFINITE);
	CloseHandle(hTimer);
	return;
}

// Implementation assumes the clock resolution is at least
// better than the waitable timer resolution.
void sleep64(uint64_t Duration) {
	if (Duration == 0)
		return; // Avoid calculation overhead

	uint64_t StartClockTime = clock64();

	if (ClockRes == 0) {
		LARGE_INTEGER li;
		QueryPerformanceFrequency(&li);
		ClockRes = li.QuadPart;
	}
	
	if (TimerResPeriod == 0) {
		ULONG Unused;
		ULONG Unused2;
		NtQueryTimerResolution(&TimerResPeriod, &Unused, &Unused2);
	}

	int64_t TimerDuration;
	uint64_t Rem;
	if (ClockRes == 10000000) // Skip exepensive division.
		TimerDuration = (int64_t)Duration;
	else
		TimerDuration = (int64_t)div_u64(Duration * 10000000, ClockRes, &Rem);

	// Most of the times, waitable timer will sleep more than
	// the specified time by 1 TimerResPeriod or less.
	WaitableTimerSleep(TimerDuration - TimerResPeriod);
	
	uint64_t TargetClockTime = StartClockTime + Duration;
	while (clock64() < TargetClockTime);

	return;
}
