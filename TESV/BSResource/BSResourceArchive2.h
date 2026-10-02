#pragma once

#include "BSResource/BSResourceLooseFileStreams.h"

class ScrapHeap;
struct BSArchiveHeader
{
	BSArchiveHeader();
	unsigned int uiMagic;
	unsigned int uiVersion;
	unsigned int uiHeaderSize;
	unsigned int uiFlags;
	unsigned int uiDirectoryCount;
	unsigned int uiFileCount;
	unsigned int uiDirectoryNameLength;
	unsigned int uiFileNameLength;
	unsigned short usArchiveType;
};
struct BSFileEntry
{
	uint64_t uiHash;
	unsigned int uiSize;
	unsigned int uiOffset;
};
struct BSDirectoryEntry
{
	uint64_t uiHash;
	unsigned int uiFileCount;
	union { uint64_t uiOffset; BSFileEntry* pFileEntries; };
};
static_assert(sizeof(BSArchiveHeader) == 36);
static_assert(sizeof(BSFileEntry) == 16);
static_assert(sizeof(BSDirectoryEntry) == 24);
static_assert(offsetof(BSDirectoryEntry, pFileEntries) == 16);

namespace BSResource
{
	struct ArchiveSourceSBTraits
	{
		using StreamType = Stream;
		static ErrorCode MovePosition(Stream&, int64_t, uint64_t&);
		static ErrorCode Read(Stream&, void*, uint64_t, uint64_t&);
	};

	class ArchiveSource
	{
	public:
		ArchiveSource(BSTSmartPointer<Stream>&, Location*);
		~ArchiveSource();
		void* Delete(unsigned int);
		ErrorCode ReadAt(void*, uint64_t, uint64_t, uint64_t&);
		ErrorCode GetOrCreateAsyncStream(BSTSmartPointer<AsyncStream>&);
		ErrorCode LockAtRead(void*&, unsigned int&, uint64_t);
		ErrorCode Fill(void*&, unsigned int&);
		void Unlock();
		void SetTOCEnd(unsigned int);
		unsigned int IncRef() { return static_cast<unsigned int>(InterlockedIncrement(reinterpret_cast<volatile LONG*>(&uiRefCount))); }
		unsigned int DecRef() { return static_cast<unsigned int>(InterlockedDecrement(reinterpret_cast<volatile LONG*>(&uiRefCount))); }
		volatile unsigned int uiRefCount;
		volatile unsigned int uiLock;
		BSTSmartPointer<Stream> spStream;
		BSTSmartPointer<AsyncStream> spAsyncStream;
		Location* pLocation;
		void* pBuffer;
		StreamBuffer<ArchiveSourceSBTraits> Buffer;
		unsigned int uiTOCEnd;
		bool bEmbeddedFileNames;
	};

	class ArchiveStream : public Stream
	{
	public:
		ArchiveStream(const BSTSmartPointer<ArchiveSource>&, unsigned int, unsigned int, const BSFixedString&, bool);
		ArchiveStream(const ArchiveStream&);
		~ArchiveStream() override;
		ErrorCode DoOpen() override;
		uint64_t DoGetKey() const override;
		void DoClose() override;
		void DoClone(BSTSmartPointer<Stream>&) const override;
		ErrorCode DoRead(void*, uint64_t, uint64_t&) const override;
		ErrorCode DoWrite(const void*, uint64_t, uint64_t&) const override;
		ErrorCode DoSeek(int64_t, SeekMode, uint64_t&) const override;
		bool DoGetName(BSFixedString&) const override;
		ErrorCode DoCreateAsync(BSTSmartPointer<AsyncStream>&) const override;
		virtual unsigned int DoGetOffset() const;
		void SetSource(const BSTSmartPointer<ArchiveSource>&);
		void GetCurrentSource(BSTSmartPointer<ArchiveSource>&) const;
		const BSTSmartPointer<Stream>& GetSourceStream() const;
		Location* GetSourceLocation() const;
		ErrorCode ReadEmbeddedName(const BSTSmartPointer<ArchiveSource>&);
		void ClearEmbeddedName();
		mutable BSTSmartPointer<ArchiveSource> spSource;
		unsigned int uiOffset;
		mutable unsigned int uiPosition;
		BSFixedString FileName;
	};

