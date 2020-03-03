#include "PCH.h"
#include "CompileJob.h"

void CompileJob::Build()
{
	mSource.Read();
	const auto& dependencies = mSource.GetDependencies();
	for (auto& import : dependencies)
	{
		std::cout << mSource.GetFilename() << " depends on " << import << std::endl;
	}
}
