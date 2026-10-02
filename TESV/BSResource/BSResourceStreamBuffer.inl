#pragma once

#include <cstring>
#include <cerrno>
#include <cstdlib>

namespace BSResource
{
	template <class Traits>
	ErrorCode StreamBuffer<Traits>::LockAtRead(void*& arBuffer, unsigned int& arSize, uint64_t auiOffset)
	{
		uiFlags |= 1;
		ErrorCode eError = EC_NONE;
		if (uiEndPosInStream == uiStartPosInStream || auiOffset < uiStartPosInStream || auiOffset >= uiEndPosInStream)
		{
			uint64_t uiAligned = auiOffset & ~uint64_t(4095);
			if (uiAligned < uiStartPosInStream || uiAligned >= uiEndPosInStream)
			{
				uint64_t uiDelta = uiAligned - uiPosInStream;
				if (uiDelta != 0)
				{
					eError = this->MovePosition(*rStream, static_cast<int64_t>(uiDelta), uiPosInStream);
					if (eError != EC_NONE)
						return eError;
				}
				unsigned int uiBytes = uiBufferSize;
				unsigned int uiRounded = (arSize + 4095) & ~4095u;
				if (uiRounded && uiRounded < uiBytes)
					uiBytes = uiRounded;
				uint64_t uiRead = 0;
				eError = this->Read(*rStream, pBuffer, uiBytes, uiRead);
				if (uiRead)
				{
					uiStartPosInStream = uiPosInStream;
					uiPosInStream += uiRead;
					uiEndPosInStream = uiPosInStream;
				}
				if (eError != EC_NONE)
					return eError;
			}
		}
		if (uiStartPosInStream >= uiEndPosInStream || uiStartPosInStream > auiOffset || auiOffset >= uiEndPosInStream)
			arSize = 0;
		else
		{
			arBuffer = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pBuffer) + auiOffset - uiStartPosInStream);
			arSize = static_cast<unsigned int>(uiEndPosInStream - auiOffset);
		}
		return eError;
	}

	inline void CopyBuffer(void* apDestination, unsigned int auiCapacity, const void* apSource, uint64_t auiSize)
	{
		if (apDestination && apSource && auiSize <= auiCapacity)
			memcpy(apDestination, apSource, static_cast<size_t>(auiSize));
		else
		{
			if (apDestination)
				memset(apDestination, 0, auiCapacity);
			errno = apDestination && apSource ? ERANGE : EINVAL;
			_invalid_parameter_noinfo();
		}
	}

	template <class Traits>
	ErrorCode StreamBuffer<Traits>::ReadAt(void* apBuffer, uint64_t auiOffset, uint64_t auiBytes, uint64_t& arRead)
	{
		if (!uiBufferSize)
		{
			arRead = 0;
			return EC_NONE;
		}
		uint64_t uiTotal = 0;
		unsigned int uiAvailable = static_cast<unsigned int>(auiBytes);
		void* pSource = nullptr;
		ErrorCode eError = LockAtRead(pSource, uiAvailable, auiOffset);
		uint64_t uiChunk = auiBytes < uiAvailable ? auiBytes : uiAvailable;
		while (eError == EC_NONE && uiChunk && auiBytes)
		{
			if (uiAvailable)
			{
				CopyBuffer(apBuffer, static_cast<unsigned int>(auiBytes), pSource, uiChunk);
				apBuffer = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(apBuffer) + uiChunk);
				uiTotal += uiChunk;
				auiBytes -= uiChunk;
				uiAvailable = 0;
			}
			else
			{
				unsigned int uiBytes = uiBufferSize;
				unsigned int uiRounded = (static_cast<unsigned int>(auiBytes) + 4095) & ~4095u;
				if (uiRounded && uiRounded < uiBytes)
					uiBytes = uiRounded;
				uint64_t uiRead = 0;
				eError = this->Read(*rStream, pBuffer, uiBytes, uiRead);
				uiAvailable = static_cast<unsigned int>(uiRead);
				pSource = pBuffer;
				if (uiRead)
				{
					uiStartPosInStream = uiPosInStream;
					uiPosInStream += uiRead;
					uiEndPosInStream = uiPosInStream;
				}
				uiChunk = auiBytes < uiAvailable ? auiBytes : uiAvailable;
			}
		}
		uiFlags &= ~1u;
		arRead = uiTotal;
		return eError;
	}

}
