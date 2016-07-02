#include "crashdump.h"

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#endif

#ifdef QT_VERSION
#include <qt_windows.h>
#else
#include <Windows.h>
#endif

#include "log.h"

#include <string.h>
#include <assert.h>
#include <tchar.h>
#include <DbgHelp.h>
#include <Shlobj.h>
#include <shtypes.h>
#include <TlHelp32.h>

#pragma comment(lib,"shell32")

typedef BOOL (WINAPI *PMiniDumpWriteDump)(HANDLE hProcess,
                                  DWORD ProcessId,
                                  HANDLE hFile,
                                  MINIDUMP_TYPE DumpType,
                                  PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                                  PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                                  PMINIDUMP_CALLBACK_INFORMATION CallbackParam);


void BrowseToFile(LPCTSTR filename)
{
  if (S_OK == CoInitializeEx(NULL, COINIT_MULTITHREADED))
  {
    ITEMIDLIST *pidl = ILCreateFromPath(filename);
    if(pidl)
    {
        SHOpenFolderAndSelectItems(pidl,0,0,0);
        ILFree(pidl);
    }

    CoUninitialize();
  }
}

void ShowMessage(const TCHAR* error, TCHAR* dumpFile)
{
  const int MAX_STR = 4096;

  const TCHAR title[] = _T("Application error");

  if (error)
  {
    TCHAR msgStr[MAX_STR];
    _tcscpy(msgStr, _T("Unfortunately application crashed and memory dump cannot be created.\n"));
    _tcscat(msgStr, error);
    MessageBox(NULL, msgStr, title, MB_OK | MB_ICONERROR);
  }
  else
  {
    TCHAR msgStr[MAX_STR];
    memset(msgStr, 0, MAX_STR * sizeof (TCHAR));

    const TCHAR msgPre[] = _T("Unfortunately application crashed and a dump file was created.\n\n")
                           _T("You can help to improve this product by submitting ")
                           _T("the dump file to application developers.\n\n")
                           _T("The dump file location is:\n\n");

    const TCHAR msgQuest[] = _T("\n\nWould you like to open a directory that contains the dump file?");

    _tcscpy_s(msgStr, MAX_STR, msgPre);
    int strLen = _tcslen(msgPre);
    _tcscat_s(msgStr + strLen, MAX_STR - strLen, dumpFile);
    strLen = _tcslen(msgStr);
    _tcscat_s(msgStr + strLen, MAX_STR - strLen, msgQuest);

    int res = MessageBox(NULL, msgStr, title,
                         MB_YESNO | MB_ICONERROR | MB_DEFBUTTON1 | MB_TASKMODAL | MB_TOPMOST);

    if (res == IDYES)
    {
      BrowseToFile(dumpFile);
    }
  }
}



struct DumperParams
{
  DumpInfoLevel level;
  CrashDumpHandler * dumper;

  TCHAR * dumpFilePath;

  DWORD dwWriterThreadId;
  _EXCEPTION_POINTERS* ExceptionInfo;
  DWORD dwExceptionThreadId;

  const TCHAR * error;
};


BOOL CALLBACK MinidumpFilter(PVOID CallbackParam,
                             const PMINIDUMP_CALLBACK_INPUT input,
                             PMINIDUMP_CALLBACK_OUTPUT output)
{
  DumperParams * p = (DumperParams*)(CallbackParam);

  switch (input->CallbackType)
  {
  case IncludeModuleCallback:
  case ThreadCallback:
  case ThreadExCallback:
    return TRUE;
  case CancelCallback:
    return FALSE;

  case IncludeThreadCallback:
  {
    // We don't want to include information about the minidump writing
    // thread, as that's not of interest to the caller
    if (input->IncludeThread.ThreadId == p->dwWriterThreadId)
    {
      return FALSE;
    }

    return TRUE;
  }
    break;

  case MemoryCallback:
  {
    // Small and medium sized dumps don't need full memory access
    if (kInfoLevelSmall == p->level ||
        kInfoLevelMedium == p->level)
    {
      return FALSE;
    }

    return TRUE;
  }
    break;

  case ModuleCallback:
  {
    if (kInfoLevelSmall == p->level)
    {
      // When creating a small dump file, we filter out any modules that
      // aren't being directly referenced.
      if (! (output->ModuleWriteFlags & ModuleReferencedByMemory))
      {
        output->ModuleWriteFlags &= ~ModuleWriteModule;
      }
    }
    else if (kInfoLevelMedium == p->level)
    {
      // When creating a medium-sized dump file, we filter out any module
      // data segments if they're not part of our core module list.  This
      // helps reduce the size of the dump file by quite a bit.
      if (output->ModuleWriteFlags & ModuleWriteDataSeg)
      {
        if (! p->dumper->isDataSegmentNeeded(input->Module.FullPath))
        {
          output->ModuleWriteFlags &= ~ModuleWriteDataSeg;
        }
      }
    }

    return TRUE;
  }
    break;
  }

  return TRUE;
}

