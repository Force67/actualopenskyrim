#pragma once

#include "BSCore/BSSpinLock.h"

#include <cstddef>
#include <cstdint>

namespace BSResource::ReadBuffer
{
	struct BufferDesc
	{
		uintptr_t uiSingleton;
		void* pBuffer;
		void* pCurrent;
		void* pEnd;
		void* pHead;
		BSSpinLock Lock;
	};

	extern BufferDesc* pBufferDesc;
	extern BufferDesc BufferDescS;
	void InitSDM();
	void KillSDM();
	void* Allocate(unsigned int auiSize);
	void* AllocateAlign(unsigned int auiSize, unsigned int auiAlignment);
	void Deallocate(void* apBuffer);
	bool Contains(void* apBuffer);
	unsigned int QBufferSize();

	static_assert(sizeof(BufferDesc) == 48);
	static_assert(offsetof(BufferDesc, pBuffer) == 8);
	static_assert(offsetof(BufferDesc, pCurrent) == 16);
	static_assert(offsetof(BufferDesc, pEnd) == 24);
	static_assert(offsetof(BufferDesc, pHead) == 32);
	static_assert(offsetof(BufferDesc, Lock) == 40);
}
