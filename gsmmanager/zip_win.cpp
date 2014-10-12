#include "zip.h"

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#endif

#include <windows.h>
#include <ole2.h>
#include <ShlDisp.h>
#include <tchar.h>

#include <algorithm>
#include <iterator>
#include <set>


#pragma comment(lib,"ole32")
#pragma comment(lib,"oleaut32")


#include <windows.h>
#include <Tlhelp32.h>

#include <set>
#include <iostream>

class CToolhelp32Snapshot
{
    HANDLE hSnapshot;

public:

/* DWORD dwFlags
TH32CS_INHERIT Indicates that the snapshot handle is to be inheritable.
TH32CS_SNAPALL Includes all processes and threads in the system, plus the heaps and modules of the process specified in th32ProcessID. Equivalent to specifying the TH32CS_SNAPHEAPLIST, TH32CS_SNAPMODULE, TH32CS_SNAPPROCESS, and TH32CS_SNAPTHREAD values.
TH32CS_SNAPHEAPLIST Includes all heaps of the process specified in th32ProcessID in the snapshot. To enumerate the heaps, see Heap32ListFirst.
TH32CS_SNAPMODULE Includes all modules of the process specified in th32ProcessID in the snapshot. To enumerate the modules, see Module32First.
TH32CS_SNAPPROCESS Includes all processes in the system in the snapshot. To enumerate the processes, see Process32First.
TH32CS_SNAPTHREAD Includes all threads in the system in the snapshot. To enumerate the threads, see Thread32First.
*/
    CToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID = 0) : hSnapshot(0)
       {
        hSnapshot = ::CreateToolhelp32Snapshot(dwFlags, th32ProcessID);
       }

    ~CToolhelp32Snapshot()
       {
        ::CloseHandle(hSnapshot);
       }

    BOOL ThreadFirst( LPTHREADENTRY32 lpte )
       {
        return ::Thread32First(hSnapshot, lpte);
       }

    BOOL ThreadNext( LPTHREADENTRY32 lpte )
       {
        return ::Thread32Next(hSnapshot, lpte);
       }
};

inline
bool
getProcessThreads( std::set<DWORD> &threadIds, DWORD pid = 0)
{
    if (!pid) pid = ::GetCurrentProcessId();
    CToolhelp32Snapshot snapshot( TH32CS_SNAPTHREAD, pid );
    THREADENTRY32 thEntry = { sizeof(thEntry) };
    if (!snapshot.ThreadFirst(&thEntry)) return false;
    do {
        if ( thEntry.th32OwnerProcessID == pid)
           threadIds.insert( thEntry.th32ThreadID );
       } while(snapshot.ThreadNext(&thEntry));
    return true;
}

template < typename InIter1, typename InIter2, typename OutIter>
OutIter setDifference( InIter1 b1, InIter1 e1, InIter2 b2, InIter2 e2, OutIter out)
   {
    for(; b1 != e1 && b2 != e2; )
       {
        if (*b1==*b2)
           {
            b1++; b2++;
           }
        else if (*b1<*b2)
           {
            *++out = *b1; ++b1;
           }
        else
           {
            *++out = *b2; ++b2;
           }
       }
    return out;
   }

inline
void printThreadSet( const std::set<DWORD> &threadIds )
   {
    std::set<DWORD>::const_iterator tidIt = threadIds.begin();
    for(; tidIt != threadIds.end(); ++tidIt)
      {
       std::cout<<"Id: "<< *tidIt<<"\n";
      }
   }



/*
inline

typedef struct tagTHREADENTRY32 {  DWORD dwSize;  DWORD cntUsage;  DWORD th32ThreadID;  DWORD th32OwnerProcessID;  LONG tpBasePri;  LONG tpDeltaPri;  DWORD dwFlags;
} THREADENTRY32, *PTHREADENTRY32;
*/



inline
void getCurrentDirectory(::std::string &dir)
{
  char  ch = 0;
  char *buf = &ch;
  DWORD size = ::GetCurrentDirectoryA(1, buf);
  if (!size) { dir = ::std::string(); return; }
  buf = (char*)_alloca(size*sizeof(char));
  ::GetCurrentDirectoryA(size, buf);
  dir = ::std::string(buf);
}

