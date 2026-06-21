#include "common.h"

int wmain()
{
    std::wstring localAppData = GetEnv(L"LOCALAPPDATA");
    if (localAppData.empty())
    {
        std::cerr << "Failed to get LOCALAPPDATA environment variable\n";
        return 1;
    }

    struct BrowserCache
    {
        std::wstring name;
        std::wstring path;
    };

    BrowserCache caches[] = {
        { L"Chrome", localAppData + L"\\Google\\Chrome\\User Data\\Default\\Cache" },
        { L"Edge",   localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\Cache" },
    };

    int overallResult = 0;
    for (auto& bc : caches)
    {
        std::cout << "Cleaning " << WToUtf8(bc.name) << " cache: " << WToUtf8(bc.path) << std::endl;
        int ret = CleanFolder(bc.path);
        if (ret != 0) overallResult = 1;
    }
    return overallResult;
}
