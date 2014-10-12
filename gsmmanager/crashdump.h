#ifndef CRASHDUMP_H
#define CRASHDUMP_H

#include <list>
#include <string>

enum DumpInfoLevel
{
  kInfoLevelSmall,
  kInfoLevelMedium,
  kInfoLevelLarge
};

class CrashDumpHandler
{
public:
  CrashDumpHandler(DumpInfoLevel level = kInfoLevelMedium);
  ~CrashDumpHandler();

  void setDataSegmentsNeeded(const std::list<std::wstring>& modules);

  bool isDataSegmentNeeded(const wchar_t* filePath) const;

  void dumpCreated(wchar_t* filePath);

  void addToArchiveList(const std::wstring& fileName);

private:

  std::list<std::wstring> m_modulesNeeded;

  std::list<std::wstring> m_archiveAddon;

};

#endif // CRASHDUMP_H
