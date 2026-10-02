#include "BSSystem/BSMemStorage.h"

#include "BSCore/BSMemoryutility.h"

BSMemStorage::~BSMemStorage()
{
	uiBufferSize = 0;
	pBuffer = nullptr;
}

uint64_t BSMemStorage::GetSize()
{
	return uiBufferSize;
}

uint64_t BSMemStorage::GetPosition()
{
	return uiCurrentPos;
}

BSStorageDefs::ErrorCode BSMemStorage::Seek(int aiOffset, BSStorageDefs::SeekMode aeSeekMode)
{
	switch (aeSeekMode)
	{
	case BSStorageDefs::kSeekBegin:
		if (aiOffset < 0 || static_cast<uint32_t>(aiOffset) >= uiBufferSize)
			return BSStorageDefs::kInvalid;
		uiCurrentPos = aiOffset;
		return BSStorageDefs::kNone;
	case BSStorageDefs::kSeekCurrent:
		if (aiOffset > 0)
		{
			if (uiCurrentPos + static_cast<uint32_t>(aiOffset) >= uiBufferSize)
				return BSStorageDefs::kInvalid;
		}
		else if (aiOffset < 0 && static_cast<uint32_t>(-aiOffset) > uiCurrentPos)
		{
			return BSStorageDefs::kInvalid;
		}
		uiCurrentPos += aiOffset;
		return BSStorageDefs::kNone;
	case BSStorageDefs::kSeekEnd:
		if (aiOffset > 0 || static_cast<uint32_t>(-aiOffset) >= uiBufferSize)
			return BSStorageDefs::kInvalid;
		uiCurrentPos = uiBufferSize + aiOffset;
		return BSStorageDefs::kNone;
	default:
		return BSStorageDefs::kNone;
	}
}

BSStorageDefs::ErrorCode BSMemStorage::Read(uint32_t auiBytes, void* apvBuffer)
{
	if (uiCurrentPos + auiBytes > uiBufferSize)
		return BSStorageDefs::kInvalid;
	BSmemcpy(apvBuffer, auiBytes, pBuffer + uiCurrentPos, auiBytes);
	uiCurrentPos += auiBytes;
	return BSStorageDefs::kNone;
}

BSStorageDefs::ErrorCode BSMemStorage::Write(uint32_t auiBytes, const void* apvBuffer)
{
	if (uiCurrentPos + auiBytes > uiBufferSize)
		return BSStorageDefs::kInvalid;
	BSmemcpy(pBuffer + uiCurrentPos, auiBytes, apvBuffer, auiBytes);
	uiCurrentPos += auiBytes;
	return BSStorageDefs::kNone;
}
