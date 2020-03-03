#include "PCH.h"
#include "Serialize.h"

void PCHArchive::Serialize(char* data, size_t numBytes)
{
	mFile.write(data, numBytes);
}
