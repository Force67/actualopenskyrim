#pragma once

#include <cstdint>
#include <cstddef>

#include "BSSystem/BSStorage.h"

// Storage over a caller provided buffer; the game hands these to script and
// save load readers with a scrap or heap allocated block.
class BSMemStorage : public BSStorage
{
public:
	BSMemStorage(uint64_t auiBufferSize, uint8_t* apBuffer);
	~BSMemStorage() override;

	uint64_t GetSize() override;
	uint64_t GetPosition() override;
	BSStorageDefs::ErrorCode Seek(int aiOffset, BSStorageDefs::SeekMode aeSeekMode) override;
	BSStorageDefs::ErrorCode Read(uint32_t auiBytes, void* apvBuffer) override;
	BSStorageDefs::ErrorCode Write(uint32_t auiBytes, const void* apvBuffer) override;

	uint32_t uiBufferSize;
	uint32_t uiCurrentPos;
	uint8_t* pBuffer;
};
static_assert(sizeof(BSMemStorage) == 0x30);
static_assert(offsetof(BSMemStorage, uiBufferSize) == 0x20);
static_assert(offsetof(BSMemStorage, uiCurrentPos) == 0x24);
static_assert(offsetof(BSMemStorage, pBuffer) == 0x28);

inline BSMemStorage::BSMemStorage(uint64_t auiBufferSize, uint8_t* apBuffer) :
	uiBufferSize(static_cast<uint32_t>(auiBufferSize)),
	uiCurrentPos(0),
	pBuffer(apBuffer)
{
	uiRefCount = 0;
	upStreamBuffer = nullptr;
	bUseStreamBuffer = false;
}
