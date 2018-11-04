#include "PCH.h"
#include "SourceFile.h"

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

    const std::filesystem::path sourceDir = std::filesystem::path(argv[1]).lexically_normal();
    std::cout << "Scanning " << sourceDir << std::endl;

    try
    {
        for(const auto& entry : std::filesystem::recursive_directory_iterator(sourceDir))
        {
            if (entry.is_regular_file())
            {
                const std::filesystem::path path = entry.path();
                if (path.extension() == ".mks")
                {
                    SourceFile source(path.lexically_normal());
                    source.Echo();
                }
            }
        }
    }
    catch (std::filesystem::filesystem_error& err)
    {
        std::cerr << "Error " << err.code() << ": " << err.what() << std::endl;
    }
 
    return 0;
}
