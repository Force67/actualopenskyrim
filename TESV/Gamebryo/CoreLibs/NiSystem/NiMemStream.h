#pragma once

#include "NiBinaryStream.h"

class NiMemStream : public NiBinaryStream
{
public:
	NiMemStream(const void* pBuffer, uint32_t uiSize);
	NiMemStream();
	~NiMemStream() override;
	operator bool() const override;
	void Seek(int32_t iNumBytes) override;
	void GetBufferInfo(BufferInfo& kInfo) override;
	void SetEndianSwap(bool bDoSwap) override;
	void* Str();
	void Freeze(bool bFreeze);

protected:
	friend class NiStream;
	uint32_t MemRead(void* pvBuffer, uint32_t uiBytes);
	uint32_t MemWrite(const void* pvBuffer, uint32_t uiBytes);
	static uint32_t ReadNoSwap(NiBinaryStream* pkStream, void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents);
	static uint32_t WriteNoSwap(NiBinaryStream* pkStream, const void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents);
	static uint32_t ReadAndSwap(NiBinaryStream* pkStream, void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents);
	static uint32_t WriteAndSwap(NiBinaryStream* pkStream, const void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents);

	char* m_pBuffer;
	uint32_t m_uiPos;
	uint32_t m_uiEnd;
	uint32_t m_uiAllocSize;
	bool m_bUserMemory;
	bool m_bFreeze;
};
static_assert(sizeof(NiMemStream) == 56);
