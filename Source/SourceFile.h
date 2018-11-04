#pragma once

class SourceFile
{
protected:
    const std::filesystem::path mFileName;
    std::string mFileData;

public:
    SourceFile(const std::filesystem::path& path);

    void Echo() const;
};