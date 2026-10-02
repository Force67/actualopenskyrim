#include "NiStream.h"
#include "NiStreamMacros.h"
#include "NiObjectGroup.h"
#include "NiRTTI.h"
#include "Gamebryo/CoreLibs/NiSystem/NiThreadProcedure.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo/CoreLibs/NiSystem/NiMemStream.h"
#include "Gamebryo/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo/CoreLibs/NiSystem/NiPath.h"
#include <cstring>
#include <cstdlib>
#include <new>

thread_local BSScrapArray<uint32_t>* pLinkIDAS = nullptr;
thread_local BSScrapArray<uint32_t>* pLinkIDBlocksAS = nullptr;

bool NiStream::Load(NiBinaryStream* pkIstr)
{
	bool bResult = false;
	if (*pkIstr)
	{
		m_pkIstr = pkIstr;
		bResult = LoadStream();
	}
	m_pkIstr = nullptr;
	return bResult;
}

bool NiStream::Save(NiBinaryStream* pkOstr)
{
	m_pkOstr = pkOstr;
	bool bResult = SaveStream();
	m_pkOstr = nullptr;
	return bResult;
}

bool NiStream::Load(char* pcBuffer, uint32_t uiBufferSize)
{
	NiMemStream kIstr(pcBuffer, uiBufferSize);
	return Load(&kIstr);
}

bool NiStream::Save(char*& pcBuffer, uint32_t& uiBufferSize)
{
	NiMemStream kOstr;
	bool bResult = Save(&kOstr);
	uiBufferSize = kOstr.m_uiEnd;
	pcBuffer = static_cast<char*>(kOstr.Str());
	return bResult;
}

bool NiStream::Load(const char* pcFileName)
{
	strcpy_s(m_acFileName, sizeof(m_acFileName), pcFileName);
	NiPath::Standardize(m_acFileName);
	SetFilePath(m_acFileName);
	NiFile* pkFile = NiFile::GetFile(m_acFileName, NiFile::READ_ONLY, 0x8000);
	bool bResult = false;
	if (pkFile && *pkFile)
		bResult = Load(pkFile);
	else
	{
		m_uiLastError = 1;
		strcpy_s(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), "Cannot open file.");
	}
	delete pkFile;
	return bResult;
}

bool NiStream::Save(const char* pcFileName)
{
	strcpy_s(m_acFileName, sizeof(m_acFileName), pcFileName);
	NiPath::Standardize(m_acFileName);
	SetFilePath(m_acFileName);
	NiFile* pkFile = NiFile::GetFile(m_acFileName, NiFile::WRITE_ONLY, 0x8000);
	bool bResult = false;
	if (pkFile && *pkFile)
		bResult = Save(pkFile);
	delete pkFile;
	return bResult;
}

void NiStream::BackgroundLoadOnExit() {}

void NiStream::SetSaveAsLittleEndian(bool bLittle)
{
	m_bSaveLittleEndian = bLittle;
}

void NiStream::SetLastError(uint32_t uiError)
{
	m_uiLastError = uiError;
}

void NiStream::SetFilePath(const char* pcFilePath)
{
	if (pcFilePath && *pcFilePath)
	{
		strncpy_s(m_acFilePath, sizeof(m_acFilePath), pcFilePath, sizeof(m_acFilePath) - 1);
		NiPath::Standardize(m_acFilePath);
	}
	else
		m_acFilePath[0] = 0;
}

void NiStream::ReadLinkID()
{
	uint32_t uiLinkID;
	NiStreamLoadBinary(*this, uiLinkID);
	pLinkIDAS->Add(uiLinkID);
}

uint32_t NiStream::ReadMultipleLinkIDs()
{
	uint32_t uiCount;
	NiStreamLoadBinary(*this, uiCount);
	pLinkIDBlocksAS->Add(uiCount);
	for (uint32_t ui = 0; ui < uiCount; ++ui)
		ReadLinkID();
	return uiCount;
}

NiObject* NiStream::ResolveLinkID()
{
	uint32_t uiLinkID;
	NiStreamLoadBinary(*this, uiLinkID);
	return uiLinkID == ~0u ? nullptr : m_kObjects.m_pBase[uiLinkID].m_pObject;
}

NiObject* NiStream::GetObjectFromLinkID()
{
	uint32_t uiIndex = m_uiLinkIndex++;
	uint32_t uiLinkID = (*pLinkIDAS)[uiIndex];
	return uiLinkID == ~0u ? nullptr : m_kObjects.m_pBase[uiLinkID].m_pObject;
}

uint32_t NiStream::GetNumberOfLinkIDs()
{
	uint32_t uiIndex = m_uiLinkBlockIndex++;
	return (*pLinkIDBlocksAS)[uiIndex];
}

void NiStream::SetNumberOfLinkIDs(uint32_t uiLinks)
{
	pLinkIDBlocksAS->Add(uiLinks);
}

NiObjectGroup* NiStream::GetGroupFromID(uint32_t uiID) const
{
	return m_kGroups[uiID];
}

