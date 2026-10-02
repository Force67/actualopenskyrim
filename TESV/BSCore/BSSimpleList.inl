#pragma once

template <class T>
void BSSimpleList<T>::AddHead(const T& arItem, AllocateFn apAllocateFn, void* apArg)
{
	if (!arItem)
		return;
	if (m_item)
	{
		BSSimpleList* pNode = apAllocateFn ? apAllocateFn(m_item, apArg) : new BSSimpleList(m_item);
		pNode->m_pkNext = m_pkNext;
		m_pkNext = pNode;
	}
	m_item = arItem;
}

template <class T>
void BSSimpleList<T>::RemoveHead(DeallocateFn apDeallocateFn, void* apArg)
{
	BSSimpleList* pNext = m_pkNext;
	if (!pNext)
	{
		m_item = T();
		return;
	}
	m_item = pNext->m_item;
	m_pkNext = pNext->m_pkNext;
	pNext->m_pkNext = nullptr;
	if (apDeallocateFn)
		apDeallocateFn(pNext, apArg);
	else
		delete pNext;
}

template <class T>
void BSSimpleList<T>::RemoveAll(DeallocateFn apDeallocateFn, void* apArg)
{
	while (m_pkNext)
	{
		BSSimpleList* pRest = m_pkNext->m_pkNext;
		m_pkNext->m_pkNext = nullptr;
		if (apDeallocateFn)
			apDeallocateFn(m_pkNext, apArg);
		else
			delete m_pkNext;
		m_pkNext = pRest;
	}
	m_item = T();
}
