// A Windows-based Singleton log class.
//
// Copyright Evan Beeton 2/1/2005

#include "PI_Logger.h"

PI_Logger *PI_Logger::mp_instance = 0;

// Singleton instance accessor. Will Init the object if not done already.
// Call Shutdown() when done logging.
PI_Logger &PI_Logger::GetInstance(void)
{
	if (!mp_instance)
		mp_instance = new PI_Logger;
	return *mp_instance;
}


bool PI_Logger::Init(const char *filename)
{
	// Is the logfile already open?
	if (fout.is_open())
		return false;

	// Attempt to open the desired logfile.
	m_filename = filename;
	fout.open(m_filename.c_str());
	if (!fout.is_open())
		return false;

	// Raise an exception if something bad happens.
	//fout.exceptions(ios::eofbit | ios::badbit | ios::failbit);

	// Write a standard greeting.
	fout << m_filename << " opened ";
	DateStamp(true);
	fout << '\t';
	TimeStamp(true);
	fout << '\n';
	InsertSeparator();
	fout << '\n';

	return true;
}

void PI_Logger::Shutdown(void)
{
	// Write a standard closing.
	fout << '\n';
	InsertSeparator();
	fout << m_filename << " closed ";
	DateStamp(true);
	fout << '\t';
	TimeStamp(true);

	fout.close();

	// Free the singleton.
	delete mp_instance;
	mp_instance = 0;
}

void PI_Logger::TimeStamp(bool showSeconds)
{
	// Always show the hour and minute.
	GetLocalTime(&m_time);
	fout <<	setfill('0') << setw(2) << m_time.wHour << ':'
		 << setw(2) << m_time.wMinute;

	// Optionally show seconds.
	if (showSeconds)
		fout << ':' << setw(2) << m_time.wSecond;
	fout << setfill(' ');
}

void PI_Logger::DateStamp(bool showDayName)
{
	static const char *days[] = {
		"Sunday",
		"Monday",
		"Tuesday",
		"Wednesday",
		"Thursday",
		"Friday",
		"Saturday",
	};

	GetLocalTime(&m_time);

	if (showDayName)
		fout << days[m_time.wDayOfWeek] << ' ';

	// M/D/YYYY format.
	fout << m_time.wMonth << '/' << m_time.wDay << '/' << m_time.wYear;
}

void PI_Logger::InsertSeparator(unsigned short width, char fillchar)
{
	for (unsigned short i = 0; i < width; ++i)
		fout << fillchar;
	fout << '\n';
}