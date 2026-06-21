#include "common.h"

int wmain()
{
    if (!IsAdmin())
    {
        std::cerr << "Administrator privileges required\n";
        return 1;
    }

    std::cout << "Cleaning system restore points...\n";
    int ret = _wsystem(L"vssadmin Delete Shadows /all /quiet");
    if (ret == 0)
    {
        std::cout << "System restore points cleaned successfully\n";
        return 0;
    }
    else
    {
        std::cerr << "Failed to clean system restore points, error code: " << ret << std::endl;
        return 1;
    }
}
