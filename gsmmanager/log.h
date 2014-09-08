﻿/*
* To enable logging define value LOG_ENABLED
*
* If you need to log from different threads/processes
* then also define this value: LOG_MULTITHREADING
*
* At start point call startLogging() and just before exit call stopLogging()
*
* To log messages use LOG, LOGEX, or Q_LOGEX macros.
*
*/


#pragma once

#include <string>
#include <iostream>
#include <sstream>

typedef enum LOG_VERBOSE
{
    LOG_VERBOSE_RAW,            // no classificator printed in RAW verbose mode
    LOG_VERBOSE_DEBUG,
    LOG_VERBOSE_NOTIFICATION,
    LOG_VERBOSE_WARNING,
    LOG_VERBOSE_ERROR,
    LOG_VERBOSE_CRITICAL,
    LOG_VERBOSE_LAST = -1       // for inner use by logging system
} LOG_VERBOSE;

// path to log file.
// when not empty logging writes to this file
// otherwise to standard output
extern std::string logFilePath;

// all messages that have status below this will be omitted
extern LOG_VERBOSE currentVerbosity;

// if set to true the flushing will be made automatically
extern bool logAutoFlush;

// flushing content of log capacitor string
extern void LogFlush();

// recognize argument as a string describes some verbosity level.
// if cannot recognize a verbosity level returns the highest verbosity level (LOG_VERBOSE_LAST)
LOG_VERBOSE validateVerbosityFromString(std::string verbosity);

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#ifdef LOG_ENABLED

bool startLogging();
void stopLogging();

void WriteLogEx(LOG_VERBOSE verbose, const wchar_t * position, const wchar_t * message);
void WriteLog(const wchar_t * message);

#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif

#define LOGEX(VERBOSITY,MESSAGEFMT) \
        { \
            std::wstringstream _msg; \
            _msg << L"MSGID:$" << WIDEN(TOSTRING(__COUNTER__)) << L"$ " \
                 << WIDEN(__FILE__) << L":" << WIDEN(TOSTRING(__LINE__)) \
                 << L" @ " << __PRETTY_FUNCTION__ << L": "; \
            WriteLogEx(VERBOSITY, _msg.str().data(), MESSAGEFMT); \
        }

#define LOG(MESSAGEFMT) LOGEX(LOG_VERBOSE_RAW, MESSAGEFMT)

// Qt
#ifdef QT_VERSION
// Q_MESSAGEFMT can be an object that accepted in QString constructor
#define Q_LOGEX(VERBOSITY,Q_MESSAGEFMT) \
  { \
    QString _str(Q_MESSAGEFMT); \
    wchar_t * _array = new wchar_t[_str.size() + 1]; \
    int _size = _str.toWCharArray(_array); \
    _array[_size] = 0; \
    LOGEX(VERBOSITY, _array); \
    delete[] _array; \
  }
#endif // QT_VERSION

#else

#define LOGEX do {} while(0);

#define LOG LOGEX

// Qt
#ifdef QT_VERSION
#define Q_LOGEX LOGEX
#endif

#endif // LOG_ENABLED
