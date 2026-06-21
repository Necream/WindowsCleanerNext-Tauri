#include "common.h"

bool IsTargetFile(const std::wstring& name)
{
    size_t dot = name.find_last_of(L'.');
    if (dot == std::wstring::npos) return false;
    std::wstring ext = name.substr(dot);
    ToLower(ext);
    return ext == L".tmp" || ext == L".cache" || ext == L".msp";
}

void WalkAndDelete(const std::wstring& dirPath)
{
    std::wstring searchPath = dirPath + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do
    {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
            continue;

        std::wstring fullPath = dirPath + L"\\" + fd.cFileName;

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            WalkAndDelete(fullPath);
        }
        else
        {
            if (IsTargetFile(fd.cFileName))
            {
                if (DeleteFileW(fullPath.c_str()))
                {
                    g_fileCount++;
                    std::cout << "Deleted: " << WToUtf8(fullPath) << std::endl;
                }
                else
                {
                    g_errorCount++;
                }
            }
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
}

int wmain()
{
    if (!IsAdmin())
    {
        std::cerr << "Administrator privileges required\n";
        return 1;
    }

    g_fileCount = 0;
    g_errorCount = 0;

    std::cout << "Scanning .tmp / .cache / .msp files in C:\\...\n";
    WalkAndDelete(L"C:\\");
    std::cout << "Done. Deleted " << g_fileCount << " file(s), "
               << g_errorCount << " error(s)\n";
    return g_errorCount > 0 ? 1 : 0;
}
