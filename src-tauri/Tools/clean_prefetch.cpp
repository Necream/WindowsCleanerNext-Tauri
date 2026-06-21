#include "common.h"

int wmain()
{
    std::cout << "Cleaning prefetch files: C:\\Windows\\Prefetch\n";
    return CleanFolder(L"C:\\Windows\\Prefetch");
}