void NiStream::SaveCString(const char* pcString)
{
	uint32_t uiLength = pcString ? static_cast<uint32_t>(std::strlen(pcString)) : 0;
	NiStreamSaveBinary(*this, uiLength);
	if (uiLength)
		NiStreamSaveBinary(*this, pcString, uiLength);
}

void NiStream::LoadCString(char*& pcString)
{
	int32_t iLength;
	NiStreamLoadBinary(*this, iLength);
	if (iLength <= 0)
		pcString = nullptr;
	else
	{
		pcString = static_cast<char*>(_NiMalloc(static_cast<uint64_t>(static_cast<int32_t>(static_cast<uint32_t>(iLength) + 1))));
		NiStreamLoadBinary(*this, pcString, static_cast<uint32_t>(iLength));
		pcString[iLength] = 0;
	}
}

void NiStream::LoadFixedString(BSFixedString& kString)
{
	uint32_t uiID;
	NiStreamLoadBinary(*this, uiID);
	if (uiID == ~0u)
		kString = BSFixedString(nullptr);
	else
		kString = m_kFixedStrings.m_pBase[uiID];
}

void NiStream::SaveFixedString(const BSFixedString& kString)
{
	uint32_t uiID = ~0u;
	if (!kString)
	{
		NiStreamSaveBinary(*this, uiID);
		return;
	}
	if (kString.pString)
	{
		for (uint32_t ui = 0; ui < m_kFixedStrings.m_uiSize; ++ui)
		{
			if (kString.pString == m_kFixedStrings.m_pBase[ui].pString)
			{
				uiID = ui;
				break;
			}
		}
	}
	NiStreamSaveBinary(*this, uiID);
}

void NiStream::LoadCStringAsFixedString(BSFixedString& kString)
{
	int32_t iLength;
	NiStreamLoadBinary(*this, iLength);
	if (iLength >= 1024)
	{
		char* pcString = static_cast<char*>(_NiMalloc(static_cast<uint64_t>(static_cast<int32_t>(static_cast<uint32_t>(iLength) + 1))));
		NiStreamLoadBinary(*this, pcString, static_cast<uint32_t>(iLength));
		pcString[iLength] = 0;
		kString = BSFixedString(pcString);
		_NiFree(pcString);
	}
	else if (iLength > 0)
	{
		char acString[1024];
		NiStreamLoadBinary(*this, acString, static_cast<uint32_t>(iLength));
		acString[iLength] = 0;
		kString = BSFixedString(acString);
	}
	else
		kString = BSFixedString(nullptr);
}

uint32_t NiStream::GetLinkIDFromObject(const NiObject* pkObject) const
{
	if (!pkObject)
		return UINT32_MAX;
	const NiTMapBase<NiTPointerAllocator<uint64_t>, const NiObject*, uint32_t>* volatile pkMap = &m_kRegisterMap;
	uint32_t uiHash = pkMap->KeyToHashIndex(pkObject);
	NiTMapItem<const NiObject*, uint32_t>* pkItem = m_kRegisterMap.m_ppkHashTable[uiHash];
	while (pkItem)
	{
		if (pkMap->IsKeysEqual(pkObject, pkItem->m_key))
			return pkItem->m_val;
		pkItem = pkItem->m_pkNext;
	}
	return UINT32_MAX;
}

void NiStream::ChangeObject(NiObject* pkObject)
{
	NiPointer<NiObject> spObject(pkObject);
	m_kObjects.SetAt(m_uiLoad, spObject);
}

void NiStream::SaveLinkID(const NiObject* pkObject)
{
	uint32_t uiID = GetLinkIDFromObject(pkObject);
	NiStreamSaveBinary(*this, uiID);
}

void NiStream::RegisterObjects()
{
	for (uint32_t i = 0; i < m_kTopObjects.m_uiSize; ++i)
		m_kTopObjects.m_pBase[i]->RegisterStreamables(*this);
}

void NiStream::SaveTopLevelObjects()
{
	uint32_t uiSize = m_kTopObjects.m_uiSize;
	NiStreamSaveBinary(*this, uiSize);
	for (uint32_t i = 0; i < uiSize; ++i)
		SaveLinkID(m_kTopObjects.m_pBase[i]);
}

uint32_t NiStream::PreSaveObjectSizeTable()
{
	uint32_t uiPosition = m_pkOstr->GetPosition();
	uint32_t uiSize = 0;
	for (uint32_t i = 0; i < m_kObjects.m_uiSize; ++i)
		NiStreamSaveBinary(*this, uiSize);
	return uiPosition;
}

bool NiStream::SaveObjectSizeTable(uint32_t uiPosition)
{
	uint32_t uiCurrent = m_pkOstr->GetPosition();
	m_pkOstr->Seek(static_cast<int32_t>(uiPosition - uiCurrent));
	for (uint32_t i = 0; i < m_kObjectSizes.m_uiSize; ++i)
	{
		uint32_t uiSize = m_kObjectSizes.m_pBase[i];
		NiStreamSaveBinary(*this, uiSize);
	}
	return true;
}

bool NiStream::LoadObjectSizeTable()
{
	m_kObjectSizes.SetSize(m_kObjects.m_uiSize);
	for (uint32_t i = 0; i < m_kObjects.m_uiSize; ++i)
	{
		uint32_t uiSize = 0;
		NiStreamLoadBinary(*this, uiSize);
		m_kObjectSizes.SetAt(i, uiSize);
	}
	return true;
}

