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

    std::filesystem::path sourceDir(argv[1]);
    sourceDir.make_preferred();
    std::cout << "Scanning " << sourceDir << "\n";

    try
    {
        for(const auto& entry : std::filesystem::recursive_directory_iterator(sourceDir))
        {
            if (entry.is_regular_file())
            {
                std::filesystem::path path = entry.path();
                if (path.extension() == ".mks")
                {
                    path.make_preferred();
                    std::cout << path << '\n';
                }
            }
        }
    }
    catch (std::filesystem::filesystem_error& err)
    {
        std::cerr << "Error " << err.code() << ": " << err.what() << "\n";
    }
 
    return 0;
}
