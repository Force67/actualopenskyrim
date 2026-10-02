#pragma once

#include <cstdint>
#include <cstring>
#include "Gamebryo/CoreLibs/NiSystem/NiMemoryDefines.h"

template<class T> class NiTPointerAllocator;

template<class Key, class Value>
class NiTMapItem
{
public:
	NiTMapItem* m_pkNext;
	Key m_key;
	Value m_val;
};

template<class Allocator, class Key, class Value>
class NiTMapBase
{
public:
	NiTMapBase(uint32_t uiHashSize = 37) :
		m_uiHashSize(uiHashSize), m_uiCount(0)
	{
		m_ppkHashTable = static_cast<NiTMapItem<Key, Value>**>(_NiMalloc(sizeof(NiTMapItem<Key, Value>*) * uiHashSize));
		memset(m_ppkHashTable, 0, sizeof(NiTMapItem<Key, Value>*) * m_uiHashSize);
	}
	virtual ~NiTMapBase();
	virtual uint32_t KeyToHashIndex(Key key) const;
	virtual bool IsKeysEqual(Key key1, Key key2) const;
	virtual void SetValue(NiTMapItem<Key, Value>* pkItem, Key key, Value val);
	virtual void ClearValue(NiTMapItem<Key, Value>*);
	virtual NiTMapItem<Key, Value>* NewItem() = 0;
	virtual void DeleteItem(NiTMapItem<Key, Value>* pkItem) = 0;

	void RemoveAll();
	uint32_t m_uiHashSize;
	NiTMapItem<Key, Value>** m_ppkHashTable;
	uint32_t m_uiCount;
	uint32_t m_uiPadding;
};