uint32_t NiStream::GetVersionFromString(const char* pcVersionString)
{
	char acVS[16];
	strcpy_s(acVS, sizeof(acVS), pcVersionString);
	char* pcContext;
	uint32_t uiVersion = 0;
	int32_t iShift = 24;
	for (char* pcPart = strtok_s(acVS, ".", &pcContext); pcPart; pcPart = strtok_s(nullptr, ".", &pcContext))
	{
		uiVersion |= static_cast<uint32_t>(atoi(pcPart)) << (static_cast<uint32_t>(iShift) & 31);
		iShift -= 8;
	}
	return uiVersion;
}

bool NiStream::GetNextSearchPath(char* pcPath, uint32_t)
{
	if (!bUseDefaultPath)
		return false;
	strncpy_s(pcPath, 260, m_acFilePath, 259);
	bUseDefaultPath = false;
	return true;
}

bool NiStream::LoadHeader()
{
	char acHeader[128];
	m_pkIstr->GetLine(acHeader, sizeof(acHeader));
	if (!strstr(acHeader, "File Format"))
	{
		m_uiLastError = 2;
		strcpy_s(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), "Not a NIF file");
		return false;
	}
	m_pkIstr->SetEndianSwap(false);
	NiStreamLoadBinary(*this, m_uiNifFileVersion);
	const char* pcError = nullptr;
	if (m_uiNifFileVersion < ms_uiNifMinVersion)
	{
		m_uiLastError = 3;
		pcError = "NIF version is too old.";
	}
	else if (m_uiNifFileVersion > ms_uiNifMaxVersion)
	{
		m_uiLastError = 4;
		pcError = "Unknown NIF version.";
	}
	if (pcError)
	{
		strcpy_s(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), pcError);
		return false;
	}
	m_bSourceIsLittleEndian = true;
	if (m_uiNifFileVersion >= 0x14000003)
	{
		NiStreamLoadBinary(*this, m_bSourceIsLittleEndian);
		if (*reinterpret_cast<const uint8_t*>(&m_bSourceIsLittleEndian) != 1 && NiBinaryStream::GetEndianMatchHint())
		{
			m_uiLastError = 6;
			strcpy_s(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), "Endian mismatch.");
			return false;
		}
	}
	if (m_uiNifFileVersion >= 0x0A000108)
	{
		NiStreamLoadBinary(*this, m_uiNifFileUserDefinedVersion);
		if (m_uiNifFileVersion == 0x0A000165)
			m_uiNifFileUserDefinedVersion = 1;
	}
	if (m_uiNifFileUserDefinedVersion < ms_uiNifMinUserDefinedVersion)
	{
		m_uiLastError = 3;
		pcError = "NIF user defined version is too old.";
	}
	else if (m_uiNifFileUserDefinedVersion > ms_uiNifMaxUserDefinedVersion)
	{
		m_uiLastError = 4;
		pcError = "Unknown NIF user defined version.";
	}
	if (pcError)
	{
		strcpy_s(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), pcError);
		return false;
	}
	uint32_t uiObjects;
	NiStreamLoadBinary(*this, uiObjects);
	m_kObjects.SetSize(uiObjects);
	m_BSStreamHeader.uiVersion = 0;
	if (m_uiNifFileVersion > 0x0A000100 && (m_uiNifFileVersion <= 0x0A000108 || m_uiNifFileUserDefinedVersion))
	{
		if (m_uiNifFileVersion > 0x0A000101)
		{
			NiStreamLoadBinary(*this, m_BSStreamHeader.uiVersion);
			if (m_uiNifFileVersion == 0x0A000102)
				m_uiNifFileVersion = 0x0A000101;
		}
		uint8_t ucLength;
		NiStreamLoadBinary(*this, ucLength);
		NiStreamLoadBinary(*this, m_BSStreamHeader.pAuthor, ucLength);
		NiStreamLoadBinary(*this, ucLength);
		NiStreamLoadBinary(*this, m_BSStreamHeader.pProcessScript, ucLength);
		NiStreamLoadBinary(*this, ucLength);
		NiStreamLoadBinary(*this, m_BSStreamHeader.pExportScript, ucLength);
		if (m_BSStreamHeader.uiVersion < 72)
		{
			m_uiLastError = 3;
			pcError = "NIF BSversion is too old";
		}
		else if (m_BSStreamHeader.uiVersion > 100)
		{
			m_uiLastError = 4;
			pcError = "NIF BSversion is newer than is currently supported!";
		}
		if (pcError)
		{
			strcpy_s(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), pcError);
			return false;
		}
	}
	m_pkIstr->SetEndianSwap(*reinterpret_cast<const uint8_t*>(&m_bSourceIsLittleEndian) != 1);
	return true;
}

