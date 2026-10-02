#include "Gamebryo/CoreLibs/NiSystem/NiBinaryStream.h"

bool NiBinaryStream::ms_bEndianMatchHint;

NiBinaryStream::NiBinaryStream() :
	m_uiAbsoluteCurrentPos(0),
	m_pfnRead(nullptr),
	m_pfnWrite(nullptr)
{
}

NiBinaryStream::~NiBinaryStream()
{
}

uint32_t NiBinaryStream::GetPosition() const
{
	return m_uiAbsoluteCurrentPos;
}

void NiBinaryStream::GetBufferInfo(BufferInfo& kInfo)
{
	kInfo.pvBuffer = nullptr;
	kInfo.uiTotalSize = 0;
	kInfo.uiBufferAllocSize = 0;
	kInfo.uiBufferReadSize = 0;
	kInfo.uiBufferPos = 0;
	kInfo.uiStreamPos = 0;
}

uint32_t NiBinaryStream::GetLine(char* pBuffer, uint32_t uiMaxBytes)
{
	uint32_t uiRead = 0;
	uint32_t uiStored = 0;
	if (uiMaxBytes > 1)
	{
		do
		{
			char c;
			uint32_t uiComponentSize = 1;
			uint32_t uiBytes = m_pfnRead(this, &c, 1, &uiComponentSize, 1);
			m_uiAbsoluteCurrentPos += uiBytes;
			uiRead += uiBytes;
			if (uiBytes != 1 || c == '\n')
				break;
			if (c != '\r')
				pBuffer[uiStored++] = c;
		}
		while (uiStored + 1 < uiMaxBytes);
	}
	pBuffer[uiStored] = '\0';
	return uiRead;
}

uint32_t NiBinaryStream::PutS(const char* pBuffer)
{
	uint32_t uiWritten = 0;
	for (; *pBuffer; ++pBuffer, ++uiWritten)
	{
		uint32_t uiComponentSize = 1;
		uint32_t uiBytes = m_pfnWrite(this, pBuffer, 1, &uiComponentSize, 1);
		m_uiAbsoluteCurrentPos += uiBytes;
		if (uiBytes != 1)
			break;
	}
	return uiWritten;
}

bool NiBinaryStream::GetEndianMatchHint()
{
	return ms_bEndianMatchHint;
}

void NiBinaryStream::SetEndianMatchHint(bool bHint)
{
	ms_bEndianMatchHint = bHint;
}

void NiBinaryStream::DoByteSwap(void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents)
{
	auto* pcBuffer = static_cast<char*>(pvBuffer);
	uint32_t uiProcessed = 0;
	while (uiProcessed < uiBytes)
	{
		for (uint32_t i = 0; i < uiNumComponents; ++i)
		{
			uint32_t uiSize = puiComponentSizes[i];
			uint32_t uiChunk = (uiSize == 16 || uiSize == 64) ? 4 : uiSize;
			if (uiSize == 2 || uiSize == 4 || uiSize == 8 || uiSize == 16 || uiSize == 64)
			{
				for (uint32_t j = 0; j < uiSize; j += uiChunk)
				{
					for (uint32_t k = 0; k < uiChunk / 2; ++k)
					{
						char c = pcBuffer[j + k];
						pcBuffer[j + k] = pcBuffer[j + uiChunk - k - 1];
						pcBuffer[j + uiChunk - k - 1] = c;
					}
				}
			}
			pcBuffer += uiSize;
			uiProcessed += uiSize;
		}
	}
}
