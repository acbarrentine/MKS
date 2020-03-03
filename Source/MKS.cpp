#include "PCH.h"
#include <tuple>
#include <typeinfo>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

// https://github.com/KonanM/tser
// TSer serialization library has a nice looking trick to get fields into
// an iteratable tuple

std::vector<char> ReadData;

#pragma warning(disable:4100)	// unreferenced formal parameter

#define SERIALIZE_FIELDS(...) \
	constexpr inline decltype(auto) MemberList() const { return std::tie(__VA_ARGS__); }

enum class Origin { PCH };

struct SerializableObject
{
	static const int ClassID = -1;
	int A;

	SERIALIZE_FIELDS(A);

	SerializableObject(int a) : A(a) {}
	SerializableObject(Origin) {}
	virtual ~SerializableObject() = default;
	virtual int GetClassID() const = 0;
	virtual void Serialize(struct PCHWriter& writer) const = 0;
};

struct Statement : public SerializableObject
{
	static const int ClassID = 1;
	int B;

	SERIALIZE_FIELDS(A, B);

	Statement(int a, int b) : SerializableObject(a), B(b) {}
	Statement(Origin o) : SerializableObject(o) {}
	virtual ~Statement() = default;
	int GetClassID() const override { return ClassID; }
	void Serialize(struct PCHWriter& writer) const override;
};

template <typename T>
struct TArray
{
	T* Data = nullptr;
	int Num = 0;
	int Max = 0;

	void Add(const T& item)
	{
		const int index = Num++;
		if (Num > Max)
		{
			Max = Num * 2;
			Data = (T*)std::realloc(Data, NumBytes());
		}
		Data[index] = item;
	}

	size_t NumBytes() const
	{
		return sizeof(T) * Num;
	}
};

struct PCHWriter
{
	std::vector<char> Data;
	std::map<void*, size_t> ObjectMap;
	std::vector<size_t> PointerFixups;
	std::vector<size_t> ConstructorFixups;

	struct DebugRecord
	{
		const char* TypeName;
		size_t Offset;
		size_t NumBytes;
	};

	std::vector<DebugRecord> DebugInfo;

	PCHWriter()
	{
		Data.reserve(512);
	}

	~PCHWriter()
	{
		std::ofstream Stream{"MKS.pch", std::ios::binary};

		// replace raw pointers with indexed offsets
		char* rawData = Data.data();
		for (size_t offset : PointerFixups)
		{
			char* c = rawData + offset;
			void** ptr = (void**)c;
			void* objectPtr = *ptr;
			CHECK(ObjectMap.find(objectPtr) != ObjectMap.end());
			size_t targetOffset = ObjectMap[objectPtr];
			*((size_t*)c) = targetOffset;
		}

		// replace vtbl objects with class IDs
		for (size_t offset : ConstructorFixups)
		{
			char* c = rawData + offset;
			SerializableObject* pObj = (SerializableObject*)c;
			size_t classID = pObj->GetClassID();
			*((size_t*)c) = classID;
		}

		const size_t numPointerFixups = PointerFixups.size();
		const size_t numConstructorFixups = ConstructorFixups.size();
		const size_t dataSize = Data.size();

		Stream.write((char*)&numPointerFixups, sizeof(numPointerFixups));
		Stream.write((char*)&numConstructorFixups, sizeof(numConstructorFixups));
		Stream.write((char*)&dataSize, sizeof(dataSize));
		Stream.write((char*)PointerFixups.data(), PointerFixups.size() * sizeof(size_t));
		Stream.write((char*)ConstructorFixups.data(), ConstructorFixups.size() * sizeof(size_t));
		Stream.write(Data.data(), Data.size());
		Stream.close();

		// write the debug markup data
		std::sort(DebugInfo.begin(), DebugInfo.end(), [](const DebugRecord& left, const DebugRecord& right) {
			return left.Offset < right.Offset;
		});

		std::ofstream debugOut{"MKS.PCH.txt"};
		unsigned char* uData = (unsigned char*)Data.data();
		for (size_t i = 0; i < DebugInfo.size();)
		{
			const DebugRecord& keyRecord = DebugInfo[i++];
			size_t currentOffset = keyRecord.Offset;
			size_t numBytes = keyRecord.NumBytes;
			debugOut << currentOffset << ": " << keyRecord.TypeName << " (" << numBytes << " bytes)" << std::endl;
			const size_t objectEnd = currentOffset + numBytes;

			while (i < DebugInfo.size() && DebugInfo[i].Offset < objectEnd)
			{
				const DebugRecord& child = DebugInfo[i++];
				if (currentOffset < child.Offset)
				{
					debugOut << "  Unknown (" << (child.Offset - currentOffset) << "): ";
					while (currentOffset < child.Offset)
					{
						const unsigned int val = uData[currentOffset++];
						debugOut << std::hex << std::setw(2) << std::setfill('0') << val << " ";
					}
					debugOut << std::dec;
					debugOut << std::endl;
				}

				debugOut << "  " << child.TypeName << "(" << child.NumBytes << "): ";
				while (currentOffset < child.Offset + child.NumBytes)
				{
					const unsigned int val = uData[currentOffset++];
					debugOut << std::hex << std::setw(2) << std::setfill('0') << val << " ";
				}
				debugOut << std::dec;
				debugOut << std::endl;
			}
		}
	}

