#pragma once

#include "NiTMapBase.h"
#include "BSCore/MemoryManager.h"
#include <new>

template<class Key, class Value>
class NiTPointerMap : public NiTMapBase<NiTPointerAllocator<uint64_t>, Key, Value>
{
public:
	NiTPointerMap(uint32_t uiHashSize = 37) :
		NiTMapBase<NiTPointerAllocator<uint64_t>, Key, Value>(uiHashSize) {}
	~NiTPointerMap() override;
	NiTMapItem<Key, Value>* NewItem() override;
	void DeleteItem(NiTMapItem<Key, Value>* pkItem) override;
};
