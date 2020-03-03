#pragma once

class PCHArchive
{
protected:
	std::fstream mFile;

public:
	void Serialize(char* data, size_t numBytes);

	template <typename T>
	auto operator<<(T& val) -> std::enable_if_t<std::is_arithmetic_v<T>>
	{
		Serialize(reinterpret_cast<char*>(&val), sizeof(val));
	}
};
