/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2017 by Aleksei Ilin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include "crashdump.h"

#include "common.h"
#include "log.h"
#include "zip.h"

#include <algorithm>
#include <stdio.h>

bool CrashDumpHandler::isDataSegmentNeeded(const wchar_t* filePath) const
{
  if (! m_modulesNeeded.size())
  {
    // if modules not specified apply all
    return true;
  }

  std::wstring fileStr(filePath);

#ifdef F_OS_WINDOWS
  std::replace(fileStr.begin(), fileStr.end(), L'\\', L'/');
#endif

  // remove directories
  std::wstring fileName;
  size_t slash = fileStr.rfind(L'/');
  if (slash != std::wstring::npos)
  {
    fileName = (fileStr.substr(slash + 1));
  }
  else
  {
    fileName = fileStr;
  }

  // remove extension
  std::wstring moduleName;
  size_t dot = fileName.rfind(L'.');
  if (dot != std::wstring::npos)
  {
    moduleName = fileName.substr(0, dot);
  }
  else
  {
    moduleName = fileName;
  }

#ifdef F_OS_WINDOWS
  // on Windows convert to upper case
  std::transform(moduleName.begin(), moduleName.end(), moduleName.begin(), ::toupper);
#endif

  bool needed = std::find(m_modulesNeeded.begin(), m_modulesNeeded.end(), moduleName)
                != m_modulesNeeded.end();

  if (needed)
  LOGEX(LOG_VERBOSE_NOTIFICATION, moduleName.c_str());
  return needed;
}

#include <Windows.h>
void CrashDumpHandler::dumpCreated(wchar_t* filePath)
{
  // create mini log file
  std::wstring miniLog;
  {
    std::wstring logFilePath = g_logFilePath;
#ifdef F_OS_WINDOWS
    std::replace(logFilePath.begin(), logFilePath.end(), L'/', L'\\');
#endif

    miniLog = logFilePath.substr(0, logFilePath.size() - 4) + L"_mini.log";
    if (fileExists(miniLog))
    {
      _wremove(miniLog.c_str());
    }

    int bufSize = 200 * 1024;
    int logSize = fileSize(logFilePath);
    if (logSize < bufSize)
    {
      bufSize = logSize;
    }

    FILE * fMini = _wfopen(miniLog.c_str(), L"wb+");
    // UTF prolog
    fputc(0xFF, fMini);
    fputc(0xFE, fMini);

    FILE * fLog = _wfopen(logFilePath.c_str(), L"rb");

    fseek(fLog, -bufSize, SEEK_END);

    char * buf = (char*)malloc(bufSize);
    fread(buf, bufSize, 1, fLog);
    fwrite(buf, bufSize, 1, fMini);
    free(buf);
    fclose(fMini);
    fclose(fLog);
  }

  // files to be added to ZIP
  std::vector<std::wstring> files;
  // dump file
  files.push_back(filePath);
  // mini log file
  files.push_back(miniLog);

  // addon
  if (m_archiveAddon.size())
  {
    std::copy(m_archiveAddon.begin(), m_archiveAddon.end(), std::back_inserter(files));
  }

  // run ZIP
  std::wstring zipName(filePath);
  zipName += L".zip";
  // remove stale ZIP file
  _wremove(zipName.c_str());
  int waitMSec = 60000;
  bool result = addFilesToZipFolder(zipName, files, true, NULL, NULL, waitMSec);

  // remove mini log
  _wremove(miniLog.c_str());

  if (result)
  {
    // remove original file dump
    _wremove(filePath);
    wcsncpy(filePath, zipName.c_str(), zipName.size() + 1);
  }
  else
  {
    LOGEX(LOG_VERBOSE_CRITICAL, L"Zip cannot be created!");
  }
}

void CrashDumpHandler::addToArchiveList(const std::wstring& fileName)
{
  std::wstring name = fileName;

#ifdef F_OS_WINDOWS
    std::replace(name.begin(), name.end(), L'/', L'\\');
#endif

  m_archiveAddon.push_back(name);
}

void CrashDumpHandler::setDataSegmentsNeeded(const std::list<std::wstring>& modules)
{
  m_modulesNeeded = modules;

#ifdef F_OS_WINDOWS
  // on Windows convert to upper case
  std::list<std::wstring>::iterator iter = m_modulesNeeded.begin();
  std::list<std::wstring>::iterator iterEnd = m_modulesNeeded.end();
  while (iter != iterEnd)
  {
    std::wstring& module = *iter;
    std::transform(module.begin(), module.end(), module.begin(), ::toupper);
    ++iter;
  }
#endif
}


