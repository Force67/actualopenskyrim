#include "BSResource/BSResourceArchive2.h"

namespace BSResource
{
	ErrorCode CompressedArchiveStream::PlatformCreateContext(unsigned int, uint64_t) { return EC_UNSUPPORTED; }
	ErrorCode CompressedArchiveStream::PlatformRead(void*, uint64_t, uint64_t&) const { return EC_UNSUPPORTED; }
	void CompressedArchiveStream::PlatformDestroyContext() {}
}
