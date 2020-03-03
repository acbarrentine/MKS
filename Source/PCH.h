#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>
#include <chrono>
#include <map>

#define NOMINMAX
#include <Windows.h>
#include <cstddef>

#pragma warning(push)
//#pragma warning(disable : 4834)		// disregarding [[nodiscard]]
#pragma warning(disable : 4267)			// conversion from size_t to int
#include <taskflow/taskflow.hpp>
#pragma warning(pop)