//--------------------------------------------------------------------
inline
void getCurrentDirectory(::std::wstring &dir)
{
  wchar_t  ch = 0;
  wchar_t *buf = &ch;
  DWORD size = ::GetCurrentDirectoryW(1, buf);
  if (!size) { dir = ::std::wstring(); return; }
  buf = (wchar_t*)_alloca(size*sizeof(wchar_t));
  ::GetCurrentDirectoryW(size, buf);
  dir = ::std::wstring(buf);
}



bool addFilesToZipFolder(const std::wstring& zipName,
                         std::vector<std::wstring>& filesToZip,
                         bool createNewArchive,
                         void (*onStartZip)(const std::wstring&),
                         bool (*onZipError)(const std::wstring&),
                         int waitTicksMax)
{
  // first, create new archive if not exist, or clear previous
  {
    DWORD dwCreationDisposition = CREATE_NEW; // if file exist, don't touch it
    if (createNewArchive)
    {
      dwCreationDisposition = CREATE_ALWAYS; // overwrite existing archive
    }

    HANDLE hZipFile = CreateFile( zipName.c_str(), GENERIC_WRITE, 0, 0
                                  , CREATE_NEW // we do not need open existing archive
                                  , FILE_ATTRIBUTE_NORMAL, 0 );

    if (hZipFile != INVALID_HANDLE_VALUE)
    {
      char sig[22] = { 'P', 'K', 0x05, 0x06, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
      DWORD written = 0;
      WriteFile( hZipFile, &sig[0], 22, &written, 0 );
      CloseHandle(hZipFile);
    }
  }

  waitTicksMax /= 100;
  if (waitTicksMax < 30)
  {
    waitTicksMax = 30; // 3 sec
  }

  // here we are ready to put files to archive
  OleInitialize(NULL);

  IShellDispatch *pSD = NULL;

  HRESULT hres = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_SERVER,IID_IShellDispatch, (LPVOID *)&pSD);
  if (SUCCEEDED(hres)) // CoCreateInstance pSD
  {
    std::wstring zipFullName = zipName;
    std::wstring::size_type spos = zipFullName.find_first_of( _T("\\/") );
    if (spos == ::std::string::npos)
    {
      std::wstring dir;
      getCurrentDirectory(dir);
      zipFullName = dir + std::wstring(_T("\\")) + zipFullName;
    }

    VARIANT vZipDir;
    VariantInit(&vZipDir);
    vZipDir.vt = VT_BSTR;
    int zipDirLen = (lstrlen(zipFullName.c_str()) + 1) * sizeof(WCHAR);
    BSTR bstrZip = SysAllocStringByteLen(NULL,zipDirLen);

#ifndef UNICODE
    MultiByteToWideChar(CP_ACP,0,zipFullName.c_str(),-1,bstrZip,zipDirLen);
#else
    lstrcpy(bstrZip, zipFullName.c_str());
#endif

    vZipDir.bstrVal = bstrZip;

    Folder* pZipFolder = NULL;
    hres = pSD->NameSpace(vZipDir, &pZipFolder);

    if (SUCCEEDED(hres))
    {
      VARIANT vOptions;
      VariantInit(&vOptions);
      vOptions.vt=VT_I4;
      vOptions.lVal=4+16+1024+4096; //0;
      //4 Do not display a progress dialog box.
      //8 Give the file being operated on a new name in a move, copy, or rename operation if a file with the target name already exists.
      //16 Respond with "Yes to All" for any dialog box that is displayed.
      //64 Preserve undo information, if possible.
      //128 Perform the operation on files only if a wildcard file name (*.*) is specified.
      //256 Display a progress dialog box but do not show the file names.
      //512 Do not confirm the creation of a new directory if the operation requires one to be created.
      //1024 Do not display a user interface if an error occurs.
      //2048 Version 4.71. Do not copy the security attributes of the file.
      //4096 Only operate in the local directory. Don't operate recursively into subdirectories.
      //9182 Version 5.0. Do not copy connected files as a group. Only copy the specified files.

      std::vector<std::wstring>::const_iterator fzIt = filesToZip.begin();
      bool fStop = false;
      for(; !fStop && fzIt != filesToZip.end(); ++fzIt)
      {
        std::wstring fullName = *fzIt;
        std::wstring::size_type spos = fullName.find_first_of(_T("\\/"));
        if (spos == std::wstring::npos)
        {
          std::wstring dir;
          getCurrentDirectory(dir);
          fullName = dir + std::wstring(_T("\\")) + fullName;
        }

        VARIANT vFileNameToZip;
        VariantInit(&vFileNameToZip);
        vFileNameToZip.vt = VT_BSTR;
        int fnLen = (lstrlen(fullName.c_str()) + 1) * sizeof(WCHAR);
        BSTR bstrFileToZip = SysAllocStringByteLen(NULL,fnLen);

#ifndef UNICODE
        MultiByteToWideChar(CP_ACP,0,fullName.c_str(),-1,bstrFileToZip,fnLen);
#else
        lstrcpy(bstrFileToZip, fullName.c_str());
#endif
        vFileNameToZip.bstrVal = bstrFileToZip;

        // pZipFolder->CopyHere internally creates thread and we can get it handle for wait on it,
        // so, we detect changes in current process threads

        if (onStartZip)
        {
          onStartZip(*fzIt);
        }

        std::set<DWORD> tidsBefore;
        getProcessThreads(tidsBefore);

        hres = pZipFolder->CopyHere(vFileNameToZip,vOptions);

        DWORD curLim = waitTicksMax;
        do
        {
          ::Sleep(100);
          std::set<DWORD> tidsAfter;
          getProcessThreads(tidsAfter);

          std::set<DWORD> tidsDiff;
          setDifference( tidsBefore.begin(),
                         tidsBefore.end(),
                         tidsAfter.begin(),
                         tidsAfter.end(),
                         std::inserter(tidsDiff, tidsDiff.end()));

          if (tidsDiff.empty())
          {
            curLim = 0;
          }
          else
          {
            curLim--;
          }
        }
        while(curLim);

        if (! SUCCEEDED(hres))
        {
          if (onZipError && (! onZipError(*fzIt)))
          {
            fStop = true;
          }
        }

        SysFreeString(bstrFileToZip);
        VariantClear(&vFileNameToZip);
      } // for(; !fStop && fzIt != filesToZip.end(); ++fzIt)

      pZipFolder->Release();
      VariantClear(&vOptions);
      SysFreeString(bstrZip);
      VariantClear(&vZipDir);

    } // if (SUCCEEDED(pSD->NameSpace(vZipDir,&pZipFolder)))

    pSD->Release();
  } // if (SUCCEEDED(hres)) // CoCreateInstance pSD

  OleUninitialize( );
  return hres == S_OK;
}

