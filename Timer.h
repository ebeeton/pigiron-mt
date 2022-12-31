// Timer.h - Interface for a simple timer.
//
// Copyright Evan Beeton 6/4/2005

#include <ctime>

class Timer
{
	// What's the starting time?
	clock_t start_time;

public:

	Timer(void) : start_time(0) { }
	~Timer() { }

	// Start the timer.
	void Start(void);

	// Get the elapsed time since the timer was started,
	// and reset it.
	float ElapsedSecondsReset(void);

	// Get the elapsed time since the timer was started.
	float ElapsedSeconds(void) const;
};