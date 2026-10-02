#pragma once

#include <cstddef>
#include <windows.h>

class NiFixedAllocator
{
public:
	NiFixedAllocator();
	~NiFixedAllocator();
	void Init(size_t stBlockSize);
	void* Allocate();
	void Deallocate(void* p);

private:
	friend class NiSmallObjectAllocator;
	struct Chunk
	{
		void Init(size_t stBlockSize, unsigned char ucBlocks);
		void* Allocate(size_t stBlockSize);
		void Deallocate(void* p, size_t stBlockSize);
		void Reset(size_t stBlockSize, unsigned char ucBlocks);
		void Release();
		bool HasAvailable(unsigned char ucNumBlocks) const;
		bool HasBlock(unsigned char* p, size_t stChunkLength) const;
		bool IsFilled() const;

		unsigned char* m_pucData;
		unsigned char m_ucFirstAvailableBlock;
		unsigned char m_ucBlocksAvailable;
	};
	static_assert(sizeof(Chunk) == 16);
	Chunk* VicinityFind(void* p);
	void Push_Back(Chunk& kChunk);
	void Pop_Back();
	void Reserve(size_t stNewSize);
	void DoDeallocate(void* p);

	size_t m_stBlockSize;
	unsigned char m_ucNumBlocks;
	Chunk* m_pkChunks;
	size_t m_stNumChunks;
	size_t m_stMaxNumChunks;
	Chunk* m_pkAllocChunk;
	Chunk* m_pkDeallocChunk;
	Chunk* m_pkEmptyChunk;
	alignas(128) CRITICAL_SECTION m_kCriticalSection;
};
static_assert(sizeof(NiFixedAllocator) == 256);

class NiSmallObjectAllocator
{
public:
	explicit NiSmallObjectAllocator(size_t stChunkSize = 25600);
	void* Allocate(size_t stNumBytes);
	void Deallocate(void* p, size_t stSize);
	NiFixedAllocator* GetFixedAllocatorForSize(size_t stNumBytes);

private:
	NiFixedAllocator m_kPool[256];
};
static_assert(sizeof(NiSmallObjectAllocator) == 65536);
