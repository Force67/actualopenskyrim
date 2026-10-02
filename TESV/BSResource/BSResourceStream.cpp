#include "BSResource/BSResourcestream.h"

#include <new>
#include <utility>

namespace BSResource
{
	StreamBase::~StreamBase() = default;
	Stream::~Stream() = default;
	AsyncStream::~AsyncStream() = default;

	AsyncStream::AsyncStream(uint64_t auiTotalSize, bool abWritable, Location* apLocation) :
		StreamBase(auiTotalSize, abWritable)
	{
		uiMinPacketSize = apLocation->DoGetMinimumAsyncPacketSize();
	}

	void* StreamBase::Delete(unsigned int auiFlags)
	{
		this->StreamBase::~StreamBase();
		if (auiFlags & 1)
			::operator delete(this, sizeof(StreamBase));
		return this;
	}

	void* Stream::Delete(unsigned int auiFlags)
	{
		this->Stream::~Stream();
		if (auiFlags & 1)
			::operator delete(this, sizeof(Stream));
		return this;
	}

	void* AsyncStream::Delete(unsigned int auiFlags)
	{
		this->AsyncStream::~AsyncStream();
		if (auiFlags & 1)
			::operator delete(this, sizeof(AsyncStream));
		return this;
	}

	uint64_t StreamBase::DoGetKey() const { return 0; }
	ErrorCode StreamBase::DoGetInfo(Info&) { return EC_UNSUPPORTED; }
	ErrorCode Stream::DoSetEndOfStream() { return EC_UNSUPPORTED; }
	ErrorCode Stream::DoCreateAsync(BSTSmartPointer<AsyncStream>&) const { return EC_UNSUPPORTED; }
	bool Stream::DoGetName(BSFixedString& arName) const
	{
		BSFixedString Name(static_cast<const char*>(nullptr));
		arName = std::move(Name);
		return false;
	}
}