void NiStream::SaveHeader()
{
	m_pkOstr->SetEndianSwap(false);
	m_pkOstr->PutS("Gamebryo File Format, Version 20.2.0.7\n");
	NiStreamSaveBinary(*this, ms_uiNifMaxVersion);
	NiStreamSaveBinary(*this, m_bSaveLittleEndian);
	NiStreamSaveBinary(*this, ms_uiNifMaxUserDefinedVersion);
	uint32_t uiObjects = m_kObjects.m_uiSize;
	NiStreamSaveBinary(*this, uiObjects);
	if (ms_uiNifMaxUserDefinedVersion)
	{
		NiStreamSaveBinary(*this, m_BSStreamHeader.uiVersion);
		uint8_t ucLength = static_cast<uint8_t>(strlen(m_BSStreamHeader.pAuthor) + 1);
		NiStreamSaveBinary(*this, ucLength);
		NiStreamSaveBinary(*this, m_BSStreamHeader.pAuthor, ucLength);
		ucLength = static_cast<uint8_t>(strlen(m_BSStreamHeader.pProcessScript) + 1);
		NiStreamSaveBinary(*this, ucLength);
		NiStreamSaveBinary(*this, m_BSStreamHeader.pProcessScript, ucLength);
		ucLength = static_cast<uint8_t>(strlen(m_BSStreamHeader.pExportScript) + 1);
		NiStreamSaveBinary(*this, ucLength);
		NiStreamSaveBinary(*this, m_BSStreamHeader.pExportScript, ucLength);
	}
	m_pkOstr->SetEndianSwap(*reinterpret_cast<const uint8_t*>(&m_bSaveLittleEndian) ^ 1);
}

void NiStream::InsertObject(NiObject* pkObject)
{
	NiPointer<NiObject> spObject(pkObject);
	uint32_t uiIndex = m_kTopObjects.m_uiSize;
	if (uiIndex >= m_kTopObjects.m_uiMaxSize)
		m_kTopObjects.SetSize(uiIndex + m_kTopObjects.m_uiGrowBy);
	m_kTopObjects.SetAt(uiIndex, spObject);
}

void NiStream::RemoveObject(NiObject* pkObject)
{
	uint32_t uiSize = m_kTopObjects.m_uiSize;
	for (uint32_t i = 0; i < uiSize; ++i)
	{
		if (m_kTopObjects.m_pBase[i] == pkObject)
		{
			m_kTopObjects.RemoveAt(i);
			return;
		}
	}
}

void NiStream::RemoveAllObjects()
{
	EnterCriticalSection(&ms_kCleanupCriticalSection);
	m_kTopObjects.RemoveAll();
	for (uint32_t i = 0; i < m_kFixedStrings.m_uiSize; ++i)
		m_kFixedStrings.m_pBase[i] = BSFixedString(nullptr);
	m_kFixedStrings.m_uiSize = 0;
	m_kFixedStrings.m_uiESize = 0;
	for (uint32_t i = 0; i < m_kObjectSizes.m_uiSize; ++i)
		m_kObjectSizes.m_pBase[i] = 0;
	m_kObjectSizes.m_uiSize = 0;
	m_kObjectSizes.m_uiESize = 0;
	LeaveCriticalSection(&ms_kCleanupCriticalSection);
}

bool NiStream::RegisterFixedString(const BSFixedString& kString)
{
	uint32_t uiSize = m_kFixedStrings.m_uiSize;
	if (kString.pString)
	{
		for (uint32_t i = 0; i < uiSize; ++i)
			if (kString.pString == m_kFixedStrings.m_pBase[i].pString)
				return true;
	}
	bool bEmpty;
	{
		BSFixedString kEmpty(nullptr);
		bEmpty = kString.pString == kEmpty.pString;
	}
	if (!bEmpty)
	{
		uiSize = m_kFixedStrings.m_uiSize;
		for (uint32_t i = 0; i < uiSize; ++i)
		{
			bool bSlotEmpty;
			{
				BSFixedString kEmpty(nullptr);
				bSlotEmpty = m_kFixedStrings.m_pBase[i].pString == kEmpty.pString;
			}
			if (bSlotEmpty)
			{
				m_kFixedStrings.m_pBase[i] = kString;
				++m_kFixedStrings.m_uiESize;
				return true;
			}
			uiSize = m_kFixedStrings.m_uiSize;
		}
		if (uiSize >= m_kFixedStrings.m_uiMaxSize)
			m_kFixedStrings.SetSize(uiSize + m_kFixedStrings.m_uiGrowBy);
		m_kFixedStrings.SetAt(uiSize, kString);
	}
	return true;
}

