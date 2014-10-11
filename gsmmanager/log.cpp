
#ifdef _MSC_VER
// MSVC compiler
#include <winsock2.h>
#include <time.h>
#else
// GCC/MinGW
#include <sys/time.h>
#endif

#include <ctime>
#include <iostream>
#include <fstream>
#include <locale>
#include <stdlib.h>
//#include <unistd.h>
#include <wchar.h>

#include "log.h"


// line endings
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__WINDOWS__)
#define ENDL "\r\n"
#define WENDL L"\r\n"
std::wostream& wendl(std::wostream& out)
{
  out.put(L'\r');
  out.put(L'\n');
  out.flush();
  return out;
}
#else
#define ENDL "\n"
#define WENDL L"\n"
std::wostream& wendl(std::wostream& out)
{
  out.put(L'\n');
  out.flush();
  return out;
}
#endif // F_OS_WINDOWS



#ifdef LOG_MULTITHREADING
// multithreading support

bool mutexInitialized = false;

#ifdef _MSC_VER
// MSVC
#if (WINVER < 0x0801)
// XP,2003,Vista,2008,7
#include <WinBase.h>
#else
// 8
#include <Synchapi.h>
#endif

HANDLE logMutex;

inline void mutexInit()
{
  logMutex = CreateMutex(NULL, FALSE, NULL);

  if (!logMutex)
  {
    printf("CreateMutex error: %d" ENDL, GetLastError());
  }

  mutexInitialized = logMutex != NULL;
}

inline void mutexDeinit()
{
  CloseHandle(logMutex);
  mutexInitialized = false;
}

inline bool mutexLock()
{
  if (!mutexInitialized)
  {
    return false;
  }

  DWORD result = WaitForSingleObject(logMutex, INFINITE);
  if (result != WAIT_OBJECT_0)
  {
    printf("Cannot lock mutex! Logging message skipped! File: %s, Line: %s" ENDL, __FILE__, TOSTRING(__LINE__));
    return false;
  }

  return true;
}

inline void mutexUnlock()
{
  ReleaseMutex(logMutex);
}

#else // #ifdef _MSC_VER

// GCC / MinGW
#include <pthread.h>

pthread_mutex_t logMutex;


inline void mutexInit()
{
  int code = pthread_mutex_init(&logMutex, NULL);
  if (code)
  {
    printf("Error initializing pthread_mutex_t! pthread_mutex_init code: %d. Terminating..." ENDL, code);
  }

  mutexInitialized = code == 0;
}

inline void mutexDeinit()
{
  int code = pthread_mutex_destroy(&logMutex);
  if(code)
  {
    printf("Error deinitializing pthread_mutex_t! pthread_mutex_destroy code: %d" ENDL, code);
  }

  mutexInitialized = false;
}

inline bool mutexLock()
{
  if (!mutexInitialized)
  {
    return false;
  }

  int mutexResult = pthread_mutex_lock(&logMutex);
  if (mutexResult)
  {
    printf("Cannot lock pthread log mutex! Logging message skipped! File: %s, Line: %s" ENDL, __FILE__, STRINGIFY(__LINE__));
    return false;
  }

  return true;
}

inline void mutexUnlock()
{
  int mutexResult = pthread_mutex_unlock(&logMutex);
  if (mutexResult)
  {
    printf("Cannot unlock pthread log mutex! This is the problem! File: %s, Line: %s" ENDL, __FILE__, STRINGIFY(__LINE__));
  }
}

#endif // #ifdef _MSC_VER
#endif // LOG_MULTITHREADING

bool startLogging()
{
    mutexInit();
    return mutexInitialized;
}

void stopLogging()
{
    mutexDeinit();
}


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
    null_wcodecvt wcodec;
    std::locale wloc(std::locale::classic(), &wcodec);
    std::wofstream output;
    output.imbue(wloc);

    bool needProlog = false;
    if (!fileExists(logFilePath))
    {
      needProlog = true;
    }

    output.open(logFilePath, std::ios_base::binary | std::ios_base::out | std::ios_base::app);

    if (output.fail())
    {
        wprintf(L"Error opening log file for logging: %s" WENDL, logFilePath.c_str());
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
#ifdef LOG_MULTITHREADING
  if (!mutexLock())
  {
    return;
  }
#endif

  // if would like to receive only high valuable messages...
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

      long usec = 0;

#ifdef _MSC_VER
// MSVC compiler
      const __int64 DELTA_EPOCH_IN_MICROSECS= 11644473600000000;

      FILETIME ft;
      __int64 tmpres = 0;
      ZeroMemory(&ft,sizeof(ft));
      GetSystemTimeAsFileTime(&ft);

      tmpres = ft.dwHighDateTime;
      tmpres <<= 32;
      tmpres |= ft.dwLowDateTime;

      /*converting file time to unix epoch*/
      tmpres /= 10;  /*convert into microseconds*/
      tmpres -= DELTA_EPOCH_IN_MICROSECS;

      //sec = (__int32)(tmpres*0.000001);
      usec =(tmpres%1000000);
#else
// GCC / MinGW
      struct timeval tv;
      gettimeofday(&tv, NULL);
      usec = tv.tv_usec / 1000;
#endif

      std::wstring formatStrPrivate = L"%04d-%02d-%02d %02d:%02d:%02d.%03d %ls %ls%ls";

      messageLen = swprintf(messageOut, MAX_LOG_MESSAGE_CHARACTERS, formatStrPrivate.c_str(),
                            now->tm_year + 1900,
                            now->tm_mon + 1,
                            now->tm_mday,
                            now->tm_hour,
                            now->tm_min,
                            now->tm_sec,
                            usec,
                            verboseStr.c_str(),
                            position,
                            message);
    }

    if (messageLen < 0)
    {
      std::wstring errMsg;

      if (errno == EILSEQ)
      {
        errMsg = L"Illegal multibyte character sequence";
      }

      wprintf(L"Message formatting error occured: \"%s\". @: %s:%d" WENDL, errMsg.c_str(), WIDEN(__FILE__), __LINE__);
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

#ifdef LOG_MULTITHREADING
  mutexUnlock();
#endif

}