#if 0
bool extractFilesFromZipFolder(const std::wstring& zipName,
                               const std::wstring& outPath,
                               int waitTicksMax)
{
  waitTicksMax /= 100;
  if (waitTicksMax < 30)
  {
    waitTicksMax = 30; // 3 sec
  }

  // here we are ready to put files to archive
  OleInitialize(NULL);

  IShellDispatch *pSD = NULL;

  HRESULT hres = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_SERVER,IID_IShellDispatch, (LPVOID *)&pSD);
  if (SUCCEEDED(hres)) // CoCreateInstance pSD
  {
    VARIANT vZipDir;
    VariantInit(&vZipDir);
    vZipDir.vt = VT_BSTR;
    int zipDirLen = (lstrlen(zipName.c_str()) + 1) * sizeof(WCHAR);
    BSTR bstrZip = SysAllocStringByteLen(NULL,zipDirLen);

#ifndef UNICODE
    MultiByteToWideChar(CP_ACP,0,zipName.c_str(),-1,bstrZip,zipDirLen);
#else
    lstrcpy(bstrZip, zipName.c_str());
#endif

    vZipDir.bstrVal = bstrZip;

    Folder* pZipFolder = NULL;
    if (SUCCEEDED(pSD->NameSpace(vZipDir,&pZipFolder)))
    {
      VARIANT vOptions;
      VariantInit(&vOptions);
      vOptions.vt=VT_I4;
      vOptions.lVal=4+16+1024+4096; //0;

      pZipFolder->Release();
      VariantClear(&vOptions);
      SysFreeString(bstrZip);
      VariantClear(&vZipDir);

    } // if (SUCCEEDED(pSD->NameSpace(vZipDir,&pZipFolder)))

    pSD->Release();
  } // if (SUCCEEDED(hres)) // CoCreateInstance pSD

  OleUninitialize( );
  return hres == S_OK;
}
#endif

