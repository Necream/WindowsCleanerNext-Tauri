#include "common.h"

int wmain()
{
    std::wstring tempPath = GetEnv(L"TEMP");
    if (tempPath.empty())
    {
        std::cerr << "Failed to get TEMP environment variable\n";
        return 1;
    }
    std::cout << "Cleaning user temp files: " << WToUtf8(tempPath) << std::endl;
    return CleanFolder(tempPath);
}