bool NiStream::RegisterSaveObject(NiObject* pkObject)
{
	NiTMapBase<NiTPointerAllocator<uint64_t>, const NiObject*, uint32_t>* volatile pkMap = &m_kRegisterMap;
	uint32_t uiHash = pkMap->KeyToHashIndex(pkObject);
	NiTMapItem<const NiObject*, uint32_t>* pkItem = m_kRegisterMap.m_ppkHashTable[uiHash];
	while (pkItem)
	{
		if (pkMap->IsKeysEqual(pkObject, pkItem->m_key))
			return false;
		pkItem = pkItem->m_pkNext;
	}
	uint32_t uiID = m_kObjects.m_uiSize;
	uiHash = pkMap->KeyToHashIndex(pkObject);
	pkItem = m_kRegisterMap.m_ppkHashTable[uiHash];
	while (pkItem && !pkMap->IsKeysEqual(pkObject, pkItem->m_key))
		pkItem = pkItem->m_pkNext;
	if (pkItem)
		pkItem->m_val = uiID;
	else
	{
		pkItem = pkMap->NewItem();
		pkMap->SetValue(pkItem, pkObject, uiID);
		pkItem->m_pkNext = m_kRegisterMap.m_ppkHashTable[uiHash];
		m_kRegisterMap.m_ppkHashTable[uiHash] = pkItem;
		++m_kRegisterMap.m_uiCount;
	}
	NiPointer<NiObject> spObject(pkObject);
	uint32_t uiIndex = m_kObjects.m_uiSize;
	if (uiIndex >= m_kObjects.m_uiMaxSize)
		m_kObjects.SetSize(uiIndex + m_kObjects.m_uiGrowBy);
	m_kObjects.SetAt(uiIndex, spObject);
	return true;
}

void NiStream::LoadTopLevelObjects()
{
	uint32_t uiSize;
	NiStreamLoadBinary(*this, uiSize);
	m_kTopObjects.SetSize(uiSize);
	for (uint32_t i = 0; i < uiSize; ++i)
	{
		uint32_t uiID;
		NiStreamLoadBinary(*this, uiID);
		NiPointer<NiObject> spObject(uiID == UINT32_MAX ? nullptr : m_kObjects.m_pBase[uiID].m_pObject);
		m_kTopObjects.SetAt(i, spObject);
	}
}

int NiStream::RegisterLoader(const char* pcRTTI, LoadFunction pfnLoad)
{
	auto* pkMap = ms_pkLoaders;
	uint32_t uiHash = pkMap->KeyToHashIndex(pcRTTI);
	auto* pkItem = pkMap->m_ppkHashTable[uiHash];
	while (pkItem && !pkMap->IsKeysEqual(pcRTTI, pkItem->m_key))
		pkItem = pkItem->m_pkNext;
	if (pkItem)
	{
		if (!pkMap->m_bCopy)
			pkItem->m_key = pcRTTI;
		pkItem->m_val = pfnLoad;
	}
	else
	{
		using Item = NiTMapItem<const char*, LoadFunction>;
		pkItem = static_cast<Item*>(MemoryManager::Instance().Allocate(sizeof(Item), 0, false));
		if (pkItem)
			memset(pkItem, 0, sizeof(Item));
		pkMap->SetValue(pkItem, pcRTTI, pfnLoad);
		pkItem->m_pkNext = pkMap->m_ppkHashTable[uiHash];
		pkMap->m_ppkHashTable[uiHash] = pkItem;
		++pkMap->m_uiCount;
	}
	return 0;
}

void NiStream::UnregisterLoader(const char* pcRTTI)
{
	auto* pkMap = ms_pkLoaders;
	uint32_t uiHash = pkMap->KeyToHashIndex(pcRTTI);
	auto* pkItem = pkMap->m_ppkHashTable[uiHash];
	if (!pkItem)
		return;
	if (pkMap->IsKeysEqual(pcRTTI, pkItem->m_key))
	{
		pkMap->m_ppkHashTable[uiHash] = pkItem->m_pkNext;
		pkMap->ClearValue(pkItem);
		pkMap->DeleteItem(pkItem);
		--pkMap->m_uiCount;
		return;
	}
	auto* pkPrevious = pkItem;
	pkItem = pkItem->m_pkNext;
	while (pkItem)
	{
		bool bMatch = pkMap->IsKeysEqual(pcRTTI, pkItem->m_key);
		auto* pkNext = pkItem->m_pkNext;
		if (bMatch)
		{
			pkPrevious->m_pkNext = pkNext;
			pkMap->ClearValue(pkItem);
			pkMap->DeleteItem(pkItem);
			--pkMap->m_uiCount;
			return;
		}
		pkPrevious = pkItem;
		pkItem = pkNext;
	}
}

NiObject* NiStream::CreateObjectByRTTI(const char* pcRTTI)
{
	auto* pkMap = ms_pkLoaders;
	uint32_t uiHash = pkMap->KeyToHashIndex(pcRTTI);
	auto* pkItem = pkMap->m_ppkHashTable[uiHash];
	while (pkItem)
	{
		if (pkMap->IsKeysEqual(pcRTTI, pkItem->m_key))
			return pkItem->m_val();
		pkItem = pkItem->m_pkNext;
	}
	return nullptr;
}

void NiStream::RegisterPostProcessFunction(PostProcessFunction pfnFunction)
{
	ms_pkPostProcessFunctions->AddFirstEmpty(pfnFunction);
}

void NiStream::UnregisterPostProcessFunction(PostProcessFunction pfnFunction)
{
	auto* pkFunctions = ms_pkPostProcessFunctions;
	uint32_t uiSize = pkFunctions->m_usSize;
	for (uint32_t i = 0; i < uiSize; ++i)
	{
		if (pkFunctions->m_pBase[i] == pfnFunction)
		{
			pkFunctions->m_pBase[i] = nullptr;
			if (pfnFunction)
				--pkFunctions->m_usESize;
			uint16_t usLast = pkFunctions->m_usSize - 1;
			if (i == usLast)
				pkFunctions->m_usSize = usLast;
			return;
		}
	}
}

