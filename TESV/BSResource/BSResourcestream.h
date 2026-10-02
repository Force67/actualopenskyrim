#pragma once

#include "BSResource/BSResourceStreamBase.h"
#include "BSSystem/BSFixedString.h"

namespace BSResource
{
	enum SeekMode : unsigned int { SM_SET, SM_CUR, SM_END };

	class Stream : public StreamBase
	{
	public:
		Stream(uint64_t auiTotalSize, bool abWritable) : StreamBase(auiTotalSize, abWritable) {}
		~Stream() override;
		virtual void DoClone(BSTSmartPointer<Stream>& arStream) const = 0;
		virtual ErrorCode DoRead(void* apBuffer, uint64_t auiBytes, uint64_t& arRead) const = 0;
		virtual ErrorCode DoWrite(const void* apBuffer, uint64_t auiBytes, uint64_t& arWritten) const = 0;
		virtual ErrorCode DoSeek(int64_t aiOffset, SeekMode aeMode, uint64_t& arPosition) const = 0;
		virtual ErrorCode DoSetEndOfStream();
		virtual bool DoGetName(BSFixedString& arName) const;
		virtual ErrorCode DoCreateAsync(BSTSmartPointer<AsyncStream>& arStream) const;
		void* Delete(unsigned int auiFlags);
	};

	class AsyncStream : public StreamBase
	{
	public:
		struct PacketAlignedBuffer
		{
			uint64_t uiResultOffset;
			unsigned int uiBufferSize;
			void* pPacketBuffer;
			unsigned int uiDataRequestSize;
			unsigned int uiDataSize;
			void* pDataStart;
		};

		AsyncStream(uint64_t auiTotalSize, bool abWritable, unsigned int auiMinimumPacketSize) :
			StreamBase(auiTotalSize, abWritable), uiMinPacketSize(auiMinimumPacketSize) {}
		AsyncStream(uint64_t auiTotalSize, bool abWritable, Location* apLocation);
		~AsyncStream() override;
		virtual void DoClone(BSTSmartPointer<AsyncStream>& arStream) const = 0;
		virtual ErrorCode DoStartRead(void* apBuffer, uint64_t auiBytes, uint64_t auiOffset) const = 0;
		virtual ErrorCode DoStartPacketAlignedBufferedRead(PacketAlignedBuffer* apBuffer, uint64_t auiBytes, uint64_t auiOffset) const = 0;
		virtual ErrorCode DoStartWrite(const void* apBuffer, uint64_t auiBytes, uint64_t auiOffset) const = 0;
		virtual ErrorCode DoTruncate(uint64_t auiSize) const = 0;
		virtual ErrorCode DoWait(uint64_t& arTransferred, bool abWait) = 0;
		void* Delete(unsigned int auiFlags);

		unsigned int uiMinPacketSize;
		char cAsyncPadding[4];
	};

	static_assert(sizeof(Stream) == 24);
	static_assert(sizeof(AsyncStream) == 32);
	static_assert(offsetof(AsyncStream, uiMinPacketSize) == 24);
	static_assert(sizeof(AsyncStream::PacketAlignedBuffer) == 40);
	static_assert(offsetof(AsyncStream::PacketAlignedBuffer, pPacketBuffer) == 16);
	static_assert(offsetof(AsyncStream::PacketAlignedBuffer, pDataStart) == 32);
}
