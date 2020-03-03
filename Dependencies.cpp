#include "PCH.h"
#include "Dependencies.h"
#include "Serialize.h"

PCHArchive& operator<<(PCHArchive& ar, fs::path& p)
{
	auto& asString = p.native();
	size_t numElements = asString.size();
	ar << numElements;
	const size_t numBytes = numElements * sizeof(fs::path::value_type);
	ar.Serialize((char*)asString.c_str(), numBytes);
	return ar;
}

PCHArchive& operator<<(PCHArchive& ar, fs::file_time_type& time)
{
	auto duration = time.time_since_epoch();
	auto count = duration.count();
	ar << count;
	return ar;
}

PCHArchive& operator<<(PCHArchive& ar, Dependency& dep)
{
	ar << dep.mPath;
	ar << dep.mTimestamp;
	ar << dep.mUpToDate;
	return ar;
}

template <typename ValueType>
PCHArchive& operator<<(PCHArchive& ar, std::vector<ValueType>& arr)
{
	size_t size = arr.size();
	ar << size;
	for (auto& val : arr)
	{
		ar << val;
	}
	return ar;
}

PCHArchive& operator<<(PCHArchive& ar, ScriptDependencies& deps)
{
	ar << deps.mSources;
	ar << deps.mOutputs;
	ar << deps.mImports;
	return ar;
}

template <typename KeyType, typename ValueType>
PCHArchive& operator<<(PCHArchive& ar, std::map<KeyType, ValueType>& map)
{
	size_t size = map.size();
	ar << size;
	for (auto& pair : map)
	{
		KeyType& key = const_cast<KeyType&>(pair.first);
		ar << key;

		ValueType& value = pair.second;
		ar << value;
	}
	return ar;
}

Dependency::Dependency(fs::path path)
	: mPath(path)
{
	if (fs::exists(path))
		mTimestamp = fs::last_write_time(path);
}

bool Dependency::IsUpToDate() const
{
	if (!fs::exists(mPath))
		return false;

	return mTimestamp == fs::last_write_time(mPath);
}

bool ScriptDependencies::IsUpToDate() const
{
	for (const auto& include : mSources)
	{
		if (!include.IsUpToDate()) return false;
	}
	for (const auto& import : mImports)
	{
		if (!import.IsUpToDate()) return false;
	}
	for (const auto& output : mOutputs)
	{
		if (!output.IsUpToDate()) return false;
	}

	return true;
}

ScriptDependencies& DependencyDatabase::AddFile(fs::path file)
{
	auto successPair = mDB.try_emplace(file);
	auto it = successPair.first;
	ScriptDependencies& deps = it->second;
	deps.mSources.emplace_back(file);
	return deps;
}

void DependencyDatabase::Serialize(PCHArchive& ar)
{
	ar << mDB;
}

bool DependencyDatabase::IsUpToDate(fs::path file) const
{
	const auto& it = mDB.find(file);
	if (it != mDB.end())
	{
		return it->second.IsUpToDate();
	}
	return false;
}
