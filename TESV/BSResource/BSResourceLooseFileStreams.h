#pragma once

#include "BSResource/BSResourcestream.h"
#include "BSSystem/BSSystemFile.h"

namespace BSResource
{
	extern bool bAllowAsyncIO;

	struct LooseFileSBTraits
	{
		using StreamType = BSSystemFile;
		struct AsyncFunctor : BSSystemFileAsyncFunctor
		{
			void Process(BSSystemFile::ErrorCode aeError, uint64_t auiTransferred) override;
			BSSystemFile::ErrorCode eError;
			uint64_t uiTransferred;
		};
		ErrorCode MovePosition(BSSystemFile&, int64_t, uint64_t&);
		ErrorCode Read(BSSystemFile&, void*, uint64_t, uint64_t&);
		uint64_t PathID;
	};

	template <class Traits>
	struct StreamBuffer : Traits
	{
		ErrorCode LockAtRead(void*&, unsigned int&, uint64_t);
		ErrorCode LockAtWrite(void*&, unsigned int&, uint64_t);
		ErrorCode ReadAt(void*, uint64_t, uint64_t, uint64_t&);
		ErrorCode WriteAt(const void*, uint64_t, uint64_t, uint64_t&);
		typename Traits::StreamType* rStream;
		uint64_t uiStartPosInStream;
		uint64_t uiEndPosInStream;
		uint64_t uiPosInStream;
		unsigned int uiBufferSize;
		void* pBuffer;
		unsigned int uiFlags;
	};

	class LooseFileStreamBase
	{
	public:
		LooseFileStreamBase(const BSFixedString& arPrefix, const BSFixedString& arDirectory, const BSFixedString& arFile);
		virtual ~LooseFileStreamBase();
		BSSystemFile::ErrorCode OpenFile(bool abWritable, bool abAsync);
		ErrorCode GetFileInfo(Info& arInfo);
		BSFixedString Prefix;
		BSFixedString DirName;
		BSFixedString FileName;
		mutable BSSystemFile File;
	};

	struct LooseFileAsyncBase
	{
		struct FunctorType : BSSystemFileAsyncFunctor
		{
			void Process(BSSystemFile::ErrorCode, uint64_t auiTransferred) override;
			uint64_t uiTransferred;
		};
		LooseFileAsyncBase();
		struct Initialized {};
		explicit LooseFileAsyncBase(Initialized) {}
		~LooseFileAsyncBase();
		static void Initialize(LooseFileAsyncBase* apBase);
		ErrorCode StartPacketAlignedBufferedRead(AsyncStream::PacketAlignedBuffer*, uint64_t, uint64_t, BSSystemFile&, unsigned int) const;
		ErrorCode Wait(uint64_t&, bool);
		union { mutable FunctorType Functor; };
		mutable AsyncStream::PacketAlignedBuffer* pCurrentBuffer;
	};

	class LooseFileStream : public LooseFileStreamBase, public Stream
	{
	public:
		LooseFileStream(const BSFixedString&, const BSFixedString&, const BSFixedString&, uint64_t, bool, Location*);
		LooseFileStream(const LooseFileStream& arStream);
		~LooseFileStream() override;
		ErrorCode DoOpen() override;
		void DoClose() override;
		ErrorCode DoGetInfo(Info&) override;
		void DoClone(BSTSmartPointer<Stream>&) const override;
		ErrorCode DoRead(void*, uint64_t, uint64_t&) const override;
		ErrorCode DoWrite(const void*, uint64_t, uint64_t&) const override;
		ErrorCode DoSeek(int64_t, SeekMode, uint64_t&) const override;
		ErrorCode DoSetEndOfStream() override;
		bool DoGetName(BSFixedString&) const override;
		ErrorCode DoCreateAsync(BSTSmartPointer<AsyncStream>&) const override;
		static void DestroyBuffer(LooseFileStream& arStream);
		Location* pLocation;
		mutable uint64_t uiFilePos;
		StreamBuffer<LooseFileSBTraits>* pBuffer;
	};

	class LooseFileAsyncStream : public LooseFileStreamBase, public AsyncStream, public LooseFileAsyncBase
	{
	public:
		LooseFileAsyncStream(const BSFixedString&, const BSFixedString&, const BSFixedString&, uint64_t, bool, Location*);
		~LooseFileAsyncStream() override;
		ErrorCode DoOpen() override;
		void DoClose() override;
		ErrorCode DoGetInfo(Info&) override;
		void DoClone(BSTSmartPointer<AsyncStream>&) const override;
		ErrorCode DoStartRead(void*, uint64_t, uint64_t) const override;
		ErrorCode DoStartPacketAlignedBufferedRead(PacketAlignedBuffer*, uint64_t, uint64_t) const override;
		ErrorCode DoStartWrite(const void*, uint64_t, uint64_t) const override;
		ErrorCode DoTruncate(uint64_t) const override;
		ErrorCode DoWait(uint64_t&, bool) override;
		volatile unsigned int uiOpenCount;
	};

	class LooseFileAsyncChild : public AsyncStream, public LooseFileAsyncBase
	{
	public:
		explicit LooseFileAsyncChild(LooseFileAsyncStream* apSource);
		~LooseFileAsyncChild() override;
		LooseFileAsyncChild(const LooseFileAsyncChild& arChild);
		ErrorCode DoOpen() override;
		void DoClose() override;
		ErrorCode DoGetInfo(Info&) override;
		void DoClone(BSTSmartPointer<AsyncStream>&) const override;
		ErrorCode DoStartRead(void*, uint64_t, uint64_t) const override;
		ErrorCode DoStartPacketAlignedBufferedRead(PacketAlignedBuffer*, uint64_t, uint64_t) const override;
		ErrorCode DoStartWrite(const void*, uint64_t, uint64_t) const override;
		ErrorCode DoTruncate(uint64_t) const override;
		ErrorCode DoWait(uint64_t&, bool) override;
		BSTSmartPointer<LooseFileAsyncStream> spSource;
	};

	static_assert(sizeof(LooseFileStreamBase) == 48);
	static_assert(offsetof(LooseFileStreamBase, Prefix) == 8);
	static_assert(offsetof(LooseFileStreamBase, File) == 32);
	static_assert(sizeof(StreamBuffer<LooseFileSBTraits>) == 64);
	static_assert(offsetof(StreamBuffer<LooseFileSBTraits>, rStream) == 8);
	static_assert(offsetof(StreamBuffer<LooseFileSBTraits>, uiBufferSize) == 40);
	static_assert(offsetof(StreamBuffer<LooseFileSBTraits>, pBuffer) == 48);
	static_assert(offsetof(StreamBuffer<LooseFileSBTraits>, uiFlags) == 56);
	static_assert(sizeof(LooseFileStream) == 96);
	static_assert(offsetof(LooseFileStream, pLocation) == 72);
	static_assert(offsetof(LooseFileStream, uiFilePos) == 80);
	static_assert(offsetof(LooseFileStream, pBuffer) == 88);
	static_assert(sizeof(LooseFileAsyncBase::FunctorType) == 56);
	static_assert(sizeof(LooseFileAsyncBase) == 64);
	static_assert(offsetof(LooseFileAsyncBase, pCurrentBuffer) == 56);
	static_assert(sizeof(LooseFileAsyncStream) == 152);
	static_assert(offsetof(LooseFileAsyncStream, uiOpenCount) == 144);
	static_assert(sizeof(LooseFileAsyncChild) == 104);
	static_assert(offsetof(LooseFileAsyncChild, spSource) == 96);
}
