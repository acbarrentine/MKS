#pragma once

#include "SourceFile.h"

class CompileJob
{
protected:
	SourceFile mSource;
	tf::Task mTask;

public:
	CompileJob(const std::filesystem::path& path)
		: mSource(path)
	{
	}

	~CompileJob() = default;
	CompileJob(const CompileJob&) = delete;
	CompileJob& operator=(const CompileJob&) = delete;
	CompileJob(CompileJob&&) = default;
	CompileJob& operator=(CompileJob&&) = default;

	void Build();
	tf::Task& GetTask() { return mTask; }
};

