#include "NiStream.h"
#include "Gamebryo/CoreLibs/NiSystem/NiBinaryStream.h"

bool NiStream::SaveStream()
{
	RegisterObjects();
	m_kObjectSizes.SetSize(m_kObjects.m_uiSize);
	SaveHeader();
	SaveRTTI();
	uint32_t uiSizeTablePosition = PreSaveObjectSizeTable();
	SaveFixedStringTable();
	UpdateObjectGroups();
	SaveObjectGroups();
	for (uint32_t i = 0; i < m_kObjects.m_uiSize; ++i)
	{
		NiObject* pkObject = m_kObjects.m_pBase[i];
		uint32_t uiPosition = m_pkOstr->GetPosition();
		pkObject->SaveBinary(*this);
		uint32_t uiSize = m_pkOstr->GetPosition() - uiPosition;
		m_kObjectSizes.SetAt(i, uiSize);
	}
	SaveTopLevelObjects();
	SaveObjectSizeTable(uiSizeTablePosition);
	m_kObjects.RemoveAll();
	m_kRegisterMap.RemoveAll();
	return true;
}
