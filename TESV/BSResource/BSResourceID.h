#pragma once

#include "BSCore/BSTSingleton.h"

#include <cstddef>
#include <cstdint>

namespace BSResource
{
	class RemapTable : public BSTSingletonImplicit<RemapTable>
	{
	public:
		RemapTable();
		const char* QTable() const { return TableA; }
		char TableA[256];
	};

	struct FileID
	{
		FileID() = default;
		FileID(unsigned int auiFile, unsigned int auiExt);
		explicit FileID(const char* apName);
		unsigned int QFile() const { return uiFile; }
		unsigned int QExt() const { return uiExt; }
		std::uint64_t QFileAndExt() const { return (std::uint64_t(uiExt) << 32) | uiFile; }

		unsigned int uiFile;
		unsigned int uiExt;
	};

	struct ID : FileID
	{
		explicit ID(const char* apPath = nullptr);
		ID(const char* apDirectory, const char* apFile);
		static void GenerateFromPath(ID& arID, const char* apPath);
		bool operator<(const ID& arID) const;
		bool operator<=(const ID& arID) const;
		bool operator==(const ID& arID) const { return uiDir == arID.uiDir && QFileAndExt() == arID.QFileAndExt(); }
		unsigned int QDir() const { return uiDir; }

		unsigned int uiDir;
	};

	static_assert(sizeof(RemapTable) == 256);
	static_assert(sizeof(FileID) == 8);
	static_assert(offsetof(FileID, uiFile) == 0);
	static_assert(offsetof(FileID, uiExt) == 4);
	static_assert(sizeof(ID) == 12);
	static_assert(offsetof(ID, uiDir) == 8);
}

template <> BSResource::RemapTable* BSTSingletonImplicit<BSResource::RemapTable>::QInstance();
