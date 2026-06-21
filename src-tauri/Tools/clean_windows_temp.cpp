#include "common.h"

int wmain()
{
    std::cout << "Cleaning Windows temp files: C:\\Windows\\Temp\n";
    return CleanFolder(L"C:\\Windows\\Temp");
}