	class ArchiveInfo
	{
	public:
		explicit ArchiveInfo(ArchiveSource&);
		~ArchiveInfo();
		const BSDirectoryEntry& QDirectoryEntry(unsigned int) const;
		unsigned int QTotalFiles() const { return uiTotalFiles; }
		BSArchiveHeader Header;
		BSDirectoryEntry* pDirectoryEntries;
		ScrapHeap* pHeap;
		void* pAllocation;
		unsigned int uiTotalFiles;
		char* pDirStringNames;
		char* pFileStringNames;
	};
	static_assert(sizeof(ArchiveInfo) == 88);
	static_assert(offsetof(ArchiveInfo, pDirectoryEntries) == 40);
	static_assert(offsetof(ArchiveInfo, pHeap) == 48);
	static_assert(offsetof(ArchiveInfo, pAllocation) == 56);
	static_assert(offsetof(ArchiveInfo, uiTotalFiles) == 64);
	static_assert(offsetof(ArchiveInfo, pDirStringNames) == 72);

	class CompressedArchiveStream : public ArchiveStream
	{
	public:
		struct Context
		{
			void* pDecompressionContext;
			unsigned int uiCompressedEnd;
			unsigned int uiConsumed;
			unsigned int uiProduced;
			unsigned int uiBufferSize;
			void* pBuffer;
		};
		CompressedArchiveStream(const BSTSmartPointer<ArchiveSource>&, unsigned int, unsigned int, bool, const BSFixedString&, bool);
		CompressedArchiveStream(const CompressedArchiveStream&);
		~CompressedArchiveStream() override;
		ErrorCode DoOpen() override;
		void DoClose() override;
		void DoClone(BSTSmartPointer<Stream>&) const override;
		ErrorCode DoRead(void*, uint64_t, uint64_t&) const override;
		ErrorCode DoSeek(int64_t, SeekMode, uint64_t&) const override;
		ErrorCode DoCreateAsync(BSTSmartPointer<AsyncStream>&) const override;
		unsigned int DoGetOffset() const override;
		ErrorCode ReadFromSourceAt(void*, uint64_t, uint64_t, uint64_t&) const;
		ErrorCode StandardCreateContext(unsigned int, uint64_t);
		ErrorCode StandardRead(void*, uint64_t, uint64_t&) const;
		void StandardDestroyContext();
		ErrorCode PlatformCreateContext(unsigned int, uint64_t);
		ErrorCode PlatformRead(void*, uint64_t, uint64_t&) const;
		void PlatformDestroyContext();
		mutable Context* pContext;
		mutable unsigned int uiRead;
	};
	static_assert(sizeof(CompressedArchiveStream) == 64);
	static_assert(offsetof(CompressedArchiveStream, pContext) == 48);
	static_assert(offsetof(CompressedArchiveStream, uiRead) == 56);
	static_assert(sizeof(CompressedArchiveStream::Context) == 32);
	static_assert(offsetof(CompressedArchiveStream::Context, pBuffer) == 24);

	static_assert(sizeof(StreamBuffer<ArchiveSourceSBTraits>) == 56);
	static_assert(offsetof(StreamBuffer<ArchiveSourceSBTraits>, rStream) == 0);
	static_assert(offsetof(StreamBuffer<ArchiveSourceSBTraits>, uiFlags) == 48);
	static_assert(sizeof(ArchiveSource) == 104);
	static_assert(offsetof(ArchiveSource, Buffer) == 40);
	static_assert(offsetof(ArchiveSource, bEmbeddedFileNames) == 100);
	static_assert(sizeof(ArchiveStream) == 48);
	static_assert(offsetof(ArchiveStream, spSource) == 24);
	static_assert(offsetof(ArchiveStream, uiPosition) == 36);
	static_assert(offsetof(ArchiveStream, FileName) == 40);
}
