#include "common.h"

int wmain()
{
    std::wstring logPath = GetEnv(L"SystemRoot") + L"\\Logs";
    std::cout << "Cleaning system logs: " << WToUtf8(logPath) << std::endl;
    return CleanFolder(logPath);
}
