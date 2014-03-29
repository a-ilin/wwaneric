#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <wchar.h>
#include <ctime>

#include "log.h"
#include "common.h"


#ifdef LOG_USE_PTHREADS
#include <pthread.h>

pthread_mutex_t logMutex;

bool mutexInitialized = false;

int startLogging()
{
    int mutexResult = 0;

    if (!mutexInitialized)
    {
        mutexResult = pthread_mutex_init(&logMutex, NULL);

        if (!mutexResult)
        {
            mutexInitialized = true;
        }
    }

    return mutexResult;
}

int stopLogging()
{
    int mutexResult = 0;

    if (mutexInitialized)
    {
        mutexResult = pthread_mutex_destroy(&logMutex);

        if (!mutexResult)
        {
            mutexInitialized = false;
        }
    }

    return mutexResult;
}

#endif

// maximum log line length in characters
#define MAX_LOG_MESSAGE_CHARACTERS 4096

// filename with full path
std::string logFilePath;

// automatically flush log on each message
bool logAutoFlush = false;

// accumulates log messages between log flushing
std::string logCapacitor;

// filter
LOG_VERBOSE currentVerbosity = LOG_VERBOSE_LAST;

// recognizes argument as a verbosity level and returns it
LOG_VERBOSE validateVerbosityFromString(std::string verbosity)
{
    if (verbosity == std::string("DEBUG"))
    {
        return LOG_VERBOSE_DEBUG;
    }

    if (verbosity == std::string("NOTIFICATION"))
    {
        return LOG_VERBOSE_NOTIFICATION;
    }

    if (verbosity == std::string("WARNING"))
    {
        return LOG_VERBOSE_WARNING;
    }

    if (verbosity == std::string("ERROR"))
    {
        return LOG_VERBOSE_ERROR;
    }

    if (verbosity == std::string("CRITICAL"))
    {
        return LOG_VERBOSE_CRITICAL;
    }

    // RAW shouldn't be recognized as a verbosity level. It used only for internal purposes.

    return LOG_VERBOSE_LAST;
}

// function controls log output
void LogRaw(const char * message, int count)
{
    // if the length is zero then exit
    if (!count)
    {
        return;
    }

    // if there is no filename set we will write log to standard output
    if (logFilePath.empty())
    {
        //fwrite(message, count, 1, stdout);
        //fflush(stdout);

        printf(message);
    }
    else
    {
        std::ofstream output;
        output.open(logFilePath, std::ios_base::out | std::ios_base::app);

        if (output.fail())
        {
            wprintf(L"Error opening log file for logging: %s\n", logFilePath.c_str());
            return;
        }

        output.write(message, count);

        output.close();
    }
}

// send content of logCapacitor to output and clears it
void LogFlush()
{
    LogRaw(logCapacitor.c_str(), logCapacitor.size());
    logCapacitor.clear();
}

// function controls log formatting
void WriteLogEx(LOG_VERBOSE verbose, const wchar_t * position, const wchar_t * message)
{
    std::wstring verboseStr;

#ifdef LOG_USE_PTHREADS

    int mutexResult;

    if (!mutexInitialized)
    {
        return;
    }

    mutexResult = pthread_mutex_lock(&logMutex);
    if (mutexResult)
    {
        wprintf(L"Cannot lock pthread log mutex! Logging message skipped! File: %s, Line: %s\n", WIDEN(__FILE__), WIDEN(STRINGIFY(__LINE__)));
        return;
    }
#endif

    // if we are like to receive only high valuable messages...
    if (verbose >= currentVerbosity)
    {
      switch (verbose)
      {
      case LOG_VERBOSE_RAW:
          break;
      case LOG_VERBOSE_DEBUG:
          verboseStr = L"DEBUG: ";
          break;
      case LOG_VERBOSE_NOTIFICATION:
          verboseStr = L"NOTIFICATION: ";
          break;
      case LOG_VERBOSE_WARNING:
          verboseStr = L"WARNING: ";
          break;
      case LOG_VERBOSE_ERROR:
          verboseStr = L"ERROR: ";
          break;
      case LOG_VERBOSE_CRITICAL:
          verboseStr = L"CRITICAL: ";
          break;
      case LOG_VERBOSE_LAST:
          verboseStr = L"DO NOT USE LOG_VERBOSE_LAST!!! ";
          break;
      }

      wchar_t messageOut[MAX_LOG_MESSAGE_CHARACTERS];

      if (verbose == LOG_VERBOSE_RAW)
      {
          std::wstring formatStrPrivate = L"%ls\n";
          swprintf(messageOut, MAX_LOG_MESSAGE_CHARACTERS, formatStrPrivate.c_str(), message);
      }
      else
      {
          time_t t = time(0);   // get time now
          struct tm * now = localtime(&t);

          std::wstring formatStrPrivate = L"%04d-%02d-%02d %02d:%02d:%02d @:%ls: %ls%ls\n";

          swprintf(messageOut, MAX_LOG_MESSAGE_CHARACTERS, formatStrPrivate.c_str(),
                                  now->tm_year + 1900,
                                  now->tm_mon + 1,
                                  now->tm_mday,
                                  now->tm_hour,
                                  now->tm_min,
                                  now->tm_sec,
                                  position,
                                  verboseStr.c_str(),
                                  message);
      }


      // convert to 8-bit characters
      char messageOutRaw[MAX_LOG_MESSAGE_CHARACTERS * sizeof(wchar_t)];

      size_t count = wcstombs(messageOutRaw, messageOut, MAX_LOG_MESSAGE_CHARACTERS * sizeof(wchar_t));

      if (count < wcslen(messageOut) )
      {
          wprintf(L"We miss some characters while logging the message!\n");
      }

      if (logAutoFlush)
      {
          LogRaw(messageOutRaw, count);
      }
      else
      {
          logCapacitor.append(messageOutRaw, count);
      }
    }

#ifdef LOG_USE_PTHREADS
    mutexResult = pthread_mutex_unlock(&logMutex);
    if (mutexResult)
    {
        wprintf(L"Cannot unlock pthread log mutex! This is the problem! File: %s, Line: %s\n", WIDEN(__FILE__), WIDEN(STRINGIFY(__LINE__)));
        return;
    }
#endif

}

