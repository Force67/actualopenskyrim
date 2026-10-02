#include "NiSmallObjectAllocator.h"
#include "NiMemoryDefines.h"

#include <cstdint>
#include <cstring>

NiFixedAllocator::NiFixedAllocator() :
	m_stBlockSize(0), m_ucNumBlocks(0), m_pkChunks(nullptr), m_stNumChunks(0),
	m_stMaxNumChunks(0), m_pkAllocChunk(nullptr), m_pkDeallocChunk(nullptr), m_pkEmptyChunk(nullptr)
{
	static_assert(offsetof(NiFixedAllocator, m_ucNumBlocks) == 8);
	static_assert(offsetof(NiFixedAllocator, m_pkChunks) == 16);
	static_assert(offsetof(NiFixedAllocator, m_stNumChunks) == 24);
	static_assert(offsetof(NiFixedAllocator, m_stMaxNumChunks) == 32);
	static_assert(offsetof(NiFixedAllocator, m_pkAllocChunk) == 40);
	static_assert(offsetof(NiFixedAllocator, m_pkDeallocChunk) == 48);
	static_assert(offsetof(NiFixedAllocator, m_pkEmptyChunk) == 56);
	static_assert(offsetof(NiFixedAllocator, m_kCriticalSection) == 128);
	InitializeCriticalSection(&m_kCriticalSection);
}

NiFixedAllocator::~NiFixedAllocator()
{
	for (size_t i = 0; i < m_stNumChunks; ++i)
		_NiExternalAlignedFree(m_pkChunks[i].m_pucData);
	_NiExternalFree(m_pkChunks);
	DeleteCriticalSection(&m_kCriticalSection);
}

NiSmallObjectAllocator::NiSmallObjectAllocator(size_t)
{
	for (size_t i = 0; i < 256; ++i)
	{
		NiFixedAllocator& kPool = m_kPool[i];
		kPool.m_pkChunks = nullptr;
		kPool.m_stNumChunks = 0;
		kPool.m_stMaxNumChunks = 0;
		kPool.m_pkAllocChunk = nullptr;
		kPool.m_stBlockSize = i + 1;
		size_t stNumBlocks = 25600 / (i + 1);
		kPool.m_ucNumBlocks = static_cast<unsigned char>(stNumBlocks > 255 ? 255 : stNumBlocks);
	}
}

void* NiFixedAllocator::Allocate()
{
	EnterCriticalSection(&m_kCriticalSection);
	Chunk* pkChunk = m_pkAllocChunk;
	if (pkChunk && !pkChunk->m_ucBlocksAvailable)
		pkChunk = m_pkAllocChunk = nullptr;
	if (m_pkEmptyChunk)
	{
		pkChunk = m_pkAllocChunk = m_pkEmptyChunk;
		m_pkEmptyChunk = nullptr;
	}
	if (!pkChunk)
	{
		size_t stNumChunks = m_stNumChunks;
		for (size_t i = 0; i < stNumChunks; ++i)
		{
			if (m_pkChunks[i].m_ucBlocksAvailable)
			{
				pkChunk = m_pkAllocChunk = &m_pkChunks[i];
				break;
			}
		}
		if (!pkChunk)
		{
			if (stNumChunks + 1 > m_stMaxNumChunks)
			{
				m_pkChunks = static_cast<Chunk*>(_NiExternalRealloc(m_pkChunks, (stNumChunks + 1) * sizeof(Chunk)));
				m_stMaxNumChunks = stNumChunks + 1;
			}
			unsigned char ucNumBlocks = m_ucNumBlocks;
			size_t stBlockSize = m_stBlockSize;
			Chunk kChunk;
			kChunk.m_pucData = static_cast<unsigned char*>(_NiExternalAlignedMalloc(stBlockSize * ucNumBlocks, 4));
			kChunk.m_ucFirstAvailableBlock = 0;
			kChunk.m_ucBlocksAvailable = ucNumBlocks;
			unsigned char* pucBlock = kChunk.m_pucData;
			for (unsigned int i = 1; i <= ucNumBlocks; ++i, pucBlock += stBlockSize)
				*pucBlock = static_cast<unsigned char>(i);
			stNumChunks = m_stNumChunks;
			if (stNumChunks + 1 > m_stMaxNumChunks)
			{
				m_pkChunks = static_cast<Chunk*>(_NiExternalRealloc(m_pkChunks, (stNumChunks + 1) * sizeof(Chunk)));
				m_stMaxNumChunks = stNumChunks + 1;
			}
			memcpy(&m_pkChunks[stNumChunks], &kChunk, sizeof(Chunk));
			++m_stNumChunks;
			m_pkDeallocChunk = m_pkChunks;
			pkChunk = m_pkAllocChunk = &m_pkChunks[m_stNumChunks - 1];
		}
	}
	void* pvResult = nullptr;
	if (pkChunk->m_ucBlocksAvailable)
	{
		unsigned char* pucBlock = pkChunk->m_pucData + m_stBlockSize * pkChunk->m_ucFirstAvailableBlock;
		pkChunk->m_ucFirstAvailableBlock = *pucBlock;
		--pkChunk->m_ucBlocksAvailable;
		pvResult = pucBlock;
	}
	LeaveCriticalSection(&m_kCriticalSection);
	return pvResult;
}