void NiStream::FreeLoadData()
{
	EnterCriticalSection(&ms_kCleanupCriticalSection);
	m_kObjects.RemoveAll();
	m_uiLinkIndex = 0;
	m_uiLinkBlockIndex = 0;
	LeaveCriticalSection(&ms_kCleanupCriticalSection);
}

void NiStream::SetSelectiveUpdateFlagsForOldVersions() {}

void NiStream::LoadRTTIString(char* pcString)
{
	uint32_t uiLength;
	NiStreamLoadBinary(*this, uiLength);
	NiStreamLoadBinary(*this, pcString, uiLength);
	pcString[uiLength] = 0;
}

void NiStream::RTTIError(const char* pcRTTI)
{
	m_uiLastError = 5;
	strcpy_s(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), pcRTTI);
	strcat_s(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), ": cannot find create function.");
}

void NiStream::SaveFixedStringTable()
{
	uint32_t uiSize = m_kFixedStrings.m_uiSize;
	NiStreamSaveBinary(*this, uiSize);
	uint32_t uiMaxLength = 0;
	for (uint16_t i = 0; i < m_kFixedStrings.m_uiSize; ++i)
	{
		uint32_t uiLength = m_kFixedStrings.m_pBase[i].QLength();
		if (uiLength > uiMaxLength)
			uiMaxLength = uiLength;
	}
	NiStreamSaveBinary(*this, uiMaxLength);
	for (uint16_t i = 0; i < m_kFixedStrings.m_uiSize; ++i)
	{
		BSFixedString* pkString = &m_kFixedStrings.m_pBase[i];
		uint32_t uiLength = pkString->QLength();
		const char* pcString = pkString->pString;
		NiStreamSaveBinary(*this, uiLength);
		NiStreamSaveBinary(*this, pcString, uiLength);
	}
}

bool NiStream::LoadFixedStringTable()
{
	uint32_t uiSize;
	NiStreamLoadBinary(*this, uiSize);
	m_kFixedStrings.SetSize(uiSize);
	uint32_t uiMaxLength;
	NiStreamLoadBinary(*this, uiMaxLength);
	MemoryManager::AutoScrapBuffer kBuffer(static_cast<uint32_t>(uiMaxLength + 1), 8);
	char* pcString = static_cast<char*>(kBuffer.QPtr());
	for (uint16_t i = 0; i < uiSize; ++i)
	{
		uint32_t uiLength = 0;
		NiStreamLoadBinary(*this, uiLength);
		NiStreamLoadBinary(*this, pcString, uiLength);
		pcString[uiLength] = 0;
		BSFixedString kString(pcString);
		bool bEmpty;
		if (i < m_kFixedStrings.m_uiSize)
		{
			{
				BSFixedString kEmpty(nullptr);
				bEmpty = kString.pString == kEmpty.pString;
			}
			bool bSlotEmpty;
			{
				BSFixedString kEmpty(nullptr);
				bSlotEmpty = m_kFixedStrings.m_pBase[i].pString == kEmpty.pString;
			}
			if (bEmpty)
			{
				if (!bSlotEmpty)
					--m_kFixedStrings.m_uiESize;
			}
			else if (bSlotEmpty)
				++m_kFixedStrings.m_uiESize;
		}
		else
		{
			m_kFixedStrings.m_uiSize = i + 1;
			{
				BSFixedString kEmpty(nullptr);
				bEmpty = kString.pString == kEmpty.pString;
			}
			if (!bEmpty)
				++m_kFixedStrings.m_uiESize;
		}
		m_kFixedStrings.m_pBase[i] = kString;
	}
	return true;
}

void NiStream::LoadObjectGroups()
{
	uint32_t uiGroups;
	NiStreamLoadBinary(*this, uiGroups);
	++uiGroups;
	m_kGroups.SetAllocSize(uiGroups);
	BSTArrayAllocatorFunctor<BSTSmallArrayHeapAllocator<32>> kFunctor(&m_kGroups);
	uint32_t uiIndex = m_kGroups.BSTArrayBase::AddUninitialized(kFunctor, m_kGroups.QAllocSize(), sizeof(NiObjectGroup*));
	m_kGroups[uiIndex] = nullptr;
	for (uint32_t i = 1; i < uiGroups; ++i)
	{
		uint32_t uiSize;
		NiStreamLoadBinary(*this, uiSize);
		void* pvMemory = MemoryManager::Instance().Allocate(sizeof(NiObjectGroup), 0, false);
		NiObjectGroup* pkGroup = pvMemory ? new (pvMemory) NiObjectGroup(uiSize) : nullptr;
		m_kGroups.Add(pkGroup);
	}
}

void NiStream::SaveObjectGroups()
{
	uint32_t uiGroups = m_kGroups.iSize - 1;
	NiStreamSaveBinary(*this, uiGroups);
	for (uint32_t i = 1; i < m_kGroups.iSize; ++i)
	{
		uint32_t uiSize = m_kGroups[i]->m_uiSize;
		NiStreamSaveBinary(*this, uiSize);
	}
}

