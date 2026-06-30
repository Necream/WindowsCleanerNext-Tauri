#include "common.h"
#include <shlobj.h>

static bool NvidiaAppInstalled()
{
    const wchar_t* checkPaths[] = {
        L"C:\\Program Files\\NVIDIA Corporation\\NVIDIA App\\CEF\\NVIDIA Overlay.exe",
        L"C:\\Program Files\\NVIDIA Corporation\\NVIDIA app\\CEF\\NVIDIA Overlay.exe",
        nullptr
    };
    for (int i = 0; checkPaths[i]; i++)
    {
        if (GetFileAttributesW(checkPaths[i]) != INVALID_FILE_ATTRIBUTES)
            return true;
    }
    return false;
}

int wmain()
{
    if (!IsAdmin())
    {
        std::cerr << "Administrator privileges required\n";
        return 1;
    }

    if (!NvidiaAppInstalled())
    {
        std::cout << "NVIDIA driver not detected\n";
        return 1;
    }

    std::cout << "Detected NVIDIA driver installed\n";

    int totalFiles = 0;
    int totalDirs = 0;
    int totalErrors = 0;

    WCHAR appDataPath[MAX_PATH];
    std::wstring localAppData;
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath)))
        localAppData = appDataPath;

    const wchar_t* paths[] = {
        L"\\NVIDIA Corporation\\GfeSDK\\Logs",
        L"\\NVIDIA Corporation\\NVIDIA App\\Logs",
        L"\\NVIDIA Corporation\\NVIDIA app\\Logs",
        L"\\NVIDIA\\GLCache",
        L"\\NVIDIA\\DXCache",
        L"\\NVIDIA Corporation\\NVIDIA App\\ShadowPlay\\Logs",
        nullptr
    };

    for (int i = 0; paths[i] != nullptr; i++)
    {
        if (localAppData.empty()) break;

        std::wstring p = localAppData + paths[i];
        DWORD attrs = GetFileAttributesW(p.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) continue;

        g_fileCount = 0;
        g_dirCount = 0;
        g_errorCount = 0;

        DeleteDirectoryContents(p);
        RemoveDirectoryW(p.c_str());

        totalFiles += g_fileCount;
        totalDirs += g_dirCount;
        totalErrors += g_errorCount;
    }

    std::cout << "Deleted " << totalFiles << " files, "
              << totalDirs << " directories, "
              << totalErrors << " errors" << std::endl;

    return totalErrors > 0 ? 1 : 0;
}
