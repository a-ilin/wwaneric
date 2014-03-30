﻿//
// To enable logging put this into CPP/H files:
// #define LOG_ENABLED
// #include "log.h"
//

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

int startLogging();
int stopLogging();

void WriteLogEx(LOG_VERBOSE verbose, const wchar_t * position, const wchar_t * message);
void WriteLog(const wchar_t * message);

#define LOGEX(VERBOSITY,MESSAGEFMT) \
        { \
            std::wstringstream _msg; \
            _msg << L"_FILE_: " WIDEN(__FILE__) L", _LINE_: " WIDEN(TOSTRING(__LINE__)) L", _FUNCTION_: " \
                << __FUNCTION__ \
                << L":"; \
            WriteLogEx(VERBOSITY, _msg.str().data(), MESSAGEFMT); \
        }

#define LOG(MESSAGEFMT) LOGEX(LOG_VERBOSE_RAW, MESSAGEFMT)

// Qt
#ifdef QT_VERSION
// Q_MESSAGEFMT is a QString object
#define Q_LOGEX(VERBOSITY,Q_MESSAGEFMT) \
  { \
    wchar_t * array = new wchar_t[Q_MESSAGEFMT.size() + 1]; \
    int size = Q_MESSAGEFMT.toWCharArray(array); \
    array[size] = 0; \
    LOGEX(VERBOSITY,array); \
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