#ifndef ZIP_H
#define ZIP_H

#include <string>
#include <vector>

bool addFilesToZipFolder(const std::wstring& zipName,
                        std::vector<std::wstring> &filesToZip,
                        bool createNewArchive, // erase all archive content or update existing
                        void (*onStartZip)(const std::wstring&),
                        bool (*onZipError)(const std::wstring&),
                        int waitTicksMax = 10000);


#if 0
bool extractFilesFromZipFolder(const std::wstring &zipName,
                               const std::wstring &outPath,
                               int waitTicksMax = 10000);
#endif

#endif /* ZIP_H */
