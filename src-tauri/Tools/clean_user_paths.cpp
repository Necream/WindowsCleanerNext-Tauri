#include "common.h"

int wmain(int argc, wchar_t* argv[])
{
    if (!IsAdmin())
    {
        std::cerr << "Administrator privileges required\n";
        return 1;
    }

    if (argc < 2)
    {
        std::cerr << "Usage: clean_user_paths.exe <path1> [path2] ...\n";
        return 1;
    }

    int overallResult = 0;
    for (int i = 1; i < argc; i++)
    {
        std::wstring path = argv[i];
        std::cout << "Cleaning user-specified path: " << WToUtf8(path) << std::endl;
        int ret = CleanFolder(path);
        if (ret != 0) overallResult = 1;
    }
    return overallResult;
}
