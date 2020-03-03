#pragma once

class SourceFile
{
protected:
    const std::filesystem::path mFileName;
    std::string mFileData;
	std::vector<std::string> mDependencies;

public:
    SourceFile(const std::filesystem::path& path);

    void Read();

    SourceFile(const SourceFile&) = delete;
    SourceFile& operator=(const SourceFile&) = delete;
	SourceFile(SourceFile&&) = default;
	SourceFile& operator=(SourceFile&&) = default;

	std::vector<std::string>& GetDependencies() { return mDependencies; }
	const std::filesystem::path& GetFilename() const;
};