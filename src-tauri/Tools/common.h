#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <iostream>

struct InitUtf8Console {
    InitUtf8Console() {
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    }
};
static InitUtf8Console _initUtf8;

inline std::string WToUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
    std::string result(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &result[0], len, NULL, NULL);
    return result;
}

inline bool IsAdmin()
{
    BOOL elevated = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        TOKEN_ELEVATION te;
        DWORD size = sizeof(te);
        if (GetTokenInformation(hToken, TokenElevation, &te, size, &size))
            elevated = te.TokenIsElevated;
        CloseHandle(hToken);
    }
    return elevated;
}

static int g_fileCount = 0;
static int g_dirCount = 0;
static int g_errorCount = 0;

inline void DeleteDirectoryContents(const std::wstring& dirPath)
{
    std::wstring searchPath = dirPath + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
            continue;

        std::wstring fullPath = dirPath + L"\\" + fd.cFileName;

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            DeleteDirectoryContents(fullPath);
            if (RemoveDirectoryW(fullPath.c_str()))
                g_dirCount++;
            else
                g_errorCount++;
        }
        else
        {
            if (DeleteFileW(fullPath.c_str()))
                g_fileCount++;
            else
                g_errorCount++;
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
}

inline int CleanFolder(const std::wstring& folderPath)
{
    if (!IsAdmin())
    {
        std::cerr << "Administrator privileges required\n";
        return 1;
    }

    DWORD attrs = GetFileAttributesW(folderPath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES)
    {
        std::cout << "Path does not exist: " << WToUtf8(folderPath) << std::endl;
        return 1;
    }

    g_fileCount = 0;
    g_dirCount = 0;
    g_errorCount = 0;

    DeleteDirectoryContents(folderPath);

    std::cout << "Deleted " << g_fileCount << " files, " << g_dirCount
              << " directories, " << g_errorCount << " errors" << std::endl;
    return g_errorCount > 0 ? 1 : 0;
}

inline std::wstring GetEnv(const wchar_t* var)
{
    wchar_t buf[4096];
    DWORD ret = GetEnvironmentVariableW(var, buf, 4096);
    if (ret == 0 || ret > 4096) return L"";
    return std::wstring(buf);
}

inline void ToLower(std::wstring& s)
{
    for (auto& c : s) c = towlower(c);
}
