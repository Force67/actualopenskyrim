#pragma once

#include <cstddef>
#include <cstdint>

class NiBinaryStream
{
public:
	struct BufferInfo
	{
		void* pvBuffer;
		uint32_t uiTotalSize;
		uint32_t uiBufferAllocSize;
		uint32_t uiBufferReadSize;
		uint32_t uiBufferPos;
		uint32_t uiStreamPos;
	};

	using ReadFunction = uint32_t (*)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t);
	using WriteFunction = uint32_t (*)(NiBinaryStream*, const void*, uint32_t, uint32_t*, uint32_t);

	NiBinaryStream();
	virtual ~NiBinaryStream();
	virtual operator bool() const = 0;
	virtual void Seek(int32_t iNumBytes) = 0;
	virtual uint32_t GetPosition() const;
	virtual void GetBufferInfo(BufferInfo& kInfo);
	virtual void SetEndianSwap(bool bDoSwap) = 0;

	uint32_t GetLine(char* pBuffer, uint32_t uiMaxBytes);
	uint32_t PutS(const char* pBuffer);
	static bool GetEndianMatchHint();
	static void SetEndianMatchHint(bool bHint);
	static void DoByteSwap(void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents);

	uint32_t m_uiAbsoluteCurrentPos;
	ReadFunction m_pfnRead;
	WriteFunction m_pfnWrite;
	static bool ms_bEndianMatchHint;
};
static_assert(sizeof(NiBinaryStream) == 0x20);
static_assert(offsetof(NiBinaryStream, m_uiAbsoluteCurrentPos) == 8);
static_assert(offsetof(NiBinaryStream, m_pfnRead) == 0x10);
static_assert(offsetof(NiBinaryStream, m_pfnWrite) == 0x18);
static_assert(sizeof(NiBinaryStream::BufferInfo) == 0x20);
