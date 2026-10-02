#pragma once

#include <cstdint>

#include "BSCore/MemoryManager.h"

class BSStorage;

namespace BSStorageDefs
{
	enum ErrorCode
	{
		kNone = 0,
		kInvalid = 1,
		kNotSupported = 2,
	};

	enum SeekMode
	{
		kSeekBegin = 0,
		kSeekCurrent = 1,
		kSeekEnd = 2,
	};

	// Scratch buffer backing BSStorage::Write; owned by the storage.
	struct StreamBuffer
	{
		uint64_t uiCapacity;
		MemoryManager::AutoScrapBuffer Buffer;
		uint8_t* pHead;
	};
	static_assert(sizeof(StreamBuffer) == 0x18);
}

// Reference counted byte sink/source; the game reads and writes whole blocks
// through the virtual interface.
class BSStorage
{
public:
	virtual ~BSStorage();
	virtual uint64_t GetSize() = 0;
	virtual uint64_t GetPosition() = 0;
	virtual BSStorageDefs::ErrorCode Seek(int aiOffset, BSStorageDefs::SeekMode aeSeekMode) = 0;
	virtual BSStorageDefs::ErrorCode Read(uint32_t auiBytes, void* apvBuffer) = 0;
	virtual BSStorageDefs::ErrorCode Write(uint32_t auiBytes, const void* apvBuffer) = 0;

	BSStorage()
	{
		uiRefCount = 0;
		upStreamBuffer = nullptr;
		bUseStreamBuffer = false;
	}

	volatile uint32_t uiRefCount;
	BSStorageDefs::StreamBuffer* upStreamBuffer;
	bool bUseStreamBuffer;
	// Fills the tail so derived classes start at 0x20 on every compiler.
	uint8_t ucPad[7];
};
static_assert(sizeof(BSStorage) == 0x20);
static_assert(offsetof(BSStorage, uiRefCount) == 0x8);
static_assert(offsetof(BSStorage, upStreamBuffer) == 0x10);
static_assert(offsetof(BSStorage, bUseStreamBuffer) == 0x18);
