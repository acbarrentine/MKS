#include <iostream>
#include <filesystem>

void Usage()
{
    std::cout << "Usage: MKS <sourceDir>\n";
}

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        Usage();
        return -1;
    }

    const char* sourceDir = argv[1];
    std::cout << "Scanning " << sourceDir << "\n";

    for(const auto& entry : std::filesystem::recursive_directory_iterator(sourceDir))
    {
        const std::filesystem::path& path = entry.path();
        std::cout << path << '\n';
    }

    return 0;
}
