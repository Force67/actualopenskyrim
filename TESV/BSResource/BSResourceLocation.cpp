#include "BSResource/BSResourceLocation.h"

#include <new>

namespace BSResource
{
	Location::~Location()
	{
		if (bMounted)
			Location::DoUnmount();
	}

	void* Location::Delete(unsigned int auiFlags)
	{
		Location::~Location();
		if (auiFlags & 1)
			::operator delete(this, sizeof(Location));
		return this;
	}

	ErrorCode Location::DoMount() { return EC_NONE; }
	void Location::DoUnmount() {}
	ErrorCode Location::DoCreateAsyncStream(const char*, BSTSmartPointer<AsyncStream>&, Location*&, bool) { return EC_UNSUPPORTED; }
	ErrorCode Location::DoGetInfo(const char*, Info&, Location*&) { return EC_UNSUPPORTED; }
	ErrorCode Location::DoGetInfo(const char*, Info&) { return EC_UNSUPPORTED; }
	ErrorCode Location::DoDelete(const char*) { return EC_UNSUPPORTED; }
	const char* Location::DoGetName() const { return nullptr; }
	unsigned int Location::DoQBufferHint() const { return 0x80000; }
	unsigned int Location::DoGetMinimumAsyncPacketSize() const { return 0x80000; }
}
