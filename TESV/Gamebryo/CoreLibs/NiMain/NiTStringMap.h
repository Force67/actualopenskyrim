#pragma once

#include "NiTPointerMap.h"
#include "Gamebryo/CoreLibs/NiSystem/NiSystem.h"

template<class Parent, class Value>
class NiTStringTemplateMap : public Parent
{
public:
	NiTStringTemplateMap(uint32_t uiHashSize, bool bCopy) : Parent(uiHashSize), m_bCopy(bCopy) {}
	~NiTStringTemplateMap() override;
	uint32_t KeyToHashIndex(const char* pcKey) const override;
	bool IsKeysEqual(const char* pcKey1, const char* pcKey2) const override;
	void SetValue(NiTMapItem<const char*, Value>* pkItem, const char* pcKey, Value value) override;
	void ClearValue(NiTMapItem<const char*, Value>* pkItem) override;

	bool m_bCopy;
};

template<class Value>
class NiTStringPointerMap : public NiTStringTemplateMap<NiTPointerMap<const char*, Value>, Value>
{
public:
	NiTStringPointerMap(uint32_t uiHashSize, bool bCopy) :
		NiTStringTemplateMap<NiTPointerMap<const char*, Value>, Value>(uiHashSize, bCopy) {}
	~NiTStringPointerMap() override = default;
};

static_assert(sizeof(NiTStringPointerMap<uint32_t>) == 40);
