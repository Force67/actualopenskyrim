#include "NiStream.h"

template<class Allocator, class Key, class Value>
NiTMapBase<Allocator, Key, Value>::~NiTMapBase()
{
	RemoveAll();
	_NiFree(m_ppkHashTable);
}

template<class Allocator, class Key, class Value>
uint32_t NiTMapBase<Allocator, Key, Value>::KeyToHashIndex(Key key) const
{
	return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(key) % m_uiHashSize);
}

template<class Allocator, class Key, class Value>
bool NiTMapBase<Allocator, Key, Value>::IsKeysEqual(Key key1, Key key2) const
{ return key1 == key2; }

template<class Allocator, class Key, class Value>
void NiTMapBase<Allocator, Key, Value>::SetValue(NiTMapItem<Key, Value>* pkItem, Key key, Value val)
{
	pkItem->m_key = key;
	pkItem->m_val = val;
}

template<class Allocator, class Key, class Value>
void NiTMapBase<Allocator, Key, Value>::ClearValue(NiTMapItem<Key, Value>*)
{}

template<class Allocator, class Key, class Value>
void NiTMapBase<Allocator, Key, Value>::RemoveAll()
{
	for (uint32_t i = 0; i < m_uiHashSize; ++i)
	{
		while (m_ppkHashTable[i])
		{
			NiTMapItem<Key, Value>* pkItem = m_ppkHashTable[i];
			m_ppkHashTable[i] = pkItem->m_pkNext;
			ClearValue(pkItem);
			DeleteItem(pkItem);
		}
	}
	m_uiCount = 0;
}

template<class Key, class Value>
NiTPointerMap<Key, Value>::~NiTPointerMap()
{ this->RemoveAll(); }

template<class Key, class Value>
NiTMapItem<Key, Value>* NiTPointerMap<Key, Value>::NewItem()
{
	auto* pkItem = static_cast<NiTMapItem<Key, Value>*>(MemoryManager::Instance().Allocate(sizeof(NiTMapItem<Key, Value>), 0, false));
	if (pkItem)
		memset(pkItem, 0, sizeof(NiTMapItem<Key, Value>));
	return pkItem;
}

template<class Key, class Value>
void NiTPointerMap<Key, Value>::DeleteItem(NiTMapItem<Key, Value>* pkItem)
{
	pkItem->m_val = Value();
	::operator delete(pkItem, sizeof(NiTMapItem<Key, Value>));
}

template<class Parent, class Value>
NiTStringTemplateMap<Parent, Value>::~NiTStringTemplateMap()
{
	if (m_bCopy)
	{
		for (uint32_t i = 0; i < this->m_uiHashSize; ++i)
		{
			auto* pkItem = this->m_ppkHashTable[i];
			while (pkItem)
			{
				const char* pcKey = pkItem->m_key;
				pkItem = pkItem->m_pkNext;
				_NiFree(const_cast<char*>(pcKey));
			}
		}
	}
}

template<class Parent, class Value>
uint32_t NiTStringTemplateMap<Parent, Value>::KeyToHashIndex(const char* pcKey) const
{
	uint32_t uiHash = 0;
	while (*pcKey)
		uiHash = static_cast<int8_t>(*pcKey++) + 33 * uiHash;
	return uiHash % this->m_uiHashSize;
}

template<class Parent, class Value>
bool NiTStringTemplateMap<Parent, Value>::IsKeysEqual(const char* pcKey1, const char* pcKey2) const
{
	return NiStricmp(pcKey1, pcKey2) == 0;
}

template<class Parent, class Value>
void NiTStringTemplateMap<Parent, Value>::SetValue(NiTMapItem<const char*, Value>* pkItem, const char* pcKey, Value value)
{
	if (m_bCopy)
	{
		size_t stLength = strlen(pcKey) + 1;
		char* pcCopy = static_cast<char*>(_NiMalloc(stLength));
		pkItem->m_key = pcCopy;
		strcpy_s(pcCopy, stLength, pcKey);
		pkItem->m_val = value;
	}
	else
	{
		pkItem->m_val = value;
		pkItem->m_key = pcKey;
	}
}

template<class Parent, class Value>
void NiTStringTemplateMap<Parent, Value>::ClearValue(NiTMapItem<const char*, Value>* pkItem)
{
	if (m_bCopy)
		_NiFree(const_cast<char*>(pkItem->m_key));
}

using ObjectLoader = NiObject* (*)();

template class NiTMapBase<NiTPointerAllocator<uint64_t>, const NiObject*, uint32_t>;
template class NiTPointerMap<const NiObject*, uint32_t>;
template class NiTMapBase<NiTPointerAllocator<uint64_t>, const char*, uint16_t>;
template class NiTPointerMap<const char*, uint16_t>;
template class NiTStringTemplateMap<NiTPointerMap<const char*, uint16_t>, uint16_t>;
template class NiTStringPointerMap<uint16_t>;
template class NiTMapBase<NiTPointerAllocator<uint64_t>, const char*, ObjectLoader>;
template class NiTPointerMap<const char*, ObjectLoader>;
template class NiTStringTemplateMap<NiTPointerMap<const char*, ObjectLoader>, ObjectLoader>;
template class NiTStringPointerMap<ObjectLoader>;