void NiStream::UpdateObjectGroups()
{
	m_kGroups.Clear(false);
	m_kGroups.SetAllocSize(1);
	BSTArrayAllocatorFunctor<BSTSmallArrayHeapAllocator<32>> kFunctor(&m_kGroups);
	uint32_t uiIndex = m_kGroups.BSTArrayBase::AddUninitialized(kFunctor, m_kGroups.QAllocSize(), sizeof(NiObjectGroup*));
	m_kGroups[uiIndex] = nullptr;
	for (uint32_t i = 0; i < m_kObjects.m_uiSize; ++i)
	{
		NiObject* pkObject = m_kObjects.m_pBase[i];
		if (!pkObject)
			continue;
		NiObjectGroup* pkGroup = pkObject->GetGroup();
		if (!pkGroup)
			continue;
		uint32_t uiGroups = m_kGroups.iSize;
		uint32_t uiGroup = 0;
		while (uiGroup < uiGroups && m_kGroups[uiGroup] != pkGroup)
			++uiGroup;
		if (!uiGroup || uiGroup == uiGroups)
		{
			m_kGroups.Add(pkGroup);
			pkGroup->m_uiSize = 0;
		}
	}
	for (uint32_t i = 0; i < m_kObjects.m_uiSize; ++i)
	{
		NiObject* pkObject = m_kObjects.m_pBase[i];
		if (pkObject)
		{
			NiObjectGroup* pkGroup = pkObject->GetGroup();
			if (pkGroup)
			{
				uint32_t uiSize = pkGroup->m_uiSize;
				pkGroup->m_uiSize = uiSize + pkObject->GetBlockAllocationSize();
			}
		}
	}
}

bool NiStream::LoadRTTI()
{
	uint16_t usTypes;
	NiStreamLoadBinary(*this, usTypes);
	MemoryManager::AutoScrapBuffer kFunctions(sizeof(LoadFunction) * usTypes, 8);
	auto* ppfnFunctions = static_cast<LoadFunction*>(kFunctions.QPtr());
	for (uint32_t i = 0; i < usTypes; ++i)
	{
		char acRTTI[320];
		LoadRTTIString(acRTTI);
		auto* pkMap = ms_pkLoaders;
		uint32_t uiHash = pkMap->KeyToHashIndex(acRTTI);
		auto* pkItem = pkMap->m_ppkHashTable[uiHash];
		while (pkItem && !pkMap->IsKeysEqual(acRTTI, pkItem->m_key))
			pkItem = pkItem->m_pkNext;
		if (pkItem)
			ppfnFunctions[i] = pkItem->m_val;
		else
		{
			RTTIError(acRTTI);
			ppfnFunctions[i] = nullptr;
		}
	}
	for (uint32_t i = 0; i < m_kObjects.m_uiMaxSize; ++i)
	{
		uint16_t usType;
		NiStreamLoadBinary(*this, usType);
		bool bCanSkip = (usType & 0x8000) != 0;
		LoadFunction pfnLoad = ppfnFunctions[usType & 0x7fff];
		if (pfnLoad)
		{
			NiPointer<NiObject> spObject(pfnLoad());
			m_kObjects.SetAt(i, spObject);
		}
		else
		{
			if (!bCanSkip)
				return false;
			NiPointer<NiObject> spObject;
			m_kObjects.SetAt(i, spObject);
		}
	}
	return true;
}