void NiFixedAllocator::Deallocate(void* p)
{
	EnterCriticalSection(&m_kCriticalSection);
	m_pkDeallocChunk = VicinityFind(p);
	DoDeallocate(p);
	LeaveCriticalSection(&m_kCriticalSection);
}

void* NiSmallObjectAllocator::Allocate(size_t stNumBytes)
{
	return m_kPool[stNumBytes - 1].Allocate();
}

void NiSmallObjectAllocator::Deallocate(void* p, size_t stSize)
{
	m_kPool[stSize - 1].Deallocate(p);
}

NiFixedAllocator::Chunk* NiFixedAllocator::VicinityFind(void* p)
{
	Chunk* pkLower = m_pkDeallocChunk;
	Chunk* pkEnd = m_pkChunks + m_stNumChunks;
	Chunk* pkUpper = pkLower + 1 == pkEnd ? nullptr : pkLower + 1;
	size_t stBlockSize = m_stBlockSize;
	size_t stChunkLength = stBlockSize * m_ucNumBlocks;
	uintptr_t uiAddress = reinterpret_cast<uintptr_t>(p);
	Chunk* pkFound = nullptr;
	for (;;)
	{
		if (pkLower)
		{
			uintptr_t uiStart = reinterpret_cast<uintptr_t>(pkLower->m_pucData);
			if (uiStart <= uiAddress && uiAddress < uiStart + stChunkLength)
			{
				pkFound = pkLower;
				break;
			}
			pkLower = pkLower == m_pkChunks ? nullptr : pkLower - 1;
			if (!pkLower && !pkUpper)
				break;
		}
		if (pkUpper)
		{
			uintptr_t uiStart = reinterpret_cast<uintptr_t>(pkUpper->m_pucData);
			if (uiStart <= uiAddress && uiAddress < uiStart + stChunkLength)
			{
				pkFound = pkUpper;
				break;
			}
			if (++pkUpper == pkEnd)
				pkUpper = nullptr;
			if (!pkLower && !pkUpper)
				break;
		}
	}
	return pkFound;
}

void NiFixedAllocator::Init(size_t stBlockSize)
{
	m_stBlockSize = stBlockSize;
	m_stNumChunks = 0;
	m_stMaxNumChunks = 0;
	m_pkChunks = nullptr;
	m_pkAllocChunk = nullptr;
	size_t stNumBlocks = 25600 / stBlockSize;
	if (stNumBlocks > 255)
		m_ucNumBlocks = 255;
	else
		m_ucNumBlocks = static_cast<unsigned char>(stNumBlocks ? stNumBlocks : stBlockSize * 8);
}

NiFixedAllocator* NiSmallObjectAllocator::GetFixedAllocatorForSize(size_t stNumBytes)
{
	return &m_kPool[stNumBytes - 1];
}

void NiFixedAllocator::DoDeallocate(void* p)
{
	Chunk* pkChunk = m_pkDeallocChunk;
	size_t stBlockSize = m_stBlockSize;
	uintptr_t uiAddress = reinterpret_cast<uintptr_t>(p);
	unsigned char* pucBlock = static_cast<unsigned char*>(p);
	*pucBlock = pkChunk->m_ucFirstAvailableBlock;
	++pkChunk->m_ucBlocksAvailable;
	pkChunk->m_ucFirstAvailableBlock = static_cast<unsigned char>((uiAddress - reinterpret_cast<uintptr_t>(pkChunk->m_pucData)) / stBlockSize);
	if (m_pkDeallocChunk->m_ucBlocksAvailable == m_ucNumBlocks)
	{
		if (m_pkEmptyChunk)
		{
			Chunk* pkLast = &m_pkChunks[m_stNumChunks - 1];
			if (pkLast == m_pkDeallocChunk)
				m_pkDeallocChunk = m_pkEmptyChunk;
			else if (pkLast != m_pkEmptyChunk)
			{
				unsigned char aucChunk[sizeof(Chunk)];
				memcpy(aucChunk, m_pkEmptyChunk, sizeof(Chunk));
				memcpy(m_pkEmptyChunk, pkLast, sizeof(Chunk));
				memcpy(pkLast, aucChunk, sizeof(Chunk));
			}
			_NiExternalAlignedFree(pkLast->m_pucData);
			--m_stNumChunks;
			m_pkAllocChunk = m_pkDeallocChunk;
		}
		m_pkEmptyChunk = m_pkDeallocChunk;
	}
}

