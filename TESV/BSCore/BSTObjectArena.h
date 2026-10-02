#pragma once

#include <cstddef>
#include <new>
#include "BSCore/ScrapHeap.h"

struct BSTObjectArenaHeapAlloc
{
	void* Allocate(size_t auiSize) const;
	void Deallocate(void* apBlock) const;
};

struct BSTObjectArenaScrapAllocBase
{
	BSTObjectArenaScrapAllocBase();
	explicit BSTObjectArenaScrapAllocBase(ScrapHeap* apScrapHeap);
	~BSTObjectArenaScrapAllocBase();
	ScrapHeap* pScrapHeap;
};

struct BSTObjectArenaScrapAlloc : BSTObjectArenaScrapAllocBase
{
	using BSTObjectArenaScrapAllocBase::BSTObjectArenaScrapAllocBase;
	void* Allocate(size_t auiSize) const;
	void Deallocate(void* apBlock) const;
};

template<unsigned int Alignment>
struct BSTObjectArenaScrapAlignAlloc : BSTObjectArenaScrapAllocBase
{
	using BSTObjectArenaScrapAllocBase::BSTObjectArenaScrapAllocBase;
	void* Allocate(unsigned int auiSize) const { return pScrapHeap->Allocate(auiSize, Alignment); }
	void Deallocate(void* apBlock) const { pScrapHeap->Deallocate(apBlock); }
};

static_assert(sizeof(BSTObjectArenaHeapAlloc) == 1);
static_assert(sizeof(BSTObjectArenaScrapAllocBase) == 8);
static_assert(sizeof(BSTObjectArenaScrapAlignAlloc<16>) == 8);

template<class T, class Allocator = BSTObjectArenaScrapAlloc, unsigned int Count = 32>
class BSTObjectArena : public Allocator
{
public:
	struct Page
	{
		alignas(T) char cBuffer[sizeof(T) * Count];
		Page* pNext;
	};
	struct Iterator
	{
		T* pCurrPos;
		Page* pCurrPage;
		T& operator*() const { return *pCurrPos; }
		T* operator->() const { return pCurrPos; }
		bool operator==(const Iterator& arRhs) const { return pCurrPos == arRhs.pCurrPos; }
		bool operator!=(const Iterator& arRhs) const { return pCurrPos != arRhs.pCurrPos; }
		Iterator& operator++()
		{
			++pCurrPos;
			if (reinterpret_cast<char*>(pCurrPos) >= PageEnd(pCurrPage))
			{
				pCurrPage = pCurrPage->pNext;
				if (pCurrPage)
					pCurrPos = reinterpret_cast<T*>(pCurrPage->cBuffer);
			}
			return *this;
		}
	};
	BSTObjectArena() : pHead(nullptr), ppTail(&pHead), pCurr(nullptr), pAvail(nullptr), pCurrAddPos(nullptr), pCurrHeadPos(nullptr), Size(0) {}
	explicit BSTObjectArena(ScrapHeap* apHeap) : Allocator(apHeap), pHead(nullptr), ppTail(&pHead), pCurr(nullptr), pAvail(nullptr), pCurrAddPos(nullptr), pCurrHeadPos(nullptr), Size(0) {}
	~BSTObjectArena() { Clear(true); }
	unsigned int QSize() const { return Size; }
	void Add(const T& arValue) { new (Reserve()) T(arValue); }
private:
	void* Reserve()
	{
		if (!pCurr || pCurrAddPos == PageEnd(pCurr))
		{
			Page* pPage = pAvail;
			if (pPage)
				pAvail = pPage->pNext;
			else
				pPage = static_cast<Page*>(this->Allocate(sizeof(Page)));
			pPage->pNext = nullptr;
			*ppTail = pPage;
			ppTail = &pPage->pNext;
			pCurr = pPage;
			pCurrAddPos = pPage->cBuffer;
			if (!pCurrHeadPos)
				pCurrHeadPos = pPage->cBuffer;
		}
		char* pResult = pCurrAddPos;
		pCurrAddPos += sizeof(T);
		++Size;
		return pResult;
	}
public:
	void RemoveHeadPage(bool abFreeMemory)
	{
		Page* pOld = pHead;
		Page* pNext = pOld->pNext;
		if (pOld == pCurr)
		{
			pCurrAddPos = nullptr;
			ppTail = &pHead;
			pCurr = nullptr;
			pCurrHeadPos = nullptr;
		}
		else
			pCurrHeadPos = pNext ? pNext->cBuffer : nullptr;
		if (abFreeMemory)
			this->Deallocate(pOld);
		else
		{
			pOld->pNext = pAvail;
			pAvail = pOld;
		}
		pHead = pNext;
	}
	void ClearFront(bool abFreeMemory)
	{
		reinterpret_cast<T*>(pCurrHeadPos)->~T();
		pCurrHeadPos += sizeof(T);
		if (pCurrHeadPos >= PageEnd(pHead))
			RemoveHeadPage(abFreeMemory);
		--Size;
	}
	void Clear(bool abFreeMemory)
	{
		while (Size)
			ClearFront(abFreeMemory);
		while (pHead)
			RemoveHeadPage(abFreeMemory);
		ppTail = &pHead;
		pCurr = nullptr;
		pCurrAddPos = nullptr;
		pCurrHeadPos = nullptr;
		if (abFreeMemory)
		{
			while (pAvail)
			{
				Page* pNext = pAvail->pNext;
				this->Deallocate(pAvail);
				pAvail = pNext;
			}
		}
	}
	Iterator Begin() const
	{
		Iterator Result{pHead ? reinterpret_cast<T*>(pCurrHeadPos) : nullptr, pHead};
		if (pCurrHeadPos == pCurrAddPos)
		{
			Result.pCurrPage = nullptr;
			if (pCurr && pCurrHeadPos >= PageEnd(pCurr) && pCurr->pNext)
				Result.pCurrPos = reinterpret_cast<T*>(pCurr->pNext->cBuffer);
		}
		return Result;
	}
	Iterator End() const
	{
		char* pPos = pCurrAddPos;
		if (pCurr && pPos >= PageEnd(pCurr) && pCurr->pNext)
			pPos = pCurr->pNext->cBuffer;
		return {reinterpret_cast<T*>(pPos), nullptr};
	}
	static char* PageEnd(const Page* apPage) { return const_cast<char*>(apPage->cBuffer) + sizeof(T) * Count; }
	Page* pHead;
	Page** ppTail;
	Page* pCurr;
	Page* pAvail;
	char* pCurrAddPos;
	char* pCurrHeadPos;
	unsigned int Size;
};
