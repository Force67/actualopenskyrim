#include "BSResource/BSResourceReadBuffer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/MemoryContextTracker.h"

namespace BSResource::ReadBuffer
{
	BufferDesc* pBufferDesc = nullptr;
	BufferDesc BufferDescS;

	void InitSDM()
	{
		if (pBufferDesc)
			return;
		pBufferDesc = &BufferDescS;
		BufferDescS.pBuffer = nullptr;
		BufferDescS.pCurrent = nullptr;
		BufferDescS.pHead = nullptr;
		BufferDescS.Lock.OwningThread = 0;
		BufferDescS.Lock.uiLockCount = 0;
		{
			AutoMemContext Context(static_cast<MEM_CONTEXT>(8));
			void* pBuffer = MemoryManager::Instance().Allocate(0x4000000, 0x80, true);
			BufferDescS.pBuffer = pBuffer;
			if (pBuffer)
			{
				BufferDescS.pCurrent = pBuffer;
				BufferDescS.pEnd = static_cast<char*>(pBuffer) + 0x4000000;
			}
		}
		pBufferDesc = &BufferDescS;
	}

	void KillSDM()
	{
		if (pBufferDesc)
		{
			void* pBuffer = pBufferDesc->pBuffer;
			if (pBuffer)
				MemoryManager::Instance().Deallocate(pBuffer, true);
			pBufferDesc = nullptr;
		}
	}

	static bool Acquire(BufferDesc& arDesc)
	{
		_mm_lfence();
		if (arDesc.Lock.OwningThread != GetCurrentThreadId())
		{
			DWORD uiThread = GetCurrentThreadId();
			_mm_lfence();
			if (arDesc.Lock.OwningThread == uiThread)
				InterlockedIncrement(reinterpret_cast<volatile LONG*>(&arDesc.Lock.uiLockCount));
			else
			{
				if (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&arDesc.Lock.uiLockCount), 1, 0))
					return false;
				arDesc.Lock.OwningThread = uiThread;
				_mm_sfence();
			}
		}
		return true;
	}

	static void* AllocateIn(BufferDesc& arDesc, unsigned int auiSize, unsigned int auiAlignment)
	{
		if (!arDesc.pBuffer)
			return nullptr;
		uintptr_t uiHeader = reinterpret_cast<uintptr_t>(arDesc.pCurrent);
		uintptr_t uiBuffer = uiHeader + 16;
		if (auiAlignment && (static_cast<unsigned int>(uiBuffer) & (auiAlignment - 1)))
		{
			uiBuffer += auiAlignment - (static_cast<unsigned int>(uiBuffer) & (auiAlignment - 1));
			uiHeader = uiBuffer - 16;
		}
		uintptr_t uiEnd = uiBuffer + auiSize;
		if (uiEnd >= reinterpret_cast<uintptr_t>(arDesc.pEnd))
			return nullptr;
		uintptr_t* pHeader = reinterpret_cast<uintptr_t*>(uiHeader);
		pHeader[1] = auiSize;
		pHeader[0] = reinterpret_cast<uintptr_t>(arDesc.pHead);
		arDesc.pHead = pHeader;
		arDesc.pCurrent = reinterpret_cast<void*>(uiEnd);
		return reinterpret_cast<void*>(uiBuffer);
	}

	void* Allocate(unsigned int auiSize)
	{
		BufferDesc* pDesc = pBufferDesc;
		return Acquire(*pDesc) ? AllocateIn(*pDesc, auiSize, 16) : nullptr;
	}

	void* AllocateAlign(unsigned int auiSize, unsigned int auiAlignment)
	{
		BufferDesc* pDesc = pBufferDesc;
		if (!Acquire(*pDesc))
			return nullptr;
		return AllocateIn(*pDesc, auiSize, auiAlignment < 16 ? auiAlignment : 16);
	}

	void Deallocate(void* apBuffer)
	{
		BufferDesc* pDesc = pBufferDesc;
		if (pDesc->pBuffer)
		{
			*(reinterpret_cast<uintptr_t*>(apBuffer) - 1) |= 0x80000000;
			uintptr_t* pHead = static_cast<uintptr_t*>(pDesc->pHead);
			while (pHead && (pHead[1] & 0x80000000))
			{
				pDesc->pCurrent = pHead;
				pHead = reinterpret_cast<uintptr_t*>(pHead[0]);
			}
			if (!pHead)
				pDesc->pCurrent = pDesc->pBuffer;
			pDesc->pHead = pHead;
		}
		if (pDesc->pCurrent == pDesc->pBuffer)
			pDesc->Lock.Unlock();
	}

	bool Contains(void* apBuffer)
	{
		uintptr_t uiBuffer = reinterpret_cast<uintptr_t>(apBuffer);
		return uiBuffer >= reinterpret_cast<uintptr_t>(pBufferDesc->pBuffer) &&
			uiBuffer < reinterpret_cast<uintptr_t>(pBufferDesc->pEnd);
	}

	unsigned int QBufferSize() { return 0x4000000; }
}
