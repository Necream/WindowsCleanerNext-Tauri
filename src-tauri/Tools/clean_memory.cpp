#include "common.h"

static std::wstring g_targetName = L"memreduct-cli.exe";
static std::wstring g_downloadPage = L"https://github.com/Necream/memreduct-cli/releases";

static std::string ReadPipe(HANDLE hPipe)
{
    std::string result;
    char buf[4096];
    DWORD bytesRead;
    while (ReadFile(hPipe, buf, sizeof(buf) - 1, &bytesRead, NULL) && bytesRead > 0)
    {
        buf[bytesRead] = '\0';
        result += buf;
    }
    return result;
}

static std::wstring GetSelfDir()
{
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    WCHAR* slash = wcsrchr(path, L'\\');
    if (slash) *slash = L'\0';
    return std::wstring(path);
}

int wmain()
{
    if (!IsAdmin())
    {
        std::cerr << "Administrator privileges required\n";
        return 1;
    }

    std::wstring selfDir = GetSelfDir();
    std::wstring cliPath = selfDir + L"\\" + g_targetName;

    if (GetFileAttributesW(cliPath.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        std::cout << "========================================" << std::endl;
        std::cout << WToUtf8(g_targetName) << " not found." << std::endl;
        std::cout << std::endl;
        std::cout << "Please download it and place in:" << std::endl;
        std::cout << "  " << WToUtf8(selfDir) << std::endl;
        std::cout << std::endl;
        std::cout << "Download page:" << std::endl;
        std::cout << "  " << WToUtf8(g_downloadPage) << std::endl;
        std::cout << "========================================" << std::endl;
        return 1;
    }

    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
    {
        std::cerr << "Failed to create pipe\n";
        return 1;
    }
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    PROCESS_INFORMATION pi = { 0 };
    std::wstring cmdLine = L"\"" + cliPath + L"\" -clean";

    if (!CreateProcessW(NULL, &cmdLine[0], NULL, NULL, TRUE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        std::cerr << "Failed to launch " << WToUtf8(g_targetName) << std::endl;
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return 1;
    }

    CloseHandle(hWrite);
    CloseHandle(pi.hThread);

    std::string output = ReadPipe(hRead);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(hRead);

    std::cout << output;
    return 0;
}