void NiFixedAllocator::Chunk::Init(size_t stBlockSize, unsigned char ucBlocks)
{
	m_pucData = static_cast<unsigned char*>(_NiExternalAlignedMalloc(stBlockSize * ucBlocks, 4));
	m_ucFirstAvailableBlock = 0;
	m_ucBlocksAvailable = ucBlocks;
	unsigned char* pucBlock = m_pucData;
	for (unsigned int i = 1; i <= ucBlocks; ++i, pucBlock += stBlockSize)
		*pucBlock = static_cast<unsigned char>(i);
}

void* NiFixedAllocator::Chunk::Allocate(size_t stBlockSize)
{
	if (!m_ucBlocksAvailable)
		return nullptr;
	unsigned char ucAvailable = m_ucBlocksAvailable;
	unsigned char* pucBlock = m_pucData + m_ucFirstAvailableBlock * stBlockSize;
	m_ucFirstAvailableBlock = *pucBlock;
	m_ucBlocksAvailable = static_cast<unsigned char>(ucAvailable - 1);
	return pucBlock;
}

void NiFixedAllocator::Chunk::Deallocate(void* p, size_t stBlockSize)
{
	unsigned char* pucBlock = static_cast<unsigned char*>(p);
	*pucBlock = m_ucFirstAvailableBlock;
	size_t stOffset = reinterpret_cast<uintptr_t>(p) - reinterpret_cast<uintptr_t>(m_pucData);
	++m_ucBlocksAvailable;
	m_ucFirstAvailableBlock = static_cast<unsigned char>(stOffset / stBlockSize);
}

void NiFixedAllocator::Chunk::Reset(size_t stBlockSize, unsigned char ucBlocks)
{
	unsigned char* pucBlock = m_pucData;
	m_ucFirstAvailableBlock = 0;
	m_ucBlocksAvailable = ucBlocks;
	for (unsigned int i = 1; i <= ucBlocks; ++i, pucBlock += stBlockSize)
		*pucBlock = static_cast<unsigned char>(i);
}

void NiFixedAllocator::Chunk::Release()
{
	_NiExternalAlignedFree(m_pucData);
}

bool NiFixedAllocator::Chunk::HasAvailable(unsigned char ucNumBlocks) const
{
	return m_ucBlocksAvailable == ucNumBlocks;
}

bool NiFixedAllocator::Chunk::HasBlock(unsigned char* p, size_t stChunkLength) const
{
	uintptr_t uiStart = reinterpret_cast<uintptr_t>(m_pucData);
	uintptr_t uiAddress = reinterpret_cast<uintptr_t>(p);
	return uiStart <= uiAddress && uiAddress < uiStart + stChunkLength;
}

bool NiFixedAllocator::Chunk::IsFilled() const
{
	return m_ucBlocksAvailable == 0;
}

void NiFixedAllocator::Push_Back(Chunk& kChunk)
{
	size_t stNumChunks = m_stNumChunks;
	if (stNumChunks + 1 > m_stMaxNumChunks)
	{
		m_pkChunks = static_cast<Chunk*>(_NiExternalRealloc(m_pkChunks, (stNumChunks + 1) * sizeof(Chunk)));
		m_stMaxNumChunks = stNumChunks + 1;
	}
	memcpy(&m_pkChunks[stNumChunks], &kChunk, sizeof(Chunk));
	++m_stNumChunks;
}

void NiFixedAllocator::Pop_Back()
{
	--m_stNumChunks;
}

void NiFixedAllocator::Reserve(size_t stNewSize)
{
	if (stNewSize > m_stMaxNumChunks)
	{
		m_pkChunks = static_cast<Chunk*>(_NiExternalRealloc(m_pkChunks, stNewSize * sizeof(Chunk)));
		m_stMaxNumChunks = stNewSize;
	}
}
