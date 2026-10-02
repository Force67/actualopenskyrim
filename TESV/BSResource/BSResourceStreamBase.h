#pragma once

#include "BSResource/BSResourceLocation.h"

namespace BSResource
{
	class StreamBase
	{
	public:
		StreamBase(uint64_t auiTotalSize, bool abWritable) : uiTotalSize(auiTotalSize), uiFlags(abWritable) {}
		virtual ~StreamBase();
		virtual ErrorCode DoOpen() = 0;
		virtual void DoClose() = 0;
		virtual uint64_t DoGetKey() const;
		virtual ErrorCode DoGetInfo(Info& arInfo);
		void* Delete(unsigned int auiFlags);

		unsigned int IncRef()
		{
			LONG iOld;
			do { iOld = uiFlags; }
			while (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&uiFlags), static_cast<LONG>(static_cast<unsigned int>(iOld) + 0x1000), iOld) != iOld);
			return (static_cast<unsigned int>(iOld) + 0x1000) & 0xfffff000;
		}
		unsigned int DecRef()
		{
			LONG iOld;
			do { iOld = uiFlags; }
			while (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&uiFlags), static_cast<LONG>(static_cast<unsigned int>(iOld) - 0x1000), iOld) != iOld);
			return (static_cast<unsigned int>(iOld) - 0x1000) & 0xfffff000;
		}
		uint64_t QTotalSize() const { return uiTotalSize; }
		bool QWritable() const { return (uiFlags & 1) != 0; }

		mutable uint64_t uiTotalSize;
		volatile unsigned int uiFlags;
		char cPadding[4];
	};

	static_assert(sizeof(StreamBase) == 24);
	static_assert(offsetof(StreamBase, uiTotalSize) == 8);
	static_assert(offsetof(StreamBase, uiFlags) == 16);
}