bool NiStream::LoadObject()
{
	char acRTTI[296];
	LoadRTTIString(acRTTI);
	auto* pkMap = ms_pkLoaders;
	uint32_t uiHash = pkMap->KeyToHashIndex(acRTTI);
	auto* pkItem = pkMap->m_ppkHashTable[uiHash];
	while (pkItem && !pkMap->IsKeysEqual(acRTTI, pkItem->m_key))
		pkItem = pkItem->m_pkNext;
	if (!pkItem)
	{
		RTTIError(acRTTI);
		return false;
	}
	NiObject* pkObject = pkItem->m_val();
	{
		NiPointer<NiObject> spObject(pkObject);
		uint32_t uiIndex = m_kObjects.m_uiSize;
		if (uiIndex >= m_kObjects.m_uiMaxSize)
			m_kObjects.SetSize(uiIndex + m_kObjects.m_uiGrowBy);
		m_kObjects.SetAt(uiIndex, spObject);
	}
	pkObject->LoadBinary(*this);
	return true;
}
void NiStream::SaveRTTI()
{
	NiTStringPointerMap<uint16_t> kTypes(37, false);
	auto FindType = [&kTypes](const char* pcName)
	{
		uint32_t uiHash = kTypes.KeyToHashIndex(pcName);
		auto* pkItem = kTypes.m_ppkHashTable[uiHash];
		while (pkItem && !kTypes.IsKeysEqual(pcName, pkItem->m_key))
			pkItem = pkItem->m_pkNext;
		return pkItem;
	};
	for (uint32_t i = 0; i < m_kObjects.m_uiSize; ++i)
	{
		const char* pcName = m_kObjects.m_pBase[i]->GetStreamableRTTI()->GetName();
		if (FindType(pcName))
			continue;
		uint16_t usType = static_cast<uint16_t>(kTypes.m_uiCount);
		uint32_t uiHash = kTypes.KeyToHashIndex(pcName);
		auto* pkItem = kTypes.m_ppkHashTable[uiHash];
		while (pkItem && !kTypes.IsKeysEqual(pcName, pkItem->m_key))
			pkItem = pkItem->m_pkNext;
		if (pkItem)
		{
			if (!kTypes.m_bCopy)
				pkItem->m_key = pcName;
			pkItem->m_val = usType;
		}
		else
		{
			pkItem = static_cast<NiTMapItem<const char*, uint16_t>*>(MemoryManager::Instance().Allocate(sizeof(*pkItem), 0, false));
			if (pkItem)
				memset(pkItem, 0, sizeof(*pkItem));
			kTypes.SetValue(pkItem, pcName, usType);
			pkItem->m_pkNext = kTypes.m_ppkHashTable[uiHash];
			kTypes.m_ppkHashTable[uiHash] = pkItem;
			++kTypes.m_uiCount;
		}
	}
	uint16_t usTypeCount = static_cast<uint16_t>(kTypes.m_uiCount);
	auto** ppcNames = static_cast<const char**>(_NiMalloc(sizeof(const char*) * usTypeCount));
	uint32_t uiBucket = 0;
	while (uiBucket < kTypes.m_uiHashSize && !kTypes.m_ppkHashTable[uiBucket])
		++uiBucket;
	auto* pkItem = uiBucket < kTypes.m_uiHashSize ? kTypes.m_ppkHashTable[uiBucket] : nullptr;
	while (pkItem)
	{
		const char* pcName = pkItem->m_key;
		uint16_t usType = pkItem->m_val;
		pkItem = pkItem->m_pkNext;
		if (!pkItem)
		{
			uiBucket = kTypes.KeyToHashIndex(pcName) + 1;
			while (uiBucket < kTypes.m_uiHashSize && !kTypes.m_ppkHashTable[uiBucket])
				++uiBucket;
			pkItem = uiBucket < kTypes.m_uiHashSize ? kTypes.m_ppkHashTable[uiBucket] : nullptr;
		}
		ppcNames[usType] = pcName;
	}
	NiStreamSaveBinary(*this, usTypeCount);
	uint32_t uiLength = 2;
	for (uint32_t i = 0; i < usTypeCount; ++i)
	{
		const char* pcName = ppcNames[i];
		uiLength = pcName ? static_cast<uint32_t>(strlen(pcName)) : 0;
		NiStreamSaveBinary(*this, uiLength);
		if (uiLength)
			NiStreamSaveBinary(*this, pcName, uiLength);
	}
	_NiFree(ppcNames);
	for (uint32_t i = 0; i < m_kObjects.m_uiSize; ++i)
	{
		NiObject* pkObject = m_kObjects.m_pBase[i];
		const char* pcName = pkObject->GetStreamableRTTI()->GetName();
		pkItem = FindType(pcName);
		if (pkItem)
			uiLength = (uiLength & 0xffff0000) | pkItem->m_val;
		if (pkObject->StreamCanSkip())
			uiLength |= 0x8000;
		uint16_t usType = static_cast<uint16_t>(uiLength);
		NiStreamSaveBinary(*this, usType);
	}
}

NiStream::NiStream() :
	m_kObjects(0, 1024), m_kObjectSizes(0, 1), m_kTopObjects(0, 1), m_kFixedStrings(0, 128)
{
	m_pkThread = nullptr;
	m_pkBackgroundLoadProcedure = nullptr;
	m_uiBackgroundLoadPriority = 2;
	m_uiBackgroundLoadProcessor = 0xffffffff;
	m_uiBackgroundLoadThread = 0xffffffff;
	m_uiLastError = 0;
	m_acLastErrorMessage[0] = 0;
	m_pkIstr = nullptr;
	m_pkOstr = nullptr;
	m_uiNifFileVersion = ms_uiNifMaxVersion;
	m_uiNifFileUserDefinedVersion = ms_uiNifMaxUserDefinedVersion;
	m_bSaveLittleEndian = m_bSourceIsLittleEndian = true;
	m_acFileName[0] = 0;
	m_uiLinkIndex = m_uiLinkBlockIndex = 0;
	bUseDefaultPath = true;
	m_uiBackgroundLoadStatus = 0;
	m_uiLastError = 0;
	m_acLastErrorMessage[0] = 0;
	m_pkSearchPath = nullptr;
	m_usNiAVObjectFlags = m_usNiTimeControllerFlags = m_usNiPropertyFlags = 0;
	m_bBackgroundLoadResult = false;
	m_uiLoad = m_uiLink = m_uiPostLink = 0;
	memset(m_acLastLoadedRTTI, 0, sizeof(m_acLastLoadedRTTI));
	memset(m_acFilePath, 0, sizeof(m_acFilePath));
}

NiStream::~NiStream()
{
	delete m_pkIstr;
	delete m_pkOstr;
	RemoveAllObjects();
	delete static_cast<NiThreadProcedure*>(m_pkBackgroundLoadProcedure);
}
