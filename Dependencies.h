#pragma once
// https://pubs.opengroup.org/onlinepubs/000095399/functions/realpath.html
// https://en.cppreference.com/w/cpp/filesystem/path/make_preferred
// https://en.cppreference.com/w/cpp/filesystem/absolute
// https://en.cppreference.com/w/cpp/filesystem/last_write_time

namespace fs = std::filesystem;

class PCHArchive;

struct Dependency
{
	fs::path mPath;
	fs::file_time_type mTimestamp;
	bool mUpToDate = false;

	Dependency() = default;
	explicit Dependency(fs::path path);
	bool IsUpToDate() const;
};

struct ScriptDependencies
{
	std::vector< Dependency > mOutputs;		// all outputs should exist.
	std::vector< Dependency > mSources;		// all sources should be older than the youngest output. A missing include is an error.
	std::vector< Dependency > mImports;		// all imports should be newer than the youngest output. A script build may omit .modules, so these should be rechecked before compiling if they're what triggered a build request

	bool IsUpToDate() const;
};

struct DependencyDatabase
{
	std::map<fs::path, ScriptDependencies> mDB;
	ScriptDependencies& AddFile(fs::path file);
	void Serialize(PCHArchive& ar);
	bool IsUpToDate(fs::path file) const;
};
