#include "NiMemStream.h"
#include "NiMemoryDefines.h"

#include <cstring>

NiMemStream::NiMemStream(const void* pBuffer, uint32_t uiSize) :
	m_pBuffer(const_cast<char*>(static_cast<const char*>(pBuffer))), m_uiPos(0),
	m_uiEnd(uiSize), m_uiAllocSize(uiSize), m_bUserMemory(true), m_bFreeze(false)
{
	static_assert(offsetof(NiMemStream, m_pBuffer) == 32);
	static_assert(offsetof(NiMemStream, m_uiPos) == 40);
	static_assert(offsetof(NiMemStream, m_uiEnd) == 44);
	static_assert(offsetof(NiMemStream, m_uiAllocSize) == 48);
	static_assert(offsetof(NiMemStream, m_bUserMemory) == 52);
	static_assert(offsetof(NiMemStream, m_bFreeze) == 53);
	m_pfnRead = ReadNoSwap;
	m_pfnWrite = WriteNoSwap;
}

NiMemStream::NiMemStream() : m_uiAllocSize(1024)
{
	m_pBuffer = static_cast<char*>(_NiMalloc(1024));
	m_uiPos = m_uiEnd = 0;
	m_bUserMemory = m_bFreeze = false;
	m_pfnRead = ReadNoSwap;
	m_pfnWrite = WriteNoSwap;
}

NiMemStream::~NiMemStream()
{
	if (!m_bUserMemory && !m_bFreeze)
		_NiFree(m_pBuffer);
}

NiMemStream::operator bool() const
{
	return true;
}

void NiMemStream::Seek(int32_t iNumBytes)
{
	uint32_t uiPos = m_uiPos + static_cast<uint32_t>(iNumBytes);
	if (static_cast<int32_t>(uiPos) >= 0 && uiPos < m_uiAllocSize)
	{
		m_uiPos = uiPos;
		m_uiAbsoluteCurrentPos = uiPos;
	}
}

void NiMemStream::GetBufferInfo(BufferInfo& kInfo)
{
	kInfo.pvBuffer = m_pBuffer;
	kInfo.uiTotalSize = m_uiEnd;
	kInfo.uiBufferAllocSize = m_uiEnd;
	kInfo.uiBufferReadSize = m_uiEnd;
	kInfo.uiBufferPos = m_uiPos;
	kInfo.uiStreamPos = m_uiPos;
}

void NiMemStream::SetEndianSwap(bool bDoSwap)
{
	m_pfnRead = bDoSwap ? ReadAndSwap : ReadNoSwap;
	m_pfnWrite = bDoSwap ? WriteAndSwap : WriteNoSwap;
}

void* NiMemStream::Str()
{
	m_bFreeze = true;
	return m_pBuffer;
}

void NiMemStream::Freeze(bool bFreeze)
{
	m_bFreeze = bFreeze;
}

uint32_t NiMemStream::MemRead(void* pvBuffer, uint32_t uiBytes)
{
	uint32_t uiPos = m_uiPos;
	uint32_t uiReadSize = m_uiEnd - uiPos;
	if (uiBytes < uiReadSize)
		uiReadSize = uiBytes;
	memcpy(pvBuffer, m_pBuffer + uiPos, uiReadSize);
	m_uiPos += uiReadSize;
	return uiReadSize;
}

uint32_t NiMemStream::MemWrite(const void* pvBuffer, uint32_t uiBytes)
{
	if (m_bUserMemory)
		return 0;
	uint32_t uiAllocSize = m_uiAllocSize;
	uint32_t uiPos = m_uiPos;
	uint32_t uiAvailable = uiAllocSize - uiPos;
	char* pcBuffer = m_pBuffer;
	if (uiBytes > uiAvailable)
	{
		uint32_t uiNewSize = uiBytes > uiAllocSize + uiAvailable ? uiPos + uiBytes : uiAllocSize * 2;
		pcBuffer = static_cast<char*>(_NiMalloc(uiNewSize));
		memcpy(pcBuffer, m_pBuffer, m_uiEnd);
		_NiFree(m_pBuffer);
		uiPos = m_uiPos;
		m_uiAllocSize = uiNewSize;
		m_pBuffer = pcBuffer;
	}
	memcpy(pcBuffer + uiPos, pvBuffer, uiBytes);
	m_uiPos += uiBytes;
	if (m_uiPos > m_uiEnd)
		m_uiEnd = m_uiPos;
	return uiBytes;
}

uint32_t NiMemStream::ReadNoSwap(NiBinaryStream* pkStream, void* pvBuffer, uint32_t uiBytes, uint32_t*, uint32_t)
{
	return static_cast<NiMemStream*>(pkStream)->MemRead(pvBuffer, uiBytes);
}

uint32_t NiMemStream::WriteNoSwap(NiBinaryStream* pkStream, const void* pvBuffer, uint32_t uiBytes, uint32_t*, uint32_t)
{
	return static_cast<NiMemStream*>(pkStream)->MemWrite(pvBuffer, uiBytes);
}

uint32_t NiMemStream::ReadAndSwap(NiBinaryStream* pkStream, void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents)
{
	if (!uiBytes)
		return 0;
	uint32_t uiReadSize = static_cast<NiMemStream*>(pkStream)->MemRead(pvBuffer, uiBytes);
	DoByteSwap(pvBuffer, uiBytes, puiComponentSizes, uiNumComponents);
	return uiReadSize;
}

uint32_t NiMemStream::WriteAndSwap(NiBinaryStream* pkStream, const void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents)
{
	if (!uiBytes)
		return 0;
	void* pvTemporary = _NiMalloc(uiBytes);
	memcpy(pvTemporary, pvBuffer, uiBytes);
	DoByteSwap(pvTemporary, uiBytes, puiComponentSizes, uiNumComponents);
	uint32_t uiWriteSize = static_cast<NiMemStream*>(pkStream)->MemWrite(pvTemporary, uiBytes);
	_NiFree(pvTemporary);
	return uiWriteSize;
}
