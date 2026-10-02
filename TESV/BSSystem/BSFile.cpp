#include "BSSystem/BSFile.h"

#include <cstring>
#include <io.h>

#include "BSCore/MemoryContextTracker.h"
#include "BSCore/MemoryManager.h"

uint32_t uiTESStandardBufferSize = 65536;

namespace
{
	const char g_acEmptyString[] = "";
	const wchar_t g_awcEmptyString[] = L"";
}

BSFile::BSFile()
{
	static_assert(offsetof(BSFile, bUseAuxBuffer) == 0x48);
	static_assert(offsetof(BSFile, m_pAuxBuffer) == 0x50);
	static_assert(offsetof(BSFile, iAuxTrueFilePos) == 0x58);
	static_assert(offsetof(BSFile, pFileName) == 0x64);
	static_assert(offsetof(BSFile, uiResult) == 0x168);
	static_assert(offsetof(BSFile, uiTrueFilePos) == 0x170);
	static_assert(offsetof(BSFile, uiFileSize) == 0x174);
	static_assert(offsetof(BSFile, bVirtualAlloc) == 0x178);

	bVirtualAlloc = false;
	m_eMode = READ_ONLY;
	m_uiBufferAllocSize = 0;
	m_uiBufferReadSize = 0;
	m_uiPos = 0;
	m_pBuffer = nullptr;
	m_pFile = nullptr;
	pFileName[0] = 0;
	uiResult = 0;
	uiIOSize = 0;
	uiTrueFilePos = 0;
	bUseAuxBuffer = false;
	m_pAuxBuffer = nullptr;
	iAuxBufferMinIndex = 0;
	iAuxBufferMaxIndex = 0;
	m_bGood = false;
	iAuxTrueFilePos = -1;
}

BSFile::~BSFile()
{
	Close();
}

void BSFile::Close()
{
	if (m_bGood && m_pFile)
	{
		Flush();
		fclose(m_pFile);
	}
	FreeBuffer();
	m_pFile = nullptr;
}

void BSFile::Seek(int32_t iNumBytes)
{
	Seek(iNumBytes, SEEK_CUR);
}

void BSFile::SetEndianSwap(bool bDoSwap)
{
	m_pfnRead = bDoSwap ? RdSwap : RdNoSwap;
	m_pfnWrite = bDoSwap ? WrSwap : WrNoSwap;
}

uint32_t BSFile::GetFileSize() const
{
	if (!uiFileSize)
		const_cast<BSFile*>(this)->GetSize();
	return uiFileSize;
}

bool BSFile::Open(bool, bool abTextMode)
{
	if (m_pFile)
		return true;
	m_uiBufferReadSize = 0;
	m_uiPos = 0;
	FreeBuffer();
	const char* pcText;
	const char* pcBinary;
	switch (m_eMode)
	{
	case READ_ONLY:
		pcText = "rt";
		pcBinary = "rb";
		break;
	case WRITE_ONLY:
		pcText = "r+t";
		pcBinary = "r+b";
		break;
	default:
		pcText = "a+t";
		pcBinary = "a+b";
		break;
	}
	m_pFile = nullptr;
	const char* pcMode = abTextMode ? pcText : pcBinary;
	fopen_s(&m_pFile, pFileName, pcMode);
	// "r+" fails on a missing file: create it, then reopen for update.
	if (!m_pFile && m_eMode == WRITE_ONLY)
	{
		m_pFile = nullptr;
		fopen_s(&m_pFile, pFileName, abTextMode ? "wt" : "wb");
		if (m_pFile)
			fclose(m_pFile);
		m_pFile = nullptr;
		fopen_s(&m_pFile, pFileName, pcMode);
	}
	CheckIsGood();
	return m_bGood;
}

bool BSFile::OpenByFilePointer(FILE* pFile)
{
	if (m_pFile)
		return true;
	m_uiBufferReadSize = 0;
	m_uiPos = 0;
	FreeBuffer();
	m_pFile = pFile;
	CheckIsGood();
	return m_bGood;
}

uint32_t BSFile::ReadString(BSString& arString, uint32_t auiMaxLen)
{
	char acChunk[260];
	uint32_t uiTotal = 0;
	uint32_t uiChunkTotal = 0;
	char c;
	do
	{
		uint32_t u = 0;
		do
		{
			uint32_t uiComponentSizes = 1;
			uint32_t uiRead = m_pfnRead(this, &c, 1, &uiComponentSizes, 1);
			m_uiAbsoluteCurrentPos += uiRead;
			uiTotal += uiRead;
			if (uiRead != 1)
				c = 0;
			if (uiTotal > auiMaxLen)
				c = 0;
			acChunk[u++] = c;
		} while (u != 259 && c);
		acChunk[u] = 0;
		if (uiChunkTotal)
			arString.Append(acChunk);
		else
			arString.Set(acChunk, 0);
		uiChunkTotal += u;
	} while (c);
	return uiTotal;
}

// Unlike the narrow version the chunk is not terminated after 260 characters.
uint32_t BSFile::ReadString(BSStringT<wchar_t, -1, DynamicMemoryManagementPol>& arString, uint32_t auiMaxLen)
{
	wchar_t awcChunk[292];
	uint32_t uiTotal = 0;
	uint32_t uiChunkTotal = 0;
	uint16_t usChar;
	do
	{
		uint32_t u = 0;
		do
		{
			uint32_t uiComponentSizes = 1;
			uint32_t uiRead = m_pfnRead(this, &usChar, 2, &uiComponentSizes, 1);
			m_uiAbsoluteCurrentPos += uiRead;
			uiTotal += uiRead;
			if (uiRead != 2)
				usChar = 0;
			if (uiTotal > auiMaxLen)
				usChar = 0;
			awcChunk[u++] = usChar;
		} while (u != 260 && usChar);
		if (uiChunkTotal)
			arString.Append(awcChunk);
		else
			arString.Set(awcChunk, 0);
		uiChunkTotal += u;
	} while (usChar);
	return uiTotal >> 1;
}

