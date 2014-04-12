#include <sys/time.h>

#include <ctime>
#include <iostream>
#include <fstream>
#include <locale>
#include <stdlib.h>
#include <unistd.h>
#include <wchar.h>

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

#endif // LOG_USE_PTHREADS

// maximum log line length in characters
#define MAX_LOG_MESSAGE_CHARACTERS 4096

// filename with full path
std::string logFilePath;

// automatically flush log on each message
bool logAutoFlush = false;

// accumulates log messages between log flushing
std::wstring logCapacitor;

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

typedef std::codecvt<wchar_t , char , mbstate_t> null_wcodecvt_base;
class null_wcodecvt : public null_wcodecvt_base
{
public:
  explicit null_wcodecvt() : null_wcodecvt_base(++m_refs)
  {
  }

  ~null_wcodecvt()
  {
    --m_refs;
  }

protected:
    result do_out(mbstate_t&,
                  const wchar_t* from,
                  const wchar_t* from_end,
                  const wchar_t*& from_next,
                  char* to,
                  char* to_limit,
                  char*& to_next) const
    {
        size_t len = (from_end - from) * sizeof(wchar_t);
        size_t lenDest = (to_limit - to);
        len = len <= lenDest ? len : lenDest;
        memcpy(to, from, len);
        from_next = from_end;
        to_next = to + len;
        return ok;
    }

    result do_in(mbstate_t&,
                 const char* from,
                 const char* from_end,
                 const char*& from_next,
                 wchar_t* to,
                 wchar_t* to_limit,
                 wchar_t*& to_next) const
    {
        size_t len = (from_end - from);
        size_t lenDest = (to_limit - to) * sizeof(wchar_t);
        len = len <= lenDest ? len : lenDest;
        memcpy(to, from, len);
        from_next = from_end;
        to_next = to + (len / sizeof(wchar_t));
        return ok;
    }

    result do_unshift(mbstate_t&, char* to, char*, char*& to_next) const
    {
        to_next = to;
        return noconv;
    }

    int do_length(mbstate_t&, const char* from, const char* end, size_t max) const
    {
        return (int)((max < (size_t)(end - from)) ? max : (end - from));
    }

    bool do_always_noconv() const throw()
    {
        return false;
    }

    int do_encoding() const throw()
    {
        return sizeof(wchar_t);
    }

    int do_max_length() const throw()
    {
        return sizeof(wchar_t);
    }

    static size_t m_refs;
};

size_t null_wcodecvt::m_refs = 0;

#ifdef F_OS_WINDOWS
std::wostream& wendl(std::wostream& out)
{
  out.put(L'\r');
  out.put(L'\n');
  out.flush();
  return out;
}
#else
std::wostream& wendl(std::wostream& out)
{
  out.put(L'\n');
  out.flush();
  return out;
}
#endif // F_OS_WINDOWS

#include <sys/stat.h>
// Function: fileExists
/**
    Check if a file exists
@param[in] filename - the name of the file to check

@return    true if the file exists, else false

*/
bool fileExists(const std::string& filename)
{
  struct stat buf;
  if (stat(filename.c_str(), &buf) != -1)
  {
    return true;
  }
  return false;
}

// function controls log output
void LogRaw(const std::wstring &str)
{
  if (!str.size())
  {
    return;
  }

  // if there is no filename set we will write log to standard output
  if (logFilePath.empty())
  {
    fwrite((char*)str.c_str(), str.size() * sizeof(wchar_t), 1, stdout);
    fflush(stdout);
  }
  else
  {
    std::wofstream output;
    null_wcodecvt wcodec;
    std::locale wloc(std::locale::classic(), &wcodec);
    output.imbue(wloc);

    bool needProlog = false;
    if (!fileExists(logFilePath))
    {
      needProlog = true;
    }

    output.open(logFilePath, std::ios_base::binary | std::ios_base::out | std::ios_base::app);

    if (output.fail())
    {
        wprintf(L"Error opening log file for logging: %s\n", logFilePath.c_str());
        return;
    }

    if (needProlog)
    {
      const wchar_t UTF_BOM = 0xfeff;
      output << UTF_BOM;
    }

    output << str;

    output.close();
  }
}

// send content of logCapacitor to output and clears it
void LogFlush()
{
    LogRaw(logCapacitor);
    logCapacitor.clear();
}

// function controls log formatting
void WriteLogEx(LOG_VERBOSE verbose, const wchar_t * position, const wchar_t * message)
{
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
  std::wstring verboseStr;
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

    int messageLen = 0;

    if (verbose == LOG_VERBOSE_RAW)
    {
      std::wstring formatStrPrivate = L"%ls";
      swprintf(messageOut, MAX_LOG_MESSAGE_CHARACTERS, formatStrPrivate.c_str(), message);
    }
    else
    {
      time_t t = time(0);
      struct tm * now = localtime(&t);

      struct timeval tv;
      gettimeofday(&tv, NULL);

      std::wstring formatStrPrivate = L"%04d-%02d-%02d %02d:%02d:%02d.%03d @F@ %ls %ls%ls";

      messageLen = swprintf(messageOut, MAX_LOG_MESSAGE_CHARACTERS, formatStrPrivate.c_str(),
                            now->tm_year + 1900,
                            now->tm_mon + 1,
                            now->tm_mday,
                            now->tm_hour,
                            now->tm_min,
                            now->tm_sec,
                            tv.tv_usec / 1000,
                            position,
                            verboseStr.c_str(),
                            message);
    }

    if (messageLen < 0)
    {
      std::wstring errMsg;

      if (errno == EILSEQ)
      {
        errMsg = L"Illegal multibyte character sequence";
      }

      wprintf(L"Message formatting error occured: \"%s\". @: %s:%d\n", errMsg.c_str(), WIDEN(__FILE__), __LINE__);
      perror(NULL);

      return;
    }

    std::wstringstream _str;
    _str << messageOut;
    wendl(_str);

    if (logAutoFlush)
    {
      LogRaw(_str.str());
    }
    else
    {
      logCapacitor.append(_str.str());
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

