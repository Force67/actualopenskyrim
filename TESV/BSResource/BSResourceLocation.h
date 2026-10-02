#pragma once

#include "BSResource/BSResourceConstants.h"
#include "BSSystem/BSTSmartPointer.h"

#include <cstddef>
#include <cstdint>
#include <windows.h>

namespace BSResource
{
	class Stream;
	class AsyncStream;
	class LocationTraverser;

	struct Info
	{
		FILETIME ModifyTime;
		FILETIME CreateTime;
		uint64_t uiFileSize;
	};

	class Location
	{
	public:
		Location() : bMounted(false) {}
		virtual ~Location();
		virtual ErrorCode DoMount();
		virtual void DoUnmount();
		virtual ErrorCode DoCreateStream(const char* apPath, BSTSmartPointer<Stream>& arStream, Location*& arLocation, bool abReadOnly) = 0;
		virtual ErrorCode DoCreateAsyncStream(const char* apPath, BSTSmartPointer<AsyncStream>& arStream, Location*& arLocation, bool abReadOnly);
		virtual ErrorCode DoTraversePrefix(const char* apPath, LocationTraverser& arTraverser) = 0;
		virtual ErrorCode DoGetInfo(const char* apPath, Info& arInfo);
		virtual ErrorCode DoGetInfo(const char* apPath, Info& arInfo, Location*& arLocation);
		virtual ErrorCode DoDelete(const char* apPath);
		virtual const char* DoGetName() const;
		virtual unsigned int DoQBufferHint() const;
		virtual unsigned int DoGetMinimumAsyncPacketSize() const;
		void* Delete(unsigned int auiFlags);

		ErrorCode Mount()
		{
			if (bMounted)
				return EC_NONE;
			ErrorCode eResult = DoMount();
			if (eResult == EC_NONE)
				bMounted = true;
			return eResult;
		}
		ErrorCode CreateStream(const char* apPath, BSTSmartPointer<Stream>& arStream, Location*& arLocation, bool abReadOnly)
		{
			return apPath ? DoCreateStream(apPath, arStream, arLocation, abReadOnly) : EC_INVALID_PATH;
		}
		ErrorCode CreateAsyncStream(const char* apPath, BSTSmartPointer<AsyncStream>& arStream, Location*& arLocation, bool abReadOnly)
		{
			return apPath ? DoCreateAsyncStream(apPath, arStream, arLocation, abReadOnly) : EC_INVALID_PATH;
		}

		bool bMounted;
		char cPadding[7];
	};

	static_assert(sizeof(Info) == 24);
	static_assert(offsetof(Info, ModifyTime) == 0);
	static_assert(offsetof(Info, CreateTime) == 8);
	static_assert(offsetof(Info, uiFileSize) == 16);
	static_assert(sizeof(Location) == 16);
	static_assert(offsetof(Location, bMounted) == 8);
}
