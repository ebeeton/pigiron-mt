// A Windows-based Singleton log class.
//
// Copyright Evan Beeton 2/1/2005

#pragma once

// Needed for time/date stamps.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Standard C++ I/O stuff.
#include <fstream>
using std::ofstream;
using std::ios;
#include <iomanip>
using std::setw;
using std::setfill;
#include <string>
using std::string;

class PI_Logger
{
	// This class is a Singleton.
	static PI_Logger *mp_instance;
	PI_Logger(const PI_Logger &rhs);
	PI_Logger &operator=(const PI_Logger &rhs);
	PI_Logger(void) { Init(); }

	ofstream fout;
	SYSTEMTIME m_time;
	string m_filename;

	// Internal initialization function.
	bool Init(const char *filename = "log.txt");

public:
	
	// Singleton instance accessor. Will Init the object if not done already.
	// Call Shutdown() when done logging.
	static PI_Logger &GetInstance(void);

	void Shutdown(void);

	// Handy stamp and separator functions.
	void TimeStamp(bool showSeconds = false);
	void DateStamp(bool showDayName = false);
	void InsertSeparator(unsigned short width = 80, char fillchar = '-');

	// All the insertion overloads - allows "logfile << type;"
#define INSERTION_OVERLOAD(type) PI_Logger &operator<<(type t) { fout << t; return *mp_instance; }
	INSERTION_OVERLOAD(string)
	INSERTION_OVERLOAD(const char *)
	INSERTION_OVERLOAD(char)
	INSERTION_OVERLOAD(int)
	INSERTION_OVERLOAD(unsigned int)
	INSERTION_OVERLOAD(float)
	INSERTION_OVERLOAD(double)
};