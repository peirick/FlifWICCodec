#pragma once

#include <windows.h>

typedef LARGE_INTEGER Stopwatch;

static inline double StopwatchReadAndReset(Stopwatch* watch) {
	const LARGE_INTEGER old_value = *watch;
	LARGE_INTEGER freq;
	if (!QueryPerformanceCounter(watch))
		return 0.0;
	if (!QueryPerformanceFrequency(&freq))
		return 0.0;
	if (freq.QuadPart == 0)
		return 0.0;
	return (watch->QuadPart - old_value.QuadPart) / (double)freq.QuadPart;
}
