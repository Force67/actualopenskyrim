#pragma once

#include "Gamebryo/CoreLibs/NiSystem/NiFile.h"

#include <cstddef>
#include <cstdint>

#include "BSSystem/BSStringT.h"

struct _SYSTEMTIME;

extern uint32_t uiTESStandardBufferSize;

class BSFile : public NiFile
{
public:
	BSFile(const char* pcName, OpenMode eMode, uint32_t auiBufferSize, bool abTextMode);
	~BSFile() override;

	void Seek(int32_t iNumBytes) override;
	void SetEndianSwap(bool bDoSwap) override;
	void Seek(int32_t iOffset, int32_t iWhence) override;
	uint32_t GetFileSize() const override;
	virtual bool Open(bool abFormal, bool abTextMode);
	virtual bool OpenByFilePointer(FILE* pFile);
	virtual uint32_t GetSize();
	virtual uint32_t ReadString(BSString& arString, uint32_t auiMaxLen);
	virtual uint32_t ReadString(BSStringT<wchar_t, -1, DynamicMemoryManagementPol>& arString, uint32_t auiMaxLen);
	virtual uint32_t GetLine(char* pcBuffer, uint32_t auiMaxBytes, wchar_t wcDelimiter);
	virtual uint32_t WriteString(const BSString& arString, bool abBinary);
	virtual uint32_t WriteString(const BSStringT<wchar_t, -1, DynamicMemoryManagementPol>& arString, bool abBinary);
	virtual bool Exist();
	virtual uint32_t ReadF(void* pvBuffer, uint32_t uiBytes);
	virtual uint32_t WriteF(const void* pvBuffer, uint32_t uiBytes);

	void Close();
	char* FileName() { return pFileName; }
	bool ChangeBufferSize(uint32_t auiNewSize);
	_SYSTEMTIME GetLastSaveTime();

protected:
	BSFile();

	void AllocateBuffer(uint32_t auiSize);
	void FreeBuffer();
	void CheckIsGood();

	// alignas keeps Itanium ABIs from packing this into NiFile's tail padding.
	alignas(8) bool bUseAuxBuffer;
	char* m_pAuxBuffer;
	int32_t iAuxTrueFilePos;
	uint32_t iAuxBufferMinIndex;
	uint32_t iAuxBufferMaxIndex;
	char pFileName[260];
	uint32_t uiResult;
	uint32_t uiIOSize;
	uint32_t uiTrueFilePos;
	uint32_t uiFileSize;
	bool bVirtualAlloc;

private:
	static uint32_t RdNoSwap(NiBinaryStream* pkStream, void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents);
	static uint32_t WrNoSwap(NiBinaryStream* pkStream, const void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents);
	static uint32_t RdSwap(NiBinaryStream* pkStream, void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents);
	static uint32_t WrSwap(NiBinaryStream* pkStream, const void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents);
};
static_assert(sizeof(BSFile) == 0x180);
