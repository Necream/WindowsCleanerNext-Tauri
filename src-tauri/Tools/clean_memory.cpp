#include "common.h"
#include <shellapi.h>

int wmain(int argc, wchar_t* argv[])
{
    if (!IsAdmin())
    {
        std::cerr << "Administrator privileges required\n";
        return 1;
    }

    if (argc < 2)
    {
        std::cerr << "Usage: clean_memory.exe <app_root_path>\n";
        std::cerr << "Example: clean_memory.exe C:\\Program Files\\WindowsCleanerNext\n";
        return 1;
    }

    std::wstring memreductPath = std::wstring(argv[1]) + L"\\WCMain\\memreduct.exe";

    DWORD attrs = GetFileAttributesW(memreductPath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES)
    {
        std::cerr << "Cannot find memreduct.exe: " << WToUtf8(memreductPath) << std::endl;
        return 1;
    }

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = memreductPath.c_str();
    sei.lpParameters = L"/clean /silent";
    sei.nShow = SW_HIDE;

    if (!ShellExecuteExW(&sei))
    {
        std::cerr << "Failed to launch memreduct.exe\n";
        return 1;
    }

    std::cout << "memreduct.exe launched, waiting 3 seconds...\n";
    Sleep(3000);

    _wsystem(L"taskkill /f /im memreduct.exe");
    std::cout << "Memory cleanup complete\n";
    return 0;
}