uint32_t BSFile::GetLine(char* pcBuffer, uint32_t auiMaxBytes, wchar_t wcDelimiter)
{
	uint32_t uiTotal = 0;
	uint32_t uCount = 0;
	if (auiMaxBytes > 1)
	{
		do
		{
			char c;
			uint32_t uiRead = ReadF(&c, 1);
			uiTotal += uiRead;
			if (uiRead != 1 || c == wcDelimiter)
				break;
			pcBuffer[uCount++] = c;
		} while (uCount + 1 < auiMaxBytes);
	}
	pcBuffer[uCount] = 0;
	return uiTotal;
}

uint32_t BSFile::WriteString(const BSString& arString, bool abBinary)
{
	uint32_t uiLength = arString.GetLength();
	const char* pcData = arString.QPtr() ? arString.QPtr() : g_acEmptyString;
	uint32_t uiComponentSizes = 1;
	uint32_t uiWritten = m_pfnWrite(this, pcData, uiLength + abBinary, &uiComponentSizes, 1);
	m_uiAbsoluteCurrentPos += uiWritten;
	uiTrueFilePos += uiWritten;
	return uiWritten;
}

uint32_t BSFile::WriteString(const BSStringT<wchar_t, -1, DynamicMemoryManagementPol>& arString, bool abBinary)
{
	uint32_t uiLength = arString.GetLength();
	const wchar_t* pwcData = arString.QPtr() ? arString.QPtr() : g_awcEmptyString;
	uint32_t uiComponentSizes = 1;
	uint32_t uiWritten = m_pfnWrite(this, pwcData, 2 * (uiLength + abBinary), &uiComponentSizes, 1);
	m_uiAbsoluteCurrentPos += uiWritten;
	uiTrueFilePos += uiWritten;
	return uiWritten >> 1;
}

bool BSFile::Exist()
{
	return _access(pFileName, 0) != -1;
}

uint32_t BSFile::WriteF(const void* pvBuffer, uint32_t uiBytes)
{
	uint32_t uiWritten = FileWrite(pvBuffer, uiBytes);
	uiTrueFilePos += uiWritten;
	return uiWritten;
}

bool BSFile::ChangeBufferSize(uint32_t auiNewSize)
{
	bool bResult = true;
	uint32_t uiMaxSize = GetSize();
	if (uiMaxSize < 10240)
		uiMaxSize = 10240;
	if (auiNewSize > uiMaxSize)
		auiNewSize = uiMaxSize;
	if (auiNewSize == m_uiBufferAllocSize)
		return true;
	AutoMemContext kContext(MC_FILE_BUFFER);
	NiFile::Seek(0, SEEK_SET);
	Flush();
	AllocateBuffer(auiNewSize);
	if (!m_pBuffer)
	{
		AllocateBuffer(uiTESStandardBufferSize);
		bResult = false;
	}
	uiTrueFilePos = 0;
	return bResult;
}

void BSFile::CheckIsGood()
{
	if (!m_pFile)
	{
		m_bGood = false;
		return;
	}
	m_bGood = true;
	if (m_uiBufferAllocSize && !m_pBuffer)
	{
		AutoMemContext kContext(MC_FILE_BUFFER);
		uint32_t uiRequest = m_uiBufferAllocSize;
		uint32_t uiSize = uiRequest;
		// A size of -1 means "buffer the whole file".
		if (uiRequest == 0xFFFFFFFF)
		{
			uiSize = GetFileSize();
			m_uiBufferAllocSize = uiSize;
		}
		AllocateBuffer(uiSize);
		if (uiRequest == 0xFFFFFFFF)
		{
			m_uiBufferReadSize = m_uiBufferAllocSize;
			m_uiPos = 0;
			if (DiskRead(m_pBuffer, m_uiBufferAllocSize) != m_uiBufferAllocSize)
				m_bGood = false;
		}
	}
}

uint32_t BSFile::RdNoSwap(NiBinaryStream* pkStream, void* pvBuffer, uint32_t uiBytes, uint32_t*, uint32_t)
{
	return static_cast<BSFile*>(pkStream)->ReadF(pvBuffer, uiBytes);
}

uint32_t BSFile::WrNoSwap(NiBinaryStream* pkStream, const void* pvBuffer, uint32_t uiBytes, uint32_t*, uint32_t)
{
	return static_cast<BSFile*>(pkStream)->WriteF(pvBuffer, uiBytes);
}

uint32_t BSFile::RdSwap(NiBinaryStream* pkStream, void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents)
{
	if (!uiBytes)
		return 0;
	uint32_t uiRead = static_cast<BSFile*>(pkStream)->ReadF(pvBuffer, uiBytes);
	if (puiComponentSizes)
		DoByteSwap(pvBuffer, uiBytes, puiComponentSizes, uiNumComponents);
	return uiRead;
}

uint32_t BSFile::WrSwap(NiBinaryStream* pkStream, const void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents)
{
	if (!uiBytes)
		return 0;
	void* pvCopy = MemoryManager::Instance().Allocate(uiBytes, 0, false);
	memcpy(pvCopy, pvBuffer, uiBytes);
	if (puiComponentSizes)
		DoByteSwap(pvCopy, uiBytes, puiComponentSizes, uiNumComponents);
	uint32_t uiWritten = static_cast<BSFile*>(pkStream)->WriteF(pvCopy, uiBytes);
	MemoryManager::Instance().Deallocate(pvCopy, false);
	return uiWritten;
}
