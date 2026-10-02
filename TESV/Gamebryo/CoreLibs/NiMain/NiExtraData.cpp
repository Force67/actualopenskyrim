#include "NiExtraData.h"

NiExtraData::NiExtraData()
{
}

NiExtraData::NiExtraData(const BSFixedString& kName)
{
	m_kName = kName;
}

NiExtraData::~NiExtraData()
{
}

const NiRTTI* NiExtraData::GetRTTI() const
{
	return &ms_RTTI;
}

bool NiExtraData::IsEqual(NiObject* pkObject)
{
	return NiObject::IsEqual(pkObject) && m_kName.pString == static_cast<NiExtraData*>(pkObject)->m_kName.pString;
}

void NiExtraData::LinkObject(NiStream&) {}
bool NiExtraData::IsStreamable() const { return true; }
bool NiExtraData::IsCloneable() const { return true; }

const BSFixedString& NiExtraData::GetName() const
{
	return m_kName;
}

void NiExtraData::SetName(const BSFixedString& kName)
{
	m_kName = kName;
}

void NiExtraData::ClearName()
{
	m_kName.pString = nullptr;
}
