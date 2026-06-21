#include "common.h"

int wmain()
{
    std::cout << "Cleaning Windows update cache: C:\\Windows\\SoftwareDistribution\\Download\n";
    return CleanFolder(L"C:\\Windows\\SoftwareDistribution\\Download");
}
