// Timer.h - Interface for a simple timer.
//
// Copyright Evan Beeton 6/4/2005

#include "Timer.h"

// Start the timer.
void Timer::Start(void)
{
	start_time = clock();
}

// Get the elapsed time since the timer was started,
// and reset it.
float Timer::ElapsedSecondsReset(void)
{
	float elapsed = float(clock() - start_time) / CLOCKS_PER_SEC;
	start_time = clock();
	return elapsed;
}

// Get the elapsed time since the timer was started.
float Timer::ElapsedSeconds(void) const
{
	return float(clock() - start_time) / CLOCKS_PER_SEC;
}