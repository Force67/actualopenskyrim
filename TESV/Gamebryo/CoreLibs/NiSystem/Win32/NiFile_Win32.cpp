#include "Gamebryo/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo/CoreLibs/NiSystem/NiMemoryDefines.h"
#include <windows.h>
#include <cstring>

struct _SECURITY_ATTRIBUTES;
int BSCreateDirectory(const char* pcDir, _SECURITY_ATTRIBUTES* pAttributes);

bool NiFile::DefaultCreateDirectoryFunc(const char* pcDir)
{
	return BSCreateDirectory(pcDir, nullptr) != 0;
}

NiFile::NiFile(const char* pcName, OpenMode eMode, uint32_t uiBufferSize)
{
	SetEndianSwap(false);
	m_uiCurrentFilePos = 0;
	m_eMode = eMode;
	const char* pcMode = eMode == READ_ONLY ? "rb" : eMode == WRITE_ONLY ? "wb" : "ab";
	m_bGood = fopen_s(&m_pFile, pcName, pcMode) == 0 && m_pFile;
	m_uiBufferAllocSize = uiBufferSize;
	m_uiBufferReadSize = 0;
	m_uiPos = 0;
	m_pBuffer = m_bGood && uiBufferSize ? static_cast<char*>(_NiMalloc(uiBufferSize)) : nullptr;
}

NiFile::NiFile()
	: m_uiBufferAllocSize(0), m_uiBufferReadSize(0), m_uiPos(0), m_uiCurrentFilePos(0),
	  m_pBuffer(nullptr), m_pFile(nullptr), m_eMode(READ_ONLY), m_bGood(false)
{
	static_assert(offsetof(NiFile, m_uiBufferAllocSize) == 32);
	static_assert(offsetof(NiFile, m_uiBufferReadSize) == 36);
	static_assert(offsetof(NiFile, m_uiPos) == 40);
	static_assert(offsetof(NiFile, m_uiCurrentFilePos) == 44);
	static_assert(offsetof(NiFile, m_pBuffer) == 48);
	static_assert(offsetof(NiFile, m_pFile) == 56);
	static_assert(offsetof(NiFile, m_eMode) == 64);
	static_assert(offsetof(NiFile, m_bGood) == 68);
}

NiFile::~NiFile()
{
	if (m_bGood && m_pFile)
	{
		Flush();
		fclose(m_pFile);
	}
	_NiFree(m_pBuffer);
}

void NiFile::Seek(int32_t iOffset, int32_t iWhence)
{
	if (!m_bGood)
		return;
	if (iWhence == SEEK_CUR)
	{
		uint32_t uiPosition = m_uiPos + static_cast<uint32_t>(iOffset);
		if (static_cast<int32_t>(uiPosition) >= 0 && static_cast<int32_t>(uiPosition) < static_cast<int32_t>(m_uiBufferReadSize))
		{
			m_uiAbsoluteCurrentPos += static_cast<uint32_t>(iOffset);
			m_uiPos = uiPosition;
			return;
		}
		if (m_eMode == READ_ONLY)
			iOffset = static_cast<int32_t>(m_uiPos - m_uiBufferReadSize + static_cast<uint32_t>(iOffset));
	}
	Flush();
	int iResult = fseek(m_pFile, iOffset, iWhence);
	m_bGood = iResult == 0;
	if (!iResult)
	{
		uint32_t uiPosition = static_cast<uint32_t>(ftell(m_pFile));
		m_uiCurrentFilePos = uiPosition;
		m_uiAbsoluteCurrentPos = uiPosition;
	}
}

uint32_t NiFile::DiskRead(void* pvBuffer, uint32_t uiBytes)
{
	uint32_t uiRead = static_cast<uint32_t>(fread(pvBuffer, 1, uiBytes, m_pFile));
	m_uiCurrentFilePos += uiRead;
	return uiRead;
}

uint32_t NiFile::DiskWrite(const void* pvBuffer, uint32_t uiBytes)
{
	return static_cast<uint32_t>(fwrite(pvBuffer, 1, uiBytes, m_pFile));
}

uint32_t NiFile::GetActualFilePosition()
{
	return static_cast<uint32_t>(ftell(m_pFile));
}

bool NiFile::Flush()
{
	if (m_eMode == READ_ONLY)
		m_uiBufferReadSize = 0;
	else if (m_uiPos && static_cast<uint32_t>(fwrite(m_pBuffer, 1, m_uiPos, m_pFile)) != m_uiPos)
	{
		m_bGood = false;
		return false;
	}
	m_uiPos = 0;
	return true;
}

uint32_t NiFile::GetFileSize() const
{
	int32_t iPosition = static_cast<int32_t>(ftell(m_pFile));
	if (iPosition < 0)
		return 0;
	fseek(m_pFile, 0, SEEK_END);
	int32_t iSize = static_cast<int32_t>(ftell(m_pFile));
	fseek(m_pFile, iPosition, SEEK_SET);
	return iSize < 0 ? 0 : static_cast<uint32_t>(iSize);
}

bool NiFile::DefaultDirectoryExistsFunc(const char* pcDir)
{
	DWORD ulAttributes = GetFileAttributesA(pcDir);
	return ulAttributes != INVALID_FILE_ATTRIBUTES && (ulAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

uint32_t NiFile::FileRead(void* pvBuffer, uint32_t uiBytes)
{
	if (!m_bGood)
		return 0;
	char* pcOutput = static_cast<char*>(pvBuffer);
	uint32_t uiAvailable = m_uiBufferReadSize - m_uiPos;
	uint32_t uiTotal = 0;
	if (uiBytes > uiAvailable)
	{
		if (uiAvailable)
		{
			memcpy(pcOutput, m_pBuffer + m_uiPos, uiAvailable);
			pcOutput += uiAvailable;
			uiBytes -= uiAvailable;
			uiTotal = uiAvailable;
		}
		Flush();
		if (uiBytes > m_uiBufferAllocSize)
			return uiTotal + DiskRead(pcOutput, uiBytes);
		uint32_t uiRead = DiskRead(m_pBuffer, m_uiBufferAllocSize);
		m_uiBufferReadSize = uiRead;
		if (uiRead < uiBytes)
			uiBytes = uiRead;
	}
	memcpy(pcOutput, m_pBuffer + m_uiPos, uiBytes);
	m_uiPos += uiBytes;
	return uiTotal + uiBytes;
}

uint32_t NiFile::FileWrite(const void* pvBuffer, uint32_t uiBytes)
{
	if (!m_bGood)
		return 0;
	const char* pcInput = static_cast<const char*>(pvBuffer);
	uint32_t uiAvailable = m_uiBufferAllocSize - m_uiPos;
	uint32_t uiTotal = 0;
	if (uiBytes > uiAvailable)
	{
		if (uiAvailable)
		{
			memcpy(m_pBuffer + m_uiPos, pcInput, uiAvailable);
			pcInput += uiAvailable;
			uiBytes -= uiAvailable;
			m_uiPos = m_uiBufferAllocSize;
			uiTotal = uiAvailable;
		}
		if (!Flush())
			return 0;
		if (uiBytes >= m_uiBufferAllocSize)
			return uiTotal + DiskWrite(pcInput, uiBytes);
	}
	memcpy(m_pBuffer + m_uiPos, pcInput, uiBytes);
	m_uiPos += uiBytes;
	return uiTotal + uiBytes;
}
