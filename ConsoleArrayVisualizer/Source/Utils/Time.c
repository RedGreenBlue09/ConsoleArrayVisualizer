
#include <Windows.h>
#include <stdint.h>

uint64_t clock64() {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
}

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

	uint64_t StartTime = clock64();
	int64_t WaitableTimerTime = ((int64_t)time / 15625 - 1) * 15625 * 10;
	if (WaitableTimerTime > 0)
		_sleep64_waitabletimer(WaitableTimerTime);
	
	uint64_t TargetClockTime = StartTime + time;
	while (clock64() < TargetClockTime);

	return;
}
