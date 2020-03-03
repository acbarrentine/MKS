#include "PCH.h"
#include "SourceFile.h"
#include <regex>

SourceFile::SourceFile(const std::filesystem::path& path)
    : mFileName(path)
{
}
void SourceFile::Read()
{
    std::ifstream thefile(mFileName, std::ios::binary);
    mFileData = std::string((std::istreambuf_iterator<char>(thefile)), std::istreambuf_iterator<char>());

	std::regex re("import[[:space:]]*<([_[:alnum:]]+)\\.mko>");
	std::smatch match;
	if (std::regex_match(mFileData, match, re))
	{
		std::sub_match importFile = match[1];
		mDependencies.push_back(importFile.str());
	}
}

const std::filesystem::path& SourceFile::GetFilename() const
{
	return mFileName;
}
