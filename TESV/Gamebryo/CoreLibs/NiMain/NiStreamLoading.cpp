#include "NiStream.h"
#include "NiSkinPartition.h"
#include "Gamebryo/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo/CoreLibs/NiSystem/NiThread.h"
#include <cstring>

bool NiStream::LoadStream()
{
	BSScrapArray<uint32_t> kLinkIDs(64);
	BSScrapArray<uint32_t> kLinkBlocks(64);
	pLinkIDAS = &kLinkIDs;
	pLinkIDBlocksAS = &kLinkBlocks;
	if (!LoadHeader())
		return false;
	m_uiLink = m_uiPostLink = 0;
	m_uiLoad = 0;
	RemoveAllObjects();
	if (!LoadRTTI() || !LoadObjectSizeTable() || !LoadFixedStringTable())
		return false;
	LoadObjectGroups();
	uint32_t uiObjects = m_kObjects.m_uiMaxSize;
	auto ClearObjects = [this]()
	{
		EnterCriticalSection(&ms_kCleanupCriticalSection);
		m_kObjects.RemoveAll();
		m_uiLinkIndex = m_uiLinkBlockIndex = 0;
		LeaveCriticalSection(&ms_kCleanupCriticalSection);
	};
	for (uint32_t i = m_uiLoad; i < uiObjects; m_uiLoad = i)
	{
		if (m_uiBackgroundLoadStatus == CANCELLING)
		{
			ClearObjects();
			return false;
		}
		if (m_uiBackgroundLoadStatus == PAUSING)
		{
			m_uiBackgroundLoadStatus = PAUSED;
			m_pkThread->Suspend();
			i = m_uiLoad;
		}
		NiObject* pkObject = m_kObjects.m_pBase[i];
		if (pkObject)
		{
			pkObject->LoadBinary(*this);
			const NiRTTI* pkRTTI = pkObject->GetRTTI();
			while (pkRTTI && pkRTTI != &NiSkinPartition::ms_RTTI)
				pkRTTI = pkRTTI->m_pkBaseRTTI;
			if (pkRTTI)
			{
				auto* pkSkin = static_cast<NiSkinPartition*>(pkObject);
				bool bHasStrips = false;
				for (uint32_t j = 0; j < pkSkin->m_uiPartitions; ++j)
					bHasStrips |= pkSkin->m_pkPartitions[j].GetStripLengthSum() != 0;
				if (bHasStrips)
				{
					m_uiLastError = 1;
					strcpy_s(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), "Skin has stripped partition. This is not supported.");
					FreeLoadData();
					return false;
				}
			}
		}
		else
		{
			m_pkIstr->GetPosition();
			m_pkIstr->Seek(static_cast<int32_t>(m_kObjectSizes.m_pBase[m_uiLoad]));
		}
		i = m_uiLoad + 1;
	}
	LoadTopLevelObjects();
	for (uint32_t i = m_uiLink; i < uiObjects; m_uiLink = i)
	{
		if (m_uiBackgroundLoadStatus == PAUSING)
		{
			m_uiBackgroundLoadStatus = PAUSED;
			m_pkThread->Suspend();
			i = m_uiLink;
		}
		NiObject* pkObject = m_kObjects.m_pBase[i];
		if (pkObject)
		{
			pkObject->LinkObject(*this);
			i = m_uiLink;
		}
		++i;
	}
	for (uint32_t i = m_uiPostLink; i < uiObjects; m_uiPostLink = i)
	{
		if (m_uiBackgroundLoadStatus == PAUSING)
		{
			m_uiBackgroundLoadStatus = PAUSED;
			m_pkThread->Suspend();
			i = m_uiPostLink;
		}
		NiObject* pkObject = m_kObjects.m_pBase[i];
		if (pkObject)
		{
			pkObject->PostLinkObject(*this);
			i = m_uiPostLink;
		}
		++i;
	}
	if (m_uiBackgroundLoadStatus == CANCELLING)
	{
		ClearObjects();
		return false;
	}
	if (m_uiBackgroundLoadStatus == PAUSING)
	{
		m_uiBackgroundLoadStatus = PAUSED;
		m_pkThread->Suspend();
	}
	auto* pkFunctions = ms_pkPostProcessFunctions;
	if (pkFunctions->m_usESize)
	{
		for (uint32_t i = 0; i < m_kTopObjects.m_uiSize; ++i)
		{
			NiObject* pkObject = m_kTopObjects.m_pBase[i];
			if (pkObject)
			{
				for (uint32_t j = 0; j < pkFunctions->m_usSize; ++j)
				{
					PostProcessFunction pfnFunction = pkFunctions->m_pBase[j];
					if (pfnFunction)
					{
						pfnFunction(*this, pkObject);
						pkFunctions = ms_pkPostProcessFunctions;
					}
				}
			}
		}
	}
	bool bResult = m_uiBackgroundLoadStatus != CANCELLING;
	ClearObjects();
	return bResult;
}

// These views cover the streaming fields until the scene classes are defined.
struct NiAVObjectStreamFlags
{
	std::byte m_acData[244];
	uint32_t m_uiFlags;
};
struct NiNodeStreamChildren
{
	std::byte m_acData[280];
	NiAVObject** m_ppChildren;
	uint16_t m_usMaxSize;
	uint16_t m_usSize;
};
static_assert(offsetof(NiAVObjectStreamFlags, m_uiFlags) == 244);
static_assert(offsetof(NiNodeStreamChildren, m_usSize) == 290);

void NiStream::SetSelectiveUpdateFlagsTTTFRecursive(NiAVObject* pkObject)
{
	auto* pkFlags = reinterpret_cast<NiAVObjectStreamFlags*>(pkObject);
	pkFlags->m_uiFlags = (pkFlags->m_uiFlags & 0xffffffe1) | 0x0e;
	auto* pkNode = reinterpret_cast<NiNodeStreamChildren*>(reinterpret_cast<NiObject*>(pkObject)->IsNode());
	if (pkNode)
	{
		for (uint32_t i = 0; i < pkNode->m_usSize; ++i)
		{
			NiAVObject* pkChild = pkNode->m_ppChildren[i];
			if (pkChild)
				SetSelectiveUpdateFlagsTTTFRecursive(pkChild);
		}
	}
}