	template <typename... Args>
	size_t SaveObject(void* data, size_t numBytes, const std::tuple<Args...>& fields)
	{
		const size_t offset = GetCurrentOffset();
		ObjectMap[data] = offset;
		char* incomingData = (char*)data;
		Data.insert(Data.end(), incomingData, incomingData + numBytes);

		const char* objStart = (const char*)data;
		std::apply([&](auto&&... args) {
			(Serialize(*this, offset + ((char*)&args - objStart), args), ...);
		}, fields);

		return offset;
	}

	void Markup(const char* type, size_t offset, size_t numBytes)
	{
		DebugInfo.emplace_back(DebugRecord{ type, offset, numBytes});
	}

	template <typename... Args>
	size_t SaveBaseObject(const SerializableObject* object, size_t numBytes, const std::tuple<Args...>& fields)
	{
		const size_t offset = SaveObject((void*)object, numBytes, fields);
		Markup("Class ID", offset, sizeof(object));
		ConstructorFixups.push_back(offset);
		return offset;
	}

	void AddPointerFixup(size_t atOffset)
	{
		PointerFixups.push_back(atOffset);
	}

	size_t GetCurrentOffset() const
	{
		return Data.size();
	}
};

struct PCHReader
{
	std::ifstream Stream{"MKS.pch", std::ios::binary};

	PCHReader()
	{
		size_t numPointerFixups;
		size_t numConstructorFixups;
		size_t dataSize;

		Serialize(&numPointerFixups, sizeof(numPointerFixups));
		Serialize(&numConstructorFixups, sizeof(numConstructorFixups));
		Serialize(&dataSize, sizeof(dataSize));

		std::vector<size_t> pointerFixups;
		pointerFixups.assign(numPointerFixups, 0);
		Serialize(pointerFixups.data(), numPointerFixups * sizeof(size_t));

		std::vector<size_t> constructorFixups;
		constructorFixups.assign(numConstructorFixups, 0);
		Serialize(constructorFixups.data(), numConstructorFixups * sizeof(size_t));

		ReadData.assign(dataSize, 0);
		Serialize(ReadData.data(), dataSize);

		// fixup pointers within the block
		char* rawData = ReadData.data();
		for (size_t offset : pointerFixups)
		{
			char* c = rawData + offset;
			size_t* src = (size_t*)c;
			size_t targetOffset = *src;
			CHECK(targetOffset < dataSize);
			char* dst = rawData + targetOffset;
			*((char**)src) = dst;
		}

		// in-place constructors
		for (size_t offset : constructorFixups)
		{
			char* c = rawData + offset;
			int* classID = (int*)c;
			switch (*classID)
			{
			case 1:
				new(c) Statement(Origin::PCH);
				break;

			case -1:
			default:
				CHECK(false);
				break;
			}
		}
	}

	void Serialize(void* data, size_t numBytes)
	{
		Stream.read((char*)data, numBytes);
	}
};

void Serialize(PCHWriter& writer, size_t offset, const SerializableObject* obj)
{
	writer.AddPointerFixup(offset);
	obj->Serialize(writer);
}

void Statement::Serialize(PCHWriter& writer) const
{
	const size_t offset = writer.SaveBaseObject(this, sizeof(Statement), MemberList());
	writer.Markup(typeid(Statement).name(), offset, sizeof(Statement));
}

template <typename T>
void Serialize(PCHWriter& writer, size_t objectOffset, const TArray<T>& arr)
{
	auto fields = std::tie(arr.Num, arr.Max);

	writer.Markup(typeid(arr).name(), objectOffset, sizeof(arr));
	size_t dataOffset = writer.SaveObject(arr.Data, arr.NumBytes(), fields);
	writer.Markup(typeid(T*).name(), dataOffset, arr.NumBytes());
	objectOffset += offsetof(TArray<T>, Data);
	writer.AddPointerFixup(objectOffset);

	for (int i = 0; i < arr.Num; ++i)
	{
		Serialize(writer, dataOffset, arr.Data[i]);
		dataOffset += sizeof(T);
	}
}

template <typename T>
std::enable_if_t<std::is_arithmetic_v<T>, void>
Serialize(PCHWriter& writer, size_t offset, T)
{
	writer.Markup(typeid(T).name(), offset, sizeof(T));
}

struct Compiler
{
	float Dummy;
	TArray<Statement*> mStatements;
	int NumStatements;

	SERIALIZE_FIELDS(Dummy, mStatements, NumStatements);

	Compiler()
		: Dummy(2.0f)
		, NumStatements(0)
	{
	}

	void AddStatement(int seed)
	{
		++NumStatements;
		mStatements.Add(new Statement(seed, seed * 2));
	}

	void WritePCH()
	{
		PCHWriter writer;
		const size_t objectPos = writer.SaveObject(this, sizeof(Compiler), MemberList());
		writer.Markup(typeid(Compiler).name(), objectPos, sizeof(Compiler));
	}
};

Compiler* ReadPCH()
{
	PCHReader reader;
	return (Compiler*) ReadData.data();
}

TEST_CASE("read write pch")
{
	Compiler writer;
	writer.AddStatement(20);
	writer.AddStatement(100);
	writer.WritePCH();

	Compiler* reader = ReadPCH();

	CHECK(reader->NumStatements == 2);
	CHECK(reader->mStatements.Num == 2);
}
