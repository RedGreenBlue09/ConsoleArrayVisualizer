
#include <Windows.h>
#include <stdint.h>

uint64_t clock64() {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
}

NTSYSAPI NTSTATUS NTAPI NtQueryTimerResolution(
	OUT PULONG MinimumResolution,
	OUT PULONG MaximumResolution,
	OUT PULONG CurrentResolution
);

static ULONG _MinTickRes = 0;
static uint64_t _ClockRes = 0;

static void _sleep64_waitabletimer(int64_t Time) {

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

	if (_ClockRes == 0) {
		LARGE_INTEGER li;
		QueryPerformanceFrequency(&li);
		_ClockRes = li.QuadPart;
	}

	uint64_t StartTime = clock64();
	
	if (_MinTickRes == 0) {
		ULONG Unused, Unused2;
		NtQueryTimerResolution(
			&_MinTickRes,
			&Unused,
			&Unused2
		);
	}
	
	int64_t WaitableTimerTime = ((int64_t)time * 10 / (int64_t)_MinTickRes - 1) * (int64_t)_MinTickRes;
	if (WaitableTimerTime > 0)
		_sleep64_waitabletimer(WaitableTimerTime);
	
	// FIXME: Potential overflow
	uint64_t TargetClockTime = StartTime + (time * _ClockRes / 1000000);
	while (clock64() < TargetClockTime);

	return;
}
