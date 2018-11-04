#include "PCH.h"
#include "SourceFile.h"

SourceFile::SourceFile(const std::filesystem::path& path)
    : mFileName(path)
{
    std::ifstream thefile(path, std::ios::binary);
    mFileData = std::string((std::istreambuf_iterator<char>(thefile)), std::istreambuf_iterator<char>());
}

void SourceFile::Echo() const
{
    std::cout << mFileName << std::endl;
    std::cout << "\t" << mFileData << std::endl;
}