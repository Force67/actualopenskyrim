#pragma once

// Singly linked list whose head node is the list object itself; an empty list
// is a head with a null item and no next node.
template <class T>
class BSSimpleList
{
public:
	using AllocateFn = BSSimpleList* (*)(T, void*);
	using DeallocateFn = void (*)(BSSimpleList*, void*);

	BSSimpleList() : m_item(), m_pkNext(nullptr) {}
	explicit BSSimpleList(const T& arItem) : m_item(arItem), m_pkNext(nullptr) {}
	~BSSimpleList() { RemoveAll(); }

	bool IsEmpty() const { return !m_pkNext && !m_item; }
	void AddHead(const T& arItem, AllocateFn apAllocateFn = nullptr, void* apArg = nullptr);
	void RemoveHead(DeallocateFn apDeallocateFn = nullptr, void* apArg = nullptr);
	void RemoveAll(DeallocateFn apDeallocateFn = nullptr, void* apArg = nullptr);

	T m_item;
	BSSimpleList* m_pkNext;
};

#include "BSCore/BSSimpleList.inl"
