#include "common.h"

void WalkPackages(const std::wstring& packagesDir)
{
    std::wstring searchPath = packagesDir + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do
    {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
            continue;

        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;

        std::wstring pkgPath = packagesDir + L"\\" + fd.cFileName;

        std::wstring subSearch = pkgPath + L"\\*";
        WIN32_FIND_DATAW subFd;
        HANDLE hSub = FindFirstFileW(subSearch.c_str(), &subFd);
        if (hSub == INVALID_HANDLE_VALUE) continue;

        do
        {
            if (wcscmp(subFd.cFileName, L".") == 0 || wcscmp(subFd.cFileName, L"..") == 0)
                continue;

            if (!(subFd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue;

            std::wstring dirName = subFd.cFileName;
            ToLower(dirName);
            if (dirName.find(L"cache") != std::wstring::npos)
            {
                std::wstring cachePath = pkgPath + L"\\" + subFd.cFileName;
                std::cout << "Deleting cache: " << WToUtf8(cachePath) << std::endl;
                DeleteDirectoryContents(cachePath);
                RemoveDirectoryW(cachePath.c_str());
            }
        } while (FindNextFileW(hSub, &subFd));
        FindClose(hSub);
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

    std::wstring localAppData = GetEnv(L"LOCALAPPDATA");
    if (localAppData.empty())
    {
        std::cerr << "Failed to get LOCALAPPDATA\n";
        return 1;
    }

    std::wstring packagesDir = localAppData + L"\\Packages";
    std::cout << "Scanning app cache: " << WToUtf8(packagesDir) << std::endl;
    WalkPackages(packagesDir);
    std::cout << "App cache cleanup complete\n";
    return 0;
}