// writes the minidump
DWORD WINAPI MinidumpWriter(DumperParams * p)
{
  // detect dump type
  int type = MiniDumpNormal;
  switch (p->level)
  {
  case kInfoLevelSmall:
  {
    type |= MiniDumpWithIndirectlyReferencedMemory |
            MiniDumpScanMemory;
  }
    break;
  case kInfoLevelMedium:
  {
    type |= MiniDumpWithDataSegs |
            MiniDumpWithPrivateReadWriteMemory |
            MiniDumpWithHandleData |
            MiniDumpWithFullMemoryInfo |
            MiniDumpWithThreadInfo |
            MiniDumpWithUnloadedModules;
  }
    break;
  case kInfoLevelLarge:
  {
    type |= MiniDumpWithDataSegs |
            MiniDumpWithPrivateReadWriteMemory |
            MiniDumpWithHandleData |
            MiniDumpWithFullMemory |
            MiniDumpWithFullMemoryInfo |
            MiniDumpWithThreadInfo |
            MiniDumpWithUnloadedModules;
            MiniDumpWithProcessThreadData;
  }
    break;
  }

  do
  {
    TCHAR appPath[_MAX_PATH];
    TCHAR appDir[_MAX_PATH];
    TCHAR appName[_MAX_PATH];

    HMODULE hDbgHelp = NULL;

    if ((GetModuleFileName(NULL, appPath, _MAX_PATH) == 0) || (GetLastError() != ERROR_SUCCESS))
    {
      p->error = _T("Call to GetModuleFileName failed!");
      break;
    }

    // setup app paths
    {
      TCHAR *slash = _tcsrchr(appPath, '\\');
      assert(slash);
      _tcscpy(appName, slash + 1);
      _tcsncpy(appDir, appPath, slash - appPath);
      TCHAR nul = 0;
      appDir[slash - appPath] = nul;
    }

    // load DbgHelp.dll
    {
      TCHAR dbgHelpPath[_MAX_PATH];
      _tcscpy(dbgHelpPath, appDir);
      _tcscat(dbgHelpPath, _T("\\DBGHELP.DLL"));
      hDbgHelp = LoadLibrary(dbgHelpPath);
    }

    if (! hDbgHelp)
    {
      p->error = _T("Cannot load DbgHelp.dll!");
      break;
    }

    PMiniDumpWriteDump pMiniDumpWriteDump = (PMiniDumpWriteDump)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
    if (! pMiniDumpWriteDump)
    {
      p->error = _T("Cannot locate MiniDumpWriteDump inside DbgHelp.dll!");
      break;
    }

    MINIDUMP_EXCEPTION_INFORMATION  M;
    HANDLE  hDumpFile;

    M.ThreadId = p->dwExceptionThreadId;
    M.ExceptionPointers = p->ExceptionInfo;
    M.ClientPointers = FALSE;

    _tcscpy(p->dumpFilePath, appPath);
    _tcscpy(p->dumpFilePath + _tcslen(p->dumpFilePath) - 3, _T("dmp"));

    hDumpFile = CreateFile(p->dumpFilePath, GENERIC_WRITE, 0,
                           NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    MINIDUMP_CALLBACK_INFORMATION callback = { 0 };
    p->dwWriterThreadId = GetCurrentThreadId();
    callback.CallbackParam = p;
    callback.CallbackRoutine = MinidumpFilter;

    pMiniDumpWriteDump(GetCurrentProcess(),
                       GetCurrentProcessId(),
                       hDumpFile,
                       (MINIDUMP_TYPE)type,
                       p->ExceptionInfo ? &M : NULL,
                       NULL,
                       &callback);

    CloseHandle(hDumpFile);
  }
  while(0);

  // disable system error dialog displaying
  SetErrorMode(SEM_NOGPFAULTERRORBOX);

  return S_OK;
}


BOOL WINAPI EnumerateThreads(DWORD (WINAPI *FuncThread)(HANDLE hThread), DWORD dwThreadExcludeId)
{
  // enumerate current process
  DWORD dwOwnerPID = 0;
  DWORD dwThreadCurrentId = GetCurrentThreadId();

  HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
  THREADENTRY32 te32;

  // Take a snapshot of all running threads
  hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (hThreadSnap == INVALID_HANDLE_VALUE)
  {
    return FALSE;
  }

  // Fill in the size of the structure before using it.
  te32.dwSize = sizeof(THREADENTRY32);

  // Retrieve information about the first thread,
  // and exit if unsuccessful
  if (! Thread32First(hThreadSnap, &te32))
  {
    // Must clean up the snapshot object!
    CloseHandle(hThreadSnap);
    return FALSE;
  }

  // calling specified function on each thread
  do
  {
    if (te32.th32OwnerProcessID == dwOwnerPID)
    {
      if (te32.th32ThreadID == dwThreadExcludeId)
      {
        // do not process passed id
        continue;
      }

      if (te32.th32ThreadID == dwThreadCurrentId)
      {
        // do not process current thread id
        continue;
      }

      HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
      // if thread was opened
      if (hThread)
      {
        FuncThread(hThread);
        CloseHandle(hThread);
      }
    }
  }
  while(Thread32Next(hThreadSnap, &te32));

  // release the snapshot object
  CloseHandle(hThreadSnap);

  return TRUE;
}

DWORD WINAPI MinidumpInitiator(LPVOID lpParameter)
{
  DumperParams * p = (DumperParams*)(lpParameter);

  // suspend other threads but minidump writer
  EnumerateThreads(SuspendThread, GetCurrentThreadId());

  // write the dump
  MinidumpWriter(p);

  // prepare the dump archive
  p->dumper->dumpCreated(p->dumpFilePath);

  // in case of stack overflow in exception thread the message can be shown in separate thread
  ShowMessage(p->error, p->dumpFilePath);

  // resume other threads
  EnumerateThreads(ResumeThread, GetCurrentThreadId());

  return S_OK;
}


// worker thread
HANDLE g_hThreadMinidump = NULL;
DumperParams g_dumperParams;

LONG WINAPI UnhExceptFilter(_EXCEPTION_POINTERS *ExceptionInfo)
{
  g_dumperParams.ExceptionInfo = ExceptionInfo;
  g_dumperParams.dwExceptionThreadId = GetCurrentThreadId();

  // run minidump writer thread
  ResumeThread(g_hThreadMinidump);
  // wait for minidump to be written
  WaitForSingleObject(g_hThreadMinidump, INFINITE);

  // kill the process
  return EXCEPTION_EXECUTE_HANDLER;
}

LPTOP_LEVEL_EXCEPTION_FILTER g_oldUnhandledExceptionFilter = NULL;

CrashDumpHandler::CrashDumpHandler(DumpInfoLevel level)
{
  g_dumperParams.level = level;
  g_dumperParams.error = NULL;
  g_dumperParams.dumper = this;
  g_dumperParams.dumpFilePath = (TCHAR*)malloc(_MAX_PATH * sizeof(TCHAR));

  g_hThreadMinidump = CreateThread(NULL, 0, MinidumpInitiator, &g_dumperParams, CREATE_SUSPENDED, NULL);
  if (g_hThreadMinidump)
  {
    g_oldUnhandledExceptionFilter = SetUnhandledExceptionFilter(&UnhExceptFilter);
  }
  else
  {
    LOGEX(LOG_VERBOSE_CRITICAL, _T("Cannot initialize exception handler thread. Minidump will not be created!"));
  }
}

CrashDumpHandler::~CrashDumpHandler()
{
  // return original exception handler
  SetUnhandledExceptionFilter(g_oldUnhandledExceptionFilter);

  // release thread
  CloseHandle(g_hThreadMinidump);

  // release file path
  free(g_dumperParams.dumpFilePath);
}





