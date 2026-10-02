#include "BSResource/BSResourceArchive2.h"
#include "BSResource/BSResourceStreamBuffer.inl"
#include "BSResource/BSResourceReadBuffer.h"
#include "BSCore/BSMemoryutility.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/MemoryContextTracker.h"
#include "BSCore/ScrapHeap.h"

#include <new>
#include <emmintrin.h>

extern "C"
{
	size_t LZ4F_createDecompressionContext(void**, unsigned int);
	size_t LZ4F_freeDecompressionContext(void*);
	size_t LZ4F_decompress(void*, void*, size_t*, const void*, size_t*, const void*);
	unsigned int LZ4F_isError(size_t);
	const char* LZ4F_getErrorName(size_t);
}

template <class Char, unsigned int SIZE>
class FixedLengthMemoryManagementPol
{
public:
	Char pMemberString[SIZE];
};

template <class Char, unsigned int SIZE, template <class, unsigned int> class MemoryPolicy>
class BSStringT : public MemoryPolicy<Char, SIZE>
{
public:
	BSStringT() : pString(nullptr), sLen(0), sMaxLen(0) {}
	int SPrintF(const char*, ...);
	Char* pString;
	unsigned short sLen;
	unsigned short sMaxLen;
};

namespace BSResource
{
	static void SetCompressionFlags(volatile unsigned int& arFlags, unsigned int auiFlags)
	{
		LONG iOld;
		do { iOld = arFlags; }
		while (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&arFlags), iOld & 0xfffff00f, iOld) != iOld);
		do { iOld = arFlags; }
		while (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&arFlags), iOld | auiFlags, iOld) != iOld);
	}
	static void AddCompressionFlags(volatile unsigned int& arFlags, unsigned int auiFlags)
	{
		LONG iOld;
		do { iOld = arFlags; }
		while (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&arFlags), iOld | auiFlags, iOld) != iOld);
	}
	CompressedArchiveStream::CompressedArchiveStream(const BSTSmartPointer<ArchiveSource>& arSource, unsigned int auiOffset,
		unsigned int auiSize, bool abPlatform, const BSFixedString& arName, bool abWritable) :
		ArchiveStream(arSource, auiOffset, auiSize, arName, abWritable), pContext(nullptr)
	{
		AddCompressionFlags(uiFlags, 16 * (static_cast<unsigned int>(abPlatform) + 1));
	}
	CompressedArchiveStream::CompressedArchiveStream(const CompressedArchiveStream& arStream) : ArchiveStream(arStream), pContext(nullptr)
	{
		AddCompressionFlags(uiFlags, arStream.uiFlags & 0xff0);
	}
	CompressedArchiveStream::~CompressedArchiveStream() = default;
	unsigned int CompressedArchiveStream::DoGetOffset() const
	{
		return pContext ? pContext->uiCompressedEnd - uiOffset : static_cast<unsigned int>(uiTotalSize);
	}
	ErrorCode CompressedArchiveStream::ReadFromSourceAt(void* apBuffer, uint64_t auiOffset, uint64_t auiBytes, uint64_t& arRead) const
	{
		BSTSmartPointer<ArchiveSource> Source;
		GetCurrentSource(Source);
		return Source->ReadAt(apBuffer, auiOffset, auiBytes, arRead);
	}
	void CompressedArchiveStream::DoClone(BSTSmartPointer<Stream>& arStream) const
	{
		AutoMemContext MemoryContext(static_cast<MEM_CONTEXT>(9));
		void* pStorage = MemoryManager::Instance().Allocate(sizeof(CompressedArchiveStream), 0, false);
		arStream = pStorage ? new (pStorage) CompressedArchiveStream(*this) : nullptr;
	}
	ErrorCode CompressedArchiveStream::DoOpen()
	{
		BSTSmartPointer<ArchiveSource> Source;
		GetCurrentSource(Source);
		unsigned int uiStart = uiOffset;
		if (uiStart <= uiStart + DoGetOffset())
			uiPosition = uiStart;
		ErrorCode eError = ReadEmbeddedName(Source);
		if (eError != EC_NONE)
			return eError;
		unsigned int uiSize = 0;
		uint64_t uiTransferred = 4;
		uiRead = 0;
		eError = Source->ReadAt(&uiSize, uiPosition, 4, uiTransferred);
		if (eError != EC_NONE)
			return eError;
		unsigned int uiCompressedSize = static_cast<unsigned int>(uiTotalSize);
		uiTotalSize = uiSize;
		unsigned int uiNext = static_cast<unsigned int>(uiTransferred) + uiPosition;
		uiStart = uiOffset;
		if (uiNext >= uiStart && uiNext <= uiStart + DoGetOffset())
			uiPosition = uiNext;
		uiCompressedSize += uiOffset - uiPosition;
		if (uiFlags & 0x20)
			return PlatformCreateContext(uiCompressedSize, Source->pLocation->DoQBufferHint());
		return StandardCreateContext(uiCompressedSize, uiSize);
	}
	ErrorCode CompressedArchiveStream::DoRead(void* apBuffer, uint64_t auiBytes, uint64_t& arRead) const
	{
		arRead = 0;
		if (!auiBytes)
			return EC_NONE;
		return (uiFlags & 0x10) ? StandardRead(apBuffer, auiBytes, arRead) : PlatformRead(apBuffer, auiBytes, arRead);
	}
	ErrorCode CompressedArchiveStream::DoSeek(int64_t aiOffset, SeekMode aeMode, uint64_t& arPosition) const
	{
		if (aeMode != SM_CUR || aiOffset < 0)
			return EC_UNSUPPORTED;
		uint64_t uiTransferred = 0;
		ErrorCode eError = (uiFlags & 0x10) ? StandardRead(nullptr, aiOffset, uiTransferred) : PlatformRead(nullptr, aiOffset, uiTransferred);
		arPosition = uiRead;
		return eError;
	}
	ErrorCode CompressedArchiveStream::DoCreateAsync(BSTSmartPointer<AsyncStream>&) const { return EC_UNSUPPORTED; }
	void CompressedArchiveStream::DoClose()
	{
		if (uiFlags & 0x20)
		{
			PlatformDestroyContext();
			SetCompressionFlags(uiFlags, 0x20);
		}
		else
		{
			StandardDestroyContext();
			SetCompressionFlags(uiFlags, 0x10);
		}
		ClearEmbeddedName();
		pContext = nullptr;
	}
	ErrorCode CompressedArchiveStream::StandardCreateContext(unsigned int auiCompressedSize, uint64_t auiSize)
	{
		uint64_t uiSize = auiSize + sizeof(Context);
		SetCompressionFlags(uiFlags, 0x10);
		pContext = nullptr;
		if (uiFlags & 4)
		{
			uint64_t uiCapacity = ReadBuffer::QBufferSize();
			if (uiSize < uiCapacity)
				uiCapacity = uiSize;
			pContext = static_cast<Context*>(ReadBuffer::Allocate(static_cast<unsigned int>(uiCapacity)));
			if (pContext)
				AddCompressionFlags(uiFlags, 0x40);
			else
			{
				uiCapacity = static_cast<unsigned int>(ScrapHeap::QMaxMemory());
				if (uiSize < uiCapacity)
					uiCapacity = uiSize;
				pContext = static_cast<Context*>(MemoryManager::Instance().GetThreadScrapHeap()->Allocate(uiCapacity, 8));
				if (pContext)
					AddCompressionFlags(uiFlags, 0x80);
			}
		}
		if (!(uiFlags & 0xc0))
			pContext = static_cast<Context*>(MemoryManager::Instance().Allocate(uiSize, 8, true));
		Context* pCurrent = pContext;
		LZ4F_createDecompressionContext(&pCurrent->pDecompressionContext, 100);
		pCurrent->pBuffer = pCurrent + 1;
		pCurrent->uiBufferSize = static_cast<unsigned int>(auiSize);
		pCurrent->uiCompressedEnd = auiCompressedSize + uiPosition;
		pCurrent->uiConsumed = pCurrent->uiProduced = 0;
		return EC_NONE;
	}
	void CompressedArchiveStream::StandardDestroyContext()
	{
		if (pContext)
		{
			uiTotalSize = pContext->uiCompressedEnd - uiOffset;
			LZ4F_freeDecompressionContext(pContext->pDecompressionContext);
			unsigned int uiAllocation = uiFlags;
			Context* pStorage = pContext;
			if (uiAllocation & 0x40)
				ReadBuffer::Deallocate(pContext);
			else if (uiAllocation & 0x80)
				MemoryManager::Instance().GetThreadScrapHeap()->Deallocate(pStorage);
			else
				MemoryManager::Instance().Deallocate(pStorage, true);
		}
		pContext = nullptr;
	}
	ErrorCode CompressedArchiveStream::StandardRead(void* apBuffer, uint64_t auiBytes, uint64_t& arRead) const
	{
		Context* pCurrent = pContext;
		char* pOutput = static_cast<char*>(pCurrent->pBuffer);
		uint64_t uiPositionNew = uiPosition;
		uint64_t uiEnd = pCurrent->uiCompressedEnd;
		uint64_t uiRemaining = static_cast<unsigned int>(uiTotalSize) - uiRead;
		if (auiBytes < uiRemaining)
			uiRemaining = auiBytes;
		BSTSmartPointer<ArchiveSource> Source;
		GetCurrentSource(Source);
		uint64_t uiAvailable = pCurrent->uiProduced - pCurrent->uiConsumed;
		uint64_t uiChunk = uiRemaining < uiAvailable ? uiRemaining : uiAvailable;
		uint64_t uiTotal = 0;
		char* pDestination = static_cast<char*>(apBuffer);
		if (uiChunk)
		{
			if (pDestination)
			{
				BSmemcpy(pDestination, uiRemaining, pOutput + pCurrent->uiConsumed, uiChunk);
				pDestination += uiChunk;
			}
			pCurrent->uiConsumed += static_cast<unsigned int>(uiChunk);
			uiTotal = uiChunk;
			uiRemaining -= uiChunk;
		}
		ErrorCode eError = EC_NONE;
		if (uiRemaining && uiPositionNew < uiEnd)
		{
			void* pDecoder = pCurrent->pDecompressionContext;
			uint64_t uiCompressedRemaining = uiEnd - uiPositionNew;
			unsigned int uiInputSize = static_cast<unsigned int>(uiCompressedRemaining);
			void* pInput = nullptr;
			ArchiveSource* pSource = Source.pPtr;
			eError = pSource->LockAtRead(pInput, uiInputSize, uiPositionNew);
			uint64_t uiInputAvailable = uiCompressedRemaining < uiInputSize ? uiCompressedRemaining : uiInputSize;
			if (uiInputAvailable && eError == EC_NONE)
			{
				pCurrent->uiConsumed = pCurrent->uiProduced = 0;
				size_t uiResult;
				bool bSuccess;
				do
				{
					size_t uiConsumed = uiInputAvailable;
					size_t uiProduced = pCurrent->uiBufferSize;
					uiResult = LZ4F_decompress(pDecoder, pOutput, &uiProduced, pInput, &uiConsumed, nullptr);
					bSuccess = LZ4F_isError(uiResult) == 0;
					if (!bSuccess)
						continue;
					uiPositionNew += uiConsumed;
					uiCompressedRemaining = uiEnd - uiPositionNew;
					pCurrent->uiProduced = static_cast<unsigned int>(uiProduced);
					unsigned int uiCopy = static_cast<unsigned int>(uiRemaining);
					if (static_cast<unsigned int>(uiProduced) < uiCopy)
						uiCopy = static_cast<unsigned int>(uiProduced);
					if (uiCopy)
					{
						if (pDestination)
						{
							BSmemcpy(pDestination, uiRemaining, pOutput, uiCopy);
							pDestination += uiCopy;
						}
						pCurrent->uiConsumed += uiCopy;
						uiTotal += uiCopy;
						uiRemaining -= uiCopy;
					}
					if (!uiRemaining)
						break;
					pCurrent->uiConsumed = pCurrent->uiProduced = 0;
					if (uiConsumed >= uiInputAvailable)
					{
						StreamBuffer<ArchiveSourceSBTraits>& SourceBuffer = pSource->Buffer;
						uint64_t uiTransferred = 0;
						unsigned int uiBytes = SourceBuffer.uiBufferSize;
						unsigned int uiRounded = (static_cast<unsigned int>(uiCompressedRemaining) + 4095) & ~4095u;
						if (uiRounded && uiRounded < uiBytes)
							uiBytes = uiRounded;
						pInput = SourceBuffer.pBuffer;
						if (SourceBuffer.rStream->QWritable())
							eError = EC_UNSUPPORTED;
						else
						{
							eError = SourceBuffer.rStream->DoRead(SourceBuffer.pBuffer, uiBytes, uiTransferred);
							pInput = SourceBuffer.pBuffer;
							if (uiTransferred)
							{
								SourceBuffer.uiStartPosInStream = SourceBuffer.uiPosInStream;
								SourceBuffer.uiPosInStream += uiTransferred;
								SourceBuffer.uiEndPosInStream = SourceBuffer.uiPosInStream;
							}
						}
						uiInputAvailable = uiCompressedRemaining < static_cast<unsigned int>(uiTransferred) ? uiCompressedRemaining : static_cast<unsigned int>(uiTransferred);
						if (eError != EC_NONE)
							break;
					}
					else
					{
						pInput = static_cast<char*>(pInput) + uiConsumed;
						uiInputAvailable -= uiConsumed;
					}
				} while (bSuccess && uiResult && uiCompressedRemaining);
				pSource->Unlock();
				if (LZ4F_isError(uiResult))
				{
					BSStringT<char, 260, FixedLengthMemoryManagementPol> Message;
					unsigned int uiArchivePosition = uiPosition;
					const char* pName = FileName.pString;
					const char* pError = LZ4F_getErrorName(uiResult);
					Message.SPrintF("Decompression error: %s. Archive Name: %s. posInArchive : %u. Compressed end: %llu. TotalRead: %llu. eresult:  %u. RemainingCompbytes: %llu. DecompressedBufferSize: %u. RequestedBytes: %u. \n",
						pError, pName, uiArchivePosition, uiEnd, uiTotal, eError, uiCompressedRemaining, pCurrent->uiBufferSize, static_cast<unsigned int>(auiBytes));
					uiPositionNew = uiEnd;
					eError = EC_FILE_ERROR;
				}
			}
		}
		unsigned int uiStart = uiOffset;
		if (uiPositionNew >= uiStart && uiPositionNew <= uiStart + DoGetOffset())
			uiPosition = static_cast<unsigned int>(uiPositionNew);
		uiRead += static_cast<unsigned int>(uiTotal);
		arRead = uiTotal;
		return eError;
	}

	ArchiveInfo::ArchiveInfo(ArchiveSource& arSource) : pDirectoryEntries(nullptr),
		pHeap(MemoryManager::Instance().GetThreadScrapHeap()), pAllocation(nullptr), uiTotalFiles(0),
		pDirStringNames(nullptr), pFileStringNames(nullptr)
	{
		uint64_t uiRead;
		arSource.ReadAt(&Header, 0, sizeof(Header), uiRead);
		uint64_t uiPosition = uiRead;
		unsigned int uiDirectories = Header.uiDirectoryCount;
		pDirectoryEntries = static_cast<BSDirectoryEntry*>(pHeap->Allocate(uint64_t(uiDirectories) * sizeof(BSDirectoryEntry), 8));
		arSource.ReadAt(pDirectoryEntries, uiPosition, uint64_t(uiDirectories) * sizeof(BSDirectoryEntry), uiRead);
		uiPosition += uiRead;
		unsigned int uiBytes = Header.uiDirectoryNameLength + Header.uiFileNameLength;
		for (unsigned int i = 0; i < uiDirectories; ++i)
			uiBytes += 20 * pDirectoryEntries[i].uiFileCount;
		pAllocation = pHeap->Allocate(uiBytes, 8);
		char* pDirectoryName = static_cast<char*>(pAllocation);
		pDirStringNames = pDirectoryName;
		pFileStringNames = pDirectoryName + Header.uiDirectoryNameLength;
		unsigned int uiFileNamesSize = Header.uiFileNameLength;
		char* pFiles = pFileStringNames + uiFileNamesSize;
		unsigned char ucLength = 0;
		for (unsigned int i = 0; i < uiDirectories; ++i)
		{
			arSource.ReadAt(&ucLength, uiPosition, 1, uiRead);
			uiPosition += uiRead;
			uint64_t uiLength = ucLength;
			arSource.ReadAt(pDirectoryName, uiPosition, uiLength, uiRead);
			uiPosition += uiRead;
			pDirectoryName += uiLength;
			uint64_t uiFiles = pDirectoryEntries[i].uiFileCount;
			arSource.ReadAt(pFiles, uiPosition, uiFiles * sizeof(BSFileEntry), uiRead);
			uiPosition += uiRead;
			pDirectoryEntries[i].pFileEntries = reinterpret_cast<BSFileEntry*>(pFiles);
			pFiles += uint64_t(pDirectoryEntries[i].uiFileCount) * sizeof(BSFileEntry);
		}
		if (uiDirectories)
			uiFileNamesSize = Header.uiFileNameLength;
		arSource.ReadAt(pFileStringNames, uiPosition, uiFileNamesSize, uiRead);
		uiPosition += uiRead;
		for (unsigned int i = 0; i < uiDirectories; ++i)
			uiTotalFiles += pDirectoryEntries[i].uiFileCount;
		arSource.SetTOCEnd(static_cast<unsigned int>(uiPosition));
	}
	ArchiveInfo::~ArchiveInfo()
	{
		if (pAllocation)
			pHeap->Deallocate(pAllocation);
		pHeap->Deallocate(pDirectoryEntries);
	}
	const BSDirectoryEntry& ArchiveInfo::QDirectoryEntry(unsigned int auiIndex) const { return pDirectoryEntries[auiIndex]; }

	ErrorCode ArchiveSourceSBTraits::MovePosition(Stream& arStream, int64_t aiOffset, uint64_t& arPosition)
	{
		return arStream.DoSeek(aiOffset, SM_CUR, arPosition);
	}
	ErrorCode ArchiveSourceSBTraits::Read(Stream& arStream, void* apBuffer, uint64_t auiBytes, uint64_t& arRead)
	{
		return arStream.QWritable() ? EC_UNSUPPORTED : arStream.DoRead(apBuffer, auiBytes, arRead);
	}
	template ErrorCode StreamBuffer<ArchiveSourceSBTraits>::LockAtRead(void*&, unsigned int&, uint64_t);
	template ErrorCode StreamBuffer<ArchiveSourceSBTraits>::ReadAt(void*, uint64_t, uint64_t, uint64_t&);

	ArchiveSource::ArchiveSource(BSTSmartPointer<Stream>& arStream, Location* apLocation) :
		uiRefCount(0), uiLock(0), spStream(arStream), spAsyncStream(nullptr), pLocation(apLocation)
	{
		pBuffer = MemoryManager::Instance().Allocate(apLocation->DoQBufferHint(), 0, false);
		unsigned int uiSize = apLocation->DoQBufferHint();
		Buffer.rStream = arStream.pPtr;
		Buffer.uiStartPosInStream = Buffer.uiEndPosInStream = Buffer.uiPosInStream = 0;
		Buffer.uiBufferSize = uiSize;
		Buffer.pBuffer = pBuffer;
		Buffer.uiFlags = 0;
		uiTOCEnd = 0;
		bEmbeddedFileNames = false;
	}
	ErrorCode ArchiveSource::ReadAt(void* apBuffer, uint64_t auiOffset, uint64_t auiBytes, uint64_t& arRead)
	{
		while (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&uiLock), 1, 0))
			Sleep(0);
		_mm_mfence();
		ErrorCode eError = Buffer.ReadAt(apBuffer, auiOffset, auiBytes, arRead);
		uiLock = 0;
		_mm_mfence();
		return eError;
	}
	ErrorCode ArchiveSource::LockAtRead(void*& arBuffer, unsigned int& arSize, uint64_t auiOffset)
	{
		while (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&uiLock), 1, 0))
			Sleep(0);
		_mm_mfence();
		return Buffer.LockAtRead(arBuffer, arSize, auiOffset);
	}
	ErrorCode ArchiveSource::Fill(void*& arBuffer, unsigned int& arSize)
	{
		return Buffer.LockAtRead(arBuffer, arSize, Buffer.uiEndPosInStream);
	}
	void ArchiveSource::Unlock()
	{
		Buffer.uiFlags &= ~1u;
		uiLock = 0;
		_mm_mfence();
	}
	void ArchiveSource::SetTOCEnd(unsigned int auiOffset) { uiTOCEnd = auiOffset; }

	namespace __anonymous_namespace__
	{
		class AsyncArchiveStream : public AsyncStream
		{
		public:
			AsyncArchiveStream(const BSTSmartPointer<AsyncStream>& arSource, unsigned int auiSize,
				unsigned int auiOffset, Location* apLocation);
			AsyncArchiveStream(const AsyncArchiveStream& arStream) :
				AsyncStream(arStream.uiTotalSize, arStream.QWritable(), arStream.uiMinPacketSize),
				spSource(arStream.spSource), uiOffset(arStream.uiOffset), pCurrentBuffer(nullptr) {}
			~AsyncArchiveStream() override = default;
			ErrorCode DoOpen() override
			{
				AsyncStream* pSource = spSource.pPtr;
				LONG iOld;
				do { iOld = pSource->uiFlags; }
				while (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&pSource->uiFlags), iOld | 8, iOld) != iOld);
				return pSource->DoOpen();
			}
			void DoClose() override
			{
				AsyncStream* pSource = spSource.pPtr;
				LONG iOld;
				do { iOld = pSource->uiFlags; }
				while (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&pSource->uiFlags), iOld & ~14, iOld) != iOld);
				pSource->DoClose();
			}
			void DoClone(BSTSmartPointer<AsyncStream>& arStream) const override
			{
				AutoMemContext Context(static_cast<MEM_CONTEXT>(8));
				void* pStorage = MemoryManager::Instance().Allocate(sizeof(AsyncArchiveStream), 0, false);
				arStream = pStorage ? new (pStorage) AsyncArchiveStream(*this) : nullptr;
			}
			ErrorCode DoStartRead(void* apBuffer, uint64_t auiBytes, uint64_t auiOffset) const override
			{
				AsyncStream* pSource = spSource.pPtr;
				return pSource->QWritable() ? EC_UNSUPPORTED : pSource->DoStartRead(apBuffer, auiBytes, auiOffset + uiOffset);
			}
			ErrorCode DoStartPacketAlignedBufferedRead(PacketAlignedBuffer* apBuffer, uint64_t auiBytes, uint64_t auiOffset) const override
			{
				AsyncStream* pSource = spSource.pPtr;
				if (pSource->QWritable())
					return EC_UNSUPPORTED;
				ErrorCode eError = pSource->DoStartPacketAlignedBufferedRead(apBuffer, auiBytes, auiOffset + uiOffset);
				if (eError == EC_NONE)
					pCurrentBuffer = apBuffer;
				return eError;
			}
			ErrorCode DoStartWrite(const void*, uint64_t, uint64_t) const override { return EC_UNSUPPORTED; }
			ErrorCode DoTruncate(uint64_t) const override { return EC_UNSUPPORTED; }
			ErrorCode DoWait(uint64_t& arTransferred, bool abWait) override
			{
				ErrorCode eError = spSource->DoWait(arTransferred, abWait);
				if (eError == EC_NONE && pCurrentBuffer)
					pCurrentBuffer->uiResultOffset -= uiOffset;
				pCurrentBuffer = nullptr;
				return eError;
			}
			BSTSmartPointer<AsyncStream> spSource;
			unsigned int uiOffset;
			mutable PacketAlignedBuffer* pCurrentBuffer;
		};
		static_assert(sizeof(AsyncArchiveStream) == 56);
		static_assert(offsetof(AsyncArchiveStream, spSource) == 32);
		static_assert(offsetof(AsyncArchiveStream, pCurrentBuffer) == 48);
		AsyncArchiveStream::AsyncArchiveStream(const BSTSmartPointer<AsyncStream>& arSource, unsigned int auiSize,
			unsigned int auiOffset, Location* apLocation) : AsyncStream(auiSize, false, apLocation),
			spSource(arSource), uiOffset(auiOffset), pCurrentBuffer(nullptr) {}
	}

	ErrorCode ArchiveSource::GetOrCreateAsyncStream(BSTSmartPointer<AsyncStream>& arStream)
	{
		BSTSmartPointer<AsyncStream> Previous;
		if (spAsyncStream.pPtr)
		{
			spAsyncStream->DoClone(arStream);
			return EC_NONE;
		}
		ErrorCode eError = spStream->DoCreateAsync(arStream);
		if (eError == EC_NONE)
		{
			AsyncStream* pExpected = spAsyncStream.pPtr;
			Previous = static_cast<AsyncStream*>(InterlockedCompareExchangePointer(
				reinterpret_cast<void* volatile*>(&spAsyncStream.pPtr), arStream.pPtr, pExpected));
			if (Previous.pPtr == pExpected && pExpected != arStream.pPtr)
			{
				BSTSmartPointerIntrusiveRefCount::Acquire(arStream.pPtr);
				BSTSmartPointerIntrusiveRefCount::Release(pExpected);
			}
		}
		return eError;
	}

	ErrorCode ArchiveStream::DoCreateAsync(BSTSmartPointer<AsyncStream>& arStream) const
	{
		BSTSmartPointer<AsyncStream> Source;
		ErrorCode eError = spSource->GetOrCreateAsyncStream(Source);
		if (eError == EC_NONE)
		{
			AutoMemContext Context(static_cast<MEM_CONTEXT>(8));
			void* pStorage = MemoryManager::Instance().Allocate(sizeof(__anonymous_namespace__::AsyncArchiveStream), 0, false);
			__anonymous_namespace__::AsyncArchiveStream* pStream = nullptr;
			if (pStorage)
			{
				Location* pLocation = spSource->pLocation;
				unsigned int uiStart = uiOffset;
				unsigned int uiSize = DoGetOffset();
				pStream = new (pStorage) __anonymous_namespace__::AsyncArchiveStream(Source, uiSize, uiStart, pLocation);
			}
			arStream = pStream;
		}
		return eError;
	}

	ArchiveSource::~ArchiveSource()
	{
		MemoryManager::Instance().Deallocate(pBuffer, false);
		if ((Buffer.uiFlags & 2) && Buffer.uiStartPosInStream < Buffer.uiPosInStream && Buffer.uiPosInStream <= Buffer.uiEndPosInStream)
		{
			uint64_t uiWritten = 0;
			if (Buffer.rStream->QWritable())
				Buffer.rStream->DoWrite(Buffer.pBuffer, Buffer.uiPosInStream - Buffer.uiStartPosInStream, uiWritten);
		}
	}

	void* ArchiveSource::Delete(unsigned int auiFlags)
	{
		this->~ArchiveSource();
		if (auiFlags & 1)
			::operator delete(this, sizeof(ArchiveSource));
		return this;
	}

	ArchiveStream::ArchiveStream(const BSTSmartPointer<ArchiveSource>& arSource, unsigned int auiOffset,
		unsigned int auiSize, const BSFixedString& arName, bool) : Stream(auiSize, false), spSource(arSource),
		uiOffset(auiOffset), uiPosition(auiOffset), FileName(arName) {}

	ArchiveStream::ArchiveStream(const ArchiveStream& arStream) : Stream(arStream.uiTotalSize, arStream.QWritable()),
		spSource(arStream.spSource), uiOffset(arStream.uiOffset), uiPosition(arStream.uiOffset), FileName(arStream.FileName) {}
	ArchiveStream::~ArchiveStream() = default;

	void ArchiveStream::SetSource(const BSTSmartPointer<ArchiveSource>& arSource)
	{
		BSTSmartPointer<ArchiveSource> Previous;
		ArchiveSource* pExpected;
		do
		{
			pExpected = spSource.pPtr;
			ArchiveSource* pActual = static_cast<ArchiveSource*>(InterlockedCompareExchangePointer(
				reinterpret_cast<void* volatile*>(&spSource.pPtr), arSource.pPtr, pExpected));
			Previous = pActual;
		} while (Previous.pPtr != pExpected);
		if (pExpected != arSource.pPtr)
		{
			BSTSmartPointerIntrusiveRefCount::Acquire(arSource.pPtr);
			BSTSmartPointerIntrusiveRefCount::Release(pExpected);
		}
		_mm_mfence();
	}

	void ArchiveStream::GetCurrentSource(BSTSmartPointer<ArchiveSource>& arSource) const
	{
		ArchiveSource* pFirst = spSource.pPtr;
		BSTSmartPointerIntrusiveRefCount::Acquire(pFirst);
		ArchiveSource* pExpected = spSource.pPtr;
		ArchiveSource* pActual = static_cast<ArchiveSource*>(InterlockedCompareExchangePointer(
			reinterpret_cast<void* volatile*>(&spSource.pPtr), pFirst, pExpected));
		arSource = pActual;
		if (arSource.pPtr == pExpected && pExpected != pFirst)
		{
			BSTSmartPointerIntrusiveRefCount::Acquire(pFirst);
			BSTSmartPointerIntrusiveRefCount::Release(pExpected);
		}
		BSTSmartPointerIntrusiveRefCount::Release(pFirst);
	}

	const BSTSmartPointer<Stream>& ArchiveStream::GetSourceStream() const
	{
		BSTSmartPointer<ArchiveSource> Source;
		GetCurrentSource(Source);
		return Source->spStream;
	}

	Location* ArchiveStream::GetSourceLocation() const
	{
		BSTSmartPointer<ArchiveSource> Source;
		GetCurrentSource(Source);
		return Source->pLocation;
	}

	void ArchiveStream::DoClone(BSTSmartPointer<Stream>& arStream) const
	{
		void* pStorage = MemoryManager::Instance().Allocate(sizeof(ArchiveStream), 0, false);
		arStream = pStorage ? new (pStorage) ArchiveStream(*this) : nullptr;
	}

	ErrorCode ArchiveStream::DoOpen()
	{
		unsigned int uiStart = uiOffset;
		if (uiStart <= uiStart + DoGetOffset())
			uiPosition = uiStart;
		BSTSmartPointer<ArchiveSource> Source;
		GetCurrentSource(Source);
		return ReadEmbeddedName(Source);
	}

	void ArchiveStream::ClearEmbeddedName()
	{
		if (spSource->bEmbeddedFileNames)
			FileName << nullptr;
	}
	void ArchiveStream::DoClose() { ClearEmbeddedName(); }

	uint64_t ArchiveStream::DoGetKey() const
	{
		uint64_t uiStart = uiOffset;
		ArchiveSource* pSource;
		{
			BSTSmartPointer<ArchiveSource> Source;
			GetCurrentSource(Source);
			pSource = Source.pPtr;
		}
		return uiStart | (reinterpret_cast<uintptr_t>(pSource->spStream.pPtr) << 32);
	}

	ErrorCode ArchiveStream::DoRead(void* apBuffer, uint64_t auiBytes, uint64_t& arRead) const
	{
		unsigned int uiStart = uiOffset;
		uint64_t uiAvailable = uiStart + DoGetOffset() - uiPosition;
		BSTSmartPointer<ArchiveSource> Source;
		GetCurrentSource(Source);
		if (auiBytes > uiAvailable)
			auiBytes = uiAvailable;
		ArchiveSource* pSource = Source.pPtr;
		ErrorCode eError = pSource->ReadAt(apBuffer, uiPosition, auiBytes, arRead);
		uiPosition += static_cast<unsigned int>(arRead);
		return eError;
	}

	ErrorCode ArchiveStream::DoWrite(const void*, uint64_t, uint64_t& arWritten) const
	{
		arWritten = 0;
		return EC_UNSUPPORTED;
	}

	ErrorCode ArchiveStream::DoSeek(int64_t aiOffset, SeekMode aeMode, uint64_t& arPosition) const
	{
		uint64_t uiPositionNew = uiPosition;
		uint64_t uiStart = uiOffset;
		uint64_t uiEnd = uiStart + DoGetOffset();
		if (aeMode == SM_SET)
			uiPositionNew = uint64_t(aiOffset) + uiOffset;
		else if (aeMode == SM_CUR)
			uiPositionNew = uint64_t(aiOffset) + uiPosition;
		else if (aeMode == SM_END)
			uiPositionNew = uint64_t(aiOffset) + uiEnd;
		if (static_cast<int64_t>(uiPositionNew) < static_cast<int64_t>(uiStart))
			uiPositionNew = uiStart;
		else if (static_cast<int64_t>(uiPositionNew) > static_cast<int64_t>(uiEnd))
			uiPositionNew = uiEnd;
		uiPosition = static_cast<unsigned int>(uiPositionNew);
		arPosition = uint64_t(uiPosition) - uiStart;
		return EC_NONE;
	}

	bool ArchiveStream::DoGetName(BSFixedString& arName) const
	{
		arName = FileName;
		return FileName != nullptr;
	}
	unsigned int ArchiveStream::DoGetOffset() const { return static_cast<unsigned int>(uiTotalSize); }

	ErrorCode ArchiveStream::ReadEmbeddedName(const BSTSmartPointer<ArchiveSource>& arSource)
	{
		if (!arSource->bEmbeddedFileNames)
			return EC_NONE;
		AutoMemContext Context(static_cast<MEM_CONTEXT>(8));
		unsigned char ucLength = 0;
		uint64_t uiRead = 1;
		ErrorCode eError = arSource->ReadAt(&ucLength, uiPosition, 1, uiRead);
		if (eError == EC_NONE)
		{
			uiPosition += static_cast<unsigned int>(uiRead);
			char Name[260];
			uiRead = ucLength;
			eError = arSource->ReadAt(Name, uiPosition, ucLength, uiRead);
			unsigned int uiBytes = static_cast<unsigned int>(uiRead);
			Name[uiRead] = 0;
			FileName << Name;
			uiPosition += uiBytes;
		}
		return eError;
	}
}
