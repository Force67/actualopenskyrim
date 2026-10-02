#include "BSResource/BSResourceLooseFiles.h"
#include "BSResource/BSResourceLooseFileStreams.h"
#include "BSResource/BSResourceStreamBuffer.inl"
#include "BSResource/BSResource.h"
#include "BSResource/BSResourceReadBuffer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/MemoryContextTracker.h"
#include "BSSystem/BSSystemFile.h"

#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <new>
#include <utility>

namespace BSResource
{
	static ErrorCode TranslateErrorCode(BSSystemFile::ErrorCode aeError)
	{
		switch (aeError)
		{
		case BSSystemFile::EC_NONE: return EC_NONE;
		case BSSystemFile::EC_NOT_EXIST: return EC_NOT_EXIST;
		case BSSystemFile::EC_INVALID_PATH: return EC_INVALID_PATH;
		default: return EC_FILE_ERROR;
		}
	}

	static bool WaitCompletion(BSSystemFileAsyncFunctor& arFunctor, bool abWait)
	{
		if (arFunctor.uiStatus == 0)
			return true;
		if (GetCurrentThreadId() == arFunctor.uiThreadId)
		{
			if (abWait)
			{
				while (arFunctor.uiStatus != 1)
					SleepEx(INFINITE, TRUE);
			}
			else
			{
				SleepEx(1, TRUE);
				if (arFunctor.uiStatus != 1)
					return false;
			}
		}
		else
		{
			if (!abWait)
				return false;
			unsigned int uiAttempts = 0;
			do
			{
				DWORD uiDelay = uiAttempts >= 10000 ? 1 : 0;
				if (uiDelay == 0)
					++uiAttempts;
				Sleep(uiDelay);
			}
			while (arFunctor.uiStatus != 1);
		}
		arFunctor.uiStatus = 0;
		return true;
	}

	static BSSystemFile::ErrorCode TruncateFile(BSSystemFile& arFile, uint64_t auiSize)
	{
		int64_t iPosition;
		BSSystemFile::ErrorCode eError = arFile.DoSeek(static_cast<int64_t>(uint64_t(0) - auiSize), BSSystemFile::SM_END, &iPosition);
		return eError == BSSystemFile::EC_NONE ? arFile.DoSetEndOfFile() : eError;
	}

	ErrorCode LooseFileSBTraits::MovePosition(BSSystemFile& arFile, int64_t aiOffset, uint64_t& arPosition)
	{
		if ((arFile.uiFlags & 0x80000000) != 0)
		{
			PathID = (PathID + static_cast<uint64_t>(aiOffset)) & ~uint64_t(4095);
			arPosition = PathID;
			return EC_NONE;
		}
		BSSystemFile::ErrorCode eError = arFile.DoSeek(aiOffset, BSSystemFile::SM_CUR, reinterpret_cast<int64_t*>(&arPosition));
		arFile.uiFlags = (arFile.uiFlags & 0xa0000000) | eError;
		return TranslateErrorCode(eError);
	}

	ErrorCode LooseFileSBTraits::Read(BSSystemFile& arFile, void* apBuffer, uint64_t auiBytes, uint64_t& arRead)
	{
		if ((arFile.uiFlags & 0x80000000) != 0)
		{
			AsyncFunctor Functor;
			Functor.uiStatus = 0;
			Functor.eError = BSSystemFile::EC_NONE;
			Functor.uiTransferred = 0;
			BSSystemFile::ErrorCode eError = arFile.DoReadAsync(apBuffer,
				static_cast<unsigned int>((auiBytes + 4095) & ~uint64_t(4095)), static_cast<int64_t>(PathID), &Functor);
			arFile.uiFlags = (arFile.uiFlags & 0xa0000000) | eError;
			WaitCompletion(Functor, true);
			arRead = Functor.uiTransferred;
			PathID += arRead;
			return TranslateErrorCode(Functor.eError);
		}
		BSSystemFile::ErrorCode eError = arFile.DoRead(apBuffer, static_cast<unsigned int>(auiBytes), &arRead);
		arFile.uiFlags = (arFile.uiFlags & 0xa0000000) | eError;
		return TranslateErrorCode(eError);
	}

	template <class Traits>
	ErrorCode StreamBuffer<Traits>::LockAtWrite(void*& arBuffer, unsigned int& arSize, uint64_t auiOffset)
	{
		uiFlags |= 1;
		ErrorCode eError = EC_NONE;
		if (auiOffset < uiStartPosInStream || auiOffset >= uiEndPosInStream)
		{
			if (uiStartPosInStream < uiPosInStream && uiPosInStream <= uiEndPosInStream)
			{
				BSSystemFile* pFile = rStream;
				uint64_t uiWritten = 0;
				BSSystemFile::ErrorCode eFileError = pFile->DoWrite(pBuffer,
					static_cast<unsigned int>(uiPosInStream - uiStartPosInStream), &uiWritten);
				pFile->uiFlags = (pFile->uiFlags & 0xa0000000) | eFileError;
				eError = TranslateErrorCode(eFileError);
				if (eError != EC_NONE)
					return eError;
			}
			if (auiOffset == uiEndPosInStream)
			{
				uiStartPosInStream = uiPosInStream;
				uiEndPosInStream = uiPosInStream + uiBufferSize;
			}
			else
			{
				uint64_t uiRemainder = auiOffset % uiBufferSize;
				uint64_t uiAligned = auiOffset - uiRemainder;
				if (uiAligned != uiPosInStream)
				{
					eError = this->MovePosition(*rStream, static_cast<int64_t>(uiAligned - uiPosInStream), uiPosInStream);
					if (eError == EC_NONE && uiRemainder)
					{
						uint64_t uiRead = 0;
						eError = this->Read(*rStream, pBuffer, uiBufferSize, uiRead);
						if (eError == EC_NONE && uiRead)
						{
							BSSystemFile* pFile = rStream;
							if ((pFile->uiFlags & 0x80000000) != 0)
							{
								this->PathID = (this->PathID - uiRead) & ~uint64_t(4095);
								uiAligned = this->PathID;
							}
							else
							{
								BSSystemFile::ErrorCode eFileError = pFile->DoSeek(static_cast<int64_t>(uint64_t(0) - uiRead),
									BSSystemFile::SM_CUR, reinterpret_cast<int64_t*>(&uiAligned));
								pFile->uiFlags = (pFile->uiFlags & 0xa0000000) | eFileError;
								eError = TranslateErrorCode(eFileError);
							}
						}
					}
					uiStartPosInStream = uiAligned;
					uiEndPosInStream = uiAligned + uiBufferSize;
					uiPosInStream = auiOffset;
					if (eError != EC_NONE)
						return eError;
				}
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

	template <class Traits>
	ErrorCode StreamBuffer<Traits>::WriteAt(const void* apBuffer, uint64_t auiOffset, uint64_t auiBytes, uint64_t& arWritten)
	{
		uint64_t uiTotal = 0;
		uint64_t uiWritten = 0;
		unsigned int uiAvailable = 0;
		void* pDestination = nullptr;
		ErrorCode eError = LockAtWrite(pDestination, uiAvailable, auiOffset);
		while (eError == EC_NONE && auiBytes)
		{
			if (uiAvailable)
			{
				uint64_t uiChunk = auiBytes < uiAvailable ? auiBytes : uiAvailable;
				CopyBuffer(pDestination, uiAvailable, apBuffer, uiChunk);
				apBuffer = reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(apBuffer) + uiChunk);
				uiPosInStream += uiChunk;
				uiTotal += uiChunk;
				auiBytes -= uiChunk;
				uiAvailable -= static_cast<unsigned int>(uiChunk);
				uiWritten = uiChunk;
			}
			else
			{
				BSSystemFile* pFile = rStream;
				uint64_t uiFlushed = 0;
				BSSystemFile::ErrorCode eFileError = pFile->DoWrite(pBuffer, uiBufferSize, &uiFlushed);
				pFile->uiFlags = (pFile->uiFlags & 0xa0000000) | eFileError;
				eError = TranslateErrorCode(eFileError);
				uiStartPosInStream = uiPosInStream;
				uiEndPosInStream = uiPosInStream + uiBufferSize;
				uiAvailable = uiBufferSize;
				if (eError == EC_NONE && auiBytes >= uiAvailable)
				{
					uint64_t uiDirect = auiBytes - auiBytes % uiAvailable;
					if (uiDirect)
					{
						pFile = rStream;
						eFileError = pFile->DoWrite(apBuffer, static_cast<unsigned int>(uiDirect), &uiWritten);
						pFile->uiFlags = (pFile->uiFlags & 0xa0000000) | eFileError;
						eError = TranslateErrorCode(eFileError);
						if (eError == EC_NONE)
						{
							uiTotal += uiWritten;
							apBuffer = reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(apBuffer) + uiWritten);
							auiBytes -= uiWritten;
							uiPosInStream += uiWritten;
							uint64_t uiSize = uiBufferSize;
							if (uiWritten == uiDirect)
							{
								uiStartPosInStream = uiPosInStream;
								uiEndPosInStream = uiPosInStream + uiSize;
								uiAvailable = static_cast<unsigned int>(uiSize);
							}
							else
							{
								uiStartPosInStream = uiPosInStream - uiPosInStream % uiSize;
								uiEndPosInStream = uiStartPosInStream + uiSize;
								uiAvailable = static_cast<unsigned int>(uiSize - uiPosInStream % uiSize);
							}
						}
					}
				}
				pDestination = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pBuffer) + uiPosInStream - uiStartPosInStream);
			}
		}
		uiFlags &= ~1u;
		arWritten = uiTotal;
		return eError;
	}

	template struct StreamBuffer<LooseFileSBTraits>;

	LooseFileLocation::~LooseFileLocation() = default;

	void TranslatedPath::TranslatePath(const char* apPath, char* apBuffer, const char*& arFile, const char*& arEnd)
	{
		const char* pFile = apPath;
		const char* pEnd = apPath;
		while (*pEnd)
		{
			char c = *pEnd;
			if (c == '/' || c == '\\')
			{
				pFile = pEnd + 1;
				c = static_cast<char>(QPathSeparator());
			}
			++pEnd;
			*apBuffer++ = c;
		}
		*apBuffer = 0;
		arFile = pFile;
		arEnd = pEnd;
	}

	void LooseFileLocation::BuildCannonicalNames(const char* apPath, char* apDirectory, char* apFile)
	{
		char Buffer[260];
		const char* pFile = apPath;
		const char* pEnd = apPath;
		TranslatedPath::TranslatePath(apPath, Buffer, pFile, pEnd);
		if (pFile == apPath)
			*apDirectory = 0;
		else
		{
			Buffer[pFile - apPath] = 0;
			strcpy_s(apDirectory, 260, Buffer);
		}
		if (pEnd == pFile)
			*apFile = 0;
		else
			strcpy_s(apFile, 260, pFile);
	}

	ErrorCode LooseFileLocation::DoCreateStream(const char* apPath, BSTSmartPointer<Stream>& arStream, Location*& arLocation, bool abWritable)
	{
		char Directory[260], File[260];
		BuildCannonicalNames(apPath, Directory, File);
		return CreateStreamFromNames(Directory, File, arStream, arLocation, abWritable);
	}

	ErrorCode LooseFileLocation::DoCreateAsyncStream(const char* apPath, BSTSmartPointer<AsyncStream>& arStream, Location*& arLocation, bool abWritable)
	{
		if (!bAsyncSupported)
			return EC_UNSUPPORTED;
		char Directory[260], File[260];
		BuildCannonicalNames(apPath, Directory, File);
		return CreateAsyncStreamFromNames(Directory, File, arStream, arLocation, abWritable);
	}

	ErrorCode LooseFileLocation::DoTraversePrefix(const char* apPath, LocationTraverser& arTraverser)
	{
		char Buffer[260];
		const char* pFile = apPath;
		const char* pEnd = apPath;
		strcpy_s(Buffer, 260, Prefix.pString);
		TranslatedPath::TranslatePath(apPath, Buffer + Prefix.QLength(), pFile, pEnd);
		char* pRelative = Buffer + Prefix.QLength();
		char* pRelativeEnd = pRelative + (pFile - apPath);
		*pRelativeEnd = 0;
		if (!DirectoryExists(Buffer))
			return EC_NOT_EXIST;
		TraversePath(Buffer, pRelative, pRelativeEnd, arTraverser, *this);
		return EC_NONE;
	}

	ErrorCode LooseFileLocation::DoGetInfo(const char* apPath, Info& arInfo)
	{
		char Buffer[260];
		const char* pFile = apPath;
		const char* pEnd = apPath;
		strcpy_s(Buffer, 260, Prefix.pString);
		TranslatedPath::TranslatePath(apPath, Buffer + Prefix.QLength(), pFile, pEnd);
		BSSystemFile File(Buffer, BSSystemFile::AM_RDONLY, BSSystemFile::OM_NONE, false);
		ErrorCode eResult = EC_NOT_EXIST;
		if (File.GetErrorCode() == BSSystemFile::EC_NONE)
		{
			BSSystemFile::Info FileInfo;
			BSSystemFile::ErrorCode eFileError = File.DoGetInfo(&FileInfo);
			File.uiFlags = (File.uiFlags & 0xa0000000) | eFileError;
			eResult = TranslateErrorCode(eFileError);
			if (eResult == EC_NONE)
				arInfo = {FileInfo.ModifyTime, FileInfo.CreateTime, FileInfo.uiFileSize};
		}
		return eResult;
	}

	ErrorCode LooseFileLocation::DoGetInfo(const char* apPath, Info& arInfo, Location*& arLocation)
	{
		ErrorCode eResult = LooseFileLocation::DoGetInfo(apPath, arInfo);
		if (eResult == EC_NONE)
			arLocation = this;
		return eResult;
	}

	ErrorCode LooseFileLocation::DoDelete(const char* apPath)
	{
		char Buffer[260];
		const char* pFile = apPath;
		const char* pEnd = apPath;
		strcpy_s(Buffer, 260, Prefix.pString);
		TranslatedPath::TranslatePath(apPath, Buffer + Prefix.QLength(), pFile, pEnd);
		return TranslateErrorCode(BSSystemFile::Delete(Buffer));
	}

	const char* LooseFileLocation::DoGetName() const { return Prefix.pString; }
	unsigned int LooseFileLocation::DoGetMinimumAsyncPacketSize() const { return uiMinimumAsyncPacketSize; }

	ErrorCode LooseFileLocation::CreateStreamFromNames(const char* apDirectory, const char* apFile,
		BSTSmartPointer<Stream>& arStream, Location*& arLocation, bool abWritable)
	{
		uint64_t uiSize = 0;
		char Buffer[260];
		Buffer[0] = 0;
		const char* pPrefix = Prefix.pString;
		if (pPrefix)
			strcat_s(Buffer, 260, pPrefix);
		if (apDirectory)
			strcat_s(Buffer, 260, apDirectory);
		if (apFile)
			strcat_s(Buffer, 260, apFile);
		if (!abWritable && !FileExists(Buffer, uiSize))
			return EC_NOT_EXIST;
		AutoMemContext Context(static_cast<MEM_CONTEXT>(8));
		void* pStorage = MemoryManager::Instance().Allocate(96, 0, false);
		LooseFileStream* pStream = nullptr;
		if (pStorage)
		{
			BSFixedString File(apFile);
			BSFixedString Directory(apDirectory);
			pStream = new (pStorage) LooseFileStream(Prefix, Directory, File, uiSize, abWritable, this);
		}
		arStream = pStream;
		arLocation = this;
		return EC_NONE;
	}

	ErrorCode LooseFileLocation::CreateAsyncStreamFromNames(const char* apDirectory, const char* apFile,
		BSTSmartPointer<AsyncStream>& arStream, Location*& arLocation, bool abWritable)
	{
		if (!bAsyncSupported)
			return EC_UNSUPPORTED;
		unsigned int uiSize = 0;
		char Buffer[260];
		Buffer[0] = 0;
		const char* pPrefix = Prefix.pString;
		if (pPrefix)
			strcat_s(Buffer, 260, pPrefix);
		if (apDirectory)
			strcat_s(Buffer, 260, apDirectory);
		if (apFile)
			strcat_s(Buffer, 260, apFile);
		if (!abWritable)
		{
			uint64_t uiFileSize = 0;
			if (!FileExists(Buffer, uiFileSize))
				return EC_NOT_EXIST;
			uiSize = static_cast<unsigned int>(uiFileSize);
		}
		AutoMemContext Context(static_cast<MEM_CONTEXT>(8));
		void* pStorage = MemoryManager::Instance().Allocate(152, 0, false);
		LooseFileAsyncStream* pStream = nullptr;
		if (pStorage)
		{
			BSFixedString File(apFile);
			BSFixedString Directory(apDirectory);
			pStream = new (pStorage) LooseFileAsyncStream(Prefix, Directory, File, uiSize, abWritable, this);
		}
		arStream = pStream;
		arLocation = this;
		return EC_NONE;
	}

	bool bAllowAsyncIO;

	LooseFileStreamBase::LooseFileStreamBase(const BSFixedString& arPrefix, const BSFixedString& arDirectory, const BSFixedString& arFile) :
		Prefix(arPrefix), DirName(arDirectory), FileName(arFile) {}

	LooseFileStreamBase::~LooseFileStreamBase()
	{
		BSSystemFile Empty;
		File = std::move(Empty);
	}

	BSSystemFile::ErrorCode LooseFileStreamBase::OpenFile(bool abWritable, bool abAsync)
	{
		char Buffer[260];
		const char* pFile = FileName.pString;
		const char* pDirectory = DirName.pString;
		const char* pPrefix = Prefix.pString;
		Buffer[0] = 0;
		if (pPrefix)
			strcat_s(Buffer, 259, pPrefix);
		if (pDirectory)
			strcat_s(Buffer, 259, pDirectory);
		if (pFile)
			strcat_s(Buffer, 259, pFile);
		{
			BSSystemFile Opened(Buffer, abWritable ? BSSystemFile::AM_RDWR : BSSystemFile::AM_RDONLY,
				abWritable ? BSSystemFile::OM_APPEND : BSSystemFile::OM_NONE,
				abWritable ? abAsync : (abAsync || bAllowAsyncIO));
			File = std::move(Opened);
		}
		return File.GetErrorCode();
	}

	ErrorCode LooseFileStreamBase::GetFileInfo(Info& arInfo)
	{
		bool bClosed;
		{
			BSSystemFile Empty;
			bClosed = File == Empty;
		}
		BSSystemFile::Info FileInfo;
		ErrorCode eResult;
		if (bClosed)
		{
			char Buffer[260];
			const char* pFile = FileName.pString;
			const char* pDirectory = DirName.pString;
			const char* pPrefix = Prefix.pString;
			Buffer[0] = 0;
			if (pPrefix)
				strcat_s(Buffer, 259, pPrefix);
			if (pDirectory)
				strcat_s(Buffer, 259, pDirectory);
			if (pFile)
				strcat_s(Buffer, 259, pFile);
			BSSystemFile Opened(Buffer, BSSystemFile::AM_RDONLY, BSSystemFile::OM_NONE, false);
			BSSystemFile::ErrorCode eError = Opened.DoGetInfo(&FileInfo);
			Opened.uiFlags = (Opened.uiFlags & 0xa0000000) | eError;
			eResult = TranslateErrorCode(eError);
		}
		else
		{
			BSSystemFile::ErrorCode eError = File.DoGetInfo(&FileInfo);
			File.uiFlags = (File.uiFlags & 0xa0000000) | eError;
			eResult = TranslateErrorCode(eError);
		}
		if (eResult == EC_NONE)
			arInfo = {FileInfo.ModifyTime, FileInfo.CreateTime, FileInfo.uiFileSize};
		return eResult;
	}

	LooseFileStream::LooseFileStream(const BSFixedString& arPrefix, const BSFixedString& arDirectory,
		const BSFixedString& arFile, uint64_t auiTotalSize, bool abWritable, Location* apLocation) :
		LooseFileStreamBase(arPrefix, arDirectory, arFile), Stream(auiTotalSize, abWritable),
		pLocation(apLocation), uiFilePos(0), pBuffer(nullptr) {}

	LooseFileStream::~LooseFileStream() { DestroyBuffer(*this); }

	LooseFileStream::LooseFileStream(const LooseFileStream& arStream) :
		LooseFileStreamBase(arStream.Prefix, arStream.DirName, arStream.FileName),
		Stream(arStream.uiTotalSize, (arStream.uiFlags & 1) != 0),
		pLocation(arStream.pLocation), uiFilePos(0), pBuffer(nullptr) {}

	ErrorCode LooseFileStream::DoOpen()
	{
		BSSystemFile::ErrorCode eError = OpenFile((uiFlags & 1) != 0, false);
		uiFilePos = 0;
		if (eError == BSSystemFile::EC_NONE)
		{
			uint64_t uiSize = 0;
			eError = File.DoGetSize(&uiSize);
			File.uiFlags = (File.uiFlags & 0xa0000000) | eError;
			if (eError == BSSystemFile::EC_NONE)
			{
				uiTotalSize = static_cast<unsigned int>(uiSize);
				if (uiFlags & 2)
				{
					AutoMemContext Context(static_cast<MEM_CONTEXT>(10));
					unsigned int uiBufferSize = 0;
					StreamBuffer<LooseFileSBTraits>* pStorage = nullptr;
					if (uiFlags & 4)
					{
						uiBufferSize = static_cast<unsigned int>(uiTotalSize);
						pStorage = static_cast<StreamBuffer<LooseFileSBTraits>*>(ReadBuffer::Allocate(uiBufferSize + 64));
					}
					if (!pStorage)
					{
						AutoMemContext AllocationContext(static_cast<MEM_CONTEXT>(10));
						uiBufferSize = pLocation->DoQBufferHint();
						pStorage = static_cast<StreamBuffer<LooseFileSBTraits>*>(MemoryManager::Instance().Allocate(uint64_t(uiBufferSize) + 64, 0, false));
					}
					pBuffer = pStorage;
					pStorage->PathID = 0;
					pStorage->rStream = &File;
					pStorage->uiStartPosInStream = 0;
					pStorage->uiEndPosInStream = 0;
					pStorage->uiPosInStream = 0;
					pStorage->uiBufferSize = uiBufferSize;
					pStorage->pBuffer = reinterpret_cast<char*>(pStorage) + 64;
					pStorage->uiFlags = 2 * (uiFlags & 1);
				}
			}
		}
		return TranslateErrorCode(eError);
	}

	void LooseFileStream::DestroyBuffer(LooseFileStream& arStream)
	{
		if (!arStream.pBuffer)
			return;
		AutoMemContext Context(static_cast<MEM_CONTEXT>(10));
		auto* pBuffer = arStream.pBuffer;
		if ((pBuffer->uiFlags & 2) && pBuffer->uiStartPosInStream < pBuffer->uiPosInStream &&
			pBuffer->uiPosInStream <= pBuffer->uiEndPosInStream)
		{
			uint64_t uiWritten = 0;
			BSSystemFile* pFile = pBuffer->rStream;
			BSSystemFile::ErrorCode eError = pFile->DoWrite(pBuffer->pBuffer,
				static_cast<unsigned int>(pBuffer->uiPosInStream - pBuffer->uiStartPosInStream), &uiWritten);
			pFile->uiFlags = (pFile->uiFlags & 0xa0000000) | eError;
		}
		if (ReadBuffer::Contains(arStream.pBuffer))
			ReadBuffer::Deallocate(arStream.pBuffer);
		else
			MemoryManager::Instance().Deallocate(arStream.pBuffer, false);
		arStream.pBuffer = nullptr;
	}

	void LooseFileStream::DoClose()
	{
		DestroyBuffer(*this);
		BSSystemFile Empty;
		File = std::move(Empty);
	}

	ErrorCode LooseFileStream::DoGetInfo(Info& arInfo) { return GetFileInfo(arInfo); }

	void LooseFileStream::DoClone(BSTSmartPointer<Stream>& arStream) const
	{
		AutoMemContext Context(static_cast<MEM_CONTEXT>(8));
		void* pStorage = MemoryManager::Instance().Allocate(96, 0, false);
		LooseFileStream* pStream = pStorage ? new (pStorage) LooseFileStream(*this) : nullptr;
		arStream = pStream;
	}

	ErrorCode LooseFileStream::DoRead(void* apBuffer, uint64_t auiBytes, uint64_t& arRead) const
	{
		ErrorCode eResult;
		if (pBuffer)
			eResult = pBuffer->ReadAt(apBuffer, uiFilePos, auiBytes, arRead);
		else if (File.uiFlags & 0x80000000)
		{
			LooseFileSBTraits::AsyncFunctor Functor;
			Functor.uiStatus = 0;
			Functor.eError = BSSystemFile::EC_NONE;
			Functor.uiTransferred = 0;
			BSSystemFile::ErrorCode eError = File.DoReadAsync(apBuffer, static_cast<unsigned int>((auiBytes + 4095) & ~uint64_t(4095)), static_cast<int64_t>(uiFilePos), &Functor);
			File.uiFlags = (File.uiFlags & 0xa0000000) | eError;
			WaitCompletion(Functor, true);
			arRead = Functor.uiTransferred;
			eResult = TranslateErrorCode(Functor.eError);
		}
		else
		{
			BSSystemFile::ErrorCode eError = File.DoRead(apBuffer, static_cast<unsigned int>(auiBytes), &arRead);
			File.uiFlags = (File.uiFlags & 0xa0000000) | eError;
			eResult = TranslateErrorCode(File.GetErrorCode());
		}
		uiFilePos += arRead;
		if (uiFilePos > static_cast<unsigned int>(uiTotalSize))
			uiTotalSize = static_cast<unsigned int>(uiFilePos);
		return eResult;
	}

	ErrorCode LooseFileStream::DoWrite(const void* apBuffer, uint64_t auiBytes, uint64_t& arWritten) const
	{
		ErrorCode eResult;
		if (pBuffer)
			eResult = pBuffer->WriteAt(apBuffer, uiFilePos, auiBytes, arWritten);
		else
		{
			BSSystemFile::ErrorCode eError = File.DoWrite(apBuffer, static_cast<unsigned int>(auiBytes), &arWritten);
			File.uiFlags = (File.uiFlags & 0xa0000000) | eError;
			eResult = TranslateErrorCode(File.GetErrorCode());
		}
		uiFilePos += arWritten;
		if (uiFilePos > static_cast<unsigned int>(uiTotalSize))
			uiTotalSize = static_cast<unsigned int>(uiFilePos);
		return eResult;
	}

	ErrorCode LooseFileStream::DoSeek(int64_t aiOffset, SeekMode aeMode, uint64_t& arPosition) const
	{
		ErrorCode eResult = EC_NONE;
		if (pBuffer || (File.uiFlags & 0x80000000))
		{
			uint64_t uiPosition = uiFilePos;
			uint64_t uiSize = static_cast<unsigned int>(uiTotalSize);
			switch (aeMode)
			{
			case SM_SET: uiPosition = static_cast<uint64_t>(aiOffset); break;
			case SM_CUR: uiPosition += static_cast<uint64_t>(aiOffset); break;
			case SM_END: uiPosition = uiSize + static_cast<uint64_t>(aiOffset); break;
			default: break;
			}
			if (static_cast<int64_t>(uiPosition) < 0)
				uiPosition = 0;
			else if (uiPosition >= uiSize)
				uiPosition = uiSize;
			uiFilePos = uiPosition;
		}
		else
		{
			BSSystemFile::SeekMode eMode = aeMode == SM_SET ? BSSystemFile::SM_SET :
				aeMode == SM_END ? BSSystemFile::SM_END : BSSystemFile::SM_CUR;
			BSSystemFile::ErrorCode eError = File.DoSeek(aiOffset, eMode, reinterpret_cast<int64_t*>(&uiFilePos));
			File.uiFlags = (File.uiFlags & 0xa0000000) | eError;
			eResult = TranslateErrorCode(File.GetErrorCode());
		}
		arPosition = uiFilePos;
		if (uiFilePos > static_cast<unsigned int>(uiTotalSize))
			uiTotalSize = static_cast<unsigned int>(uiFilePos);
		return eResult;
	}

	ErrorCode LooseFileStream::DoSetEndOfStream()
	{
		BSSystemFile::ErrorCode eError = File.DoSetEndOfFile();
		File.uiFlags = (File.uiFlags & 0xa0000000) | eError;
		if (eError == BSSystemFile::EC_NONE)
			uiTotalSize = static_cast<unsigned int>(uiFilePos);
		return TranslateErrorCode(eError);
	}

	bool LooseFileStream::DoGetName(BSFixedString& arName) const
	{
		const char* pDirectory = DirName.pString;
		if (!pDirectory || !FileName.pString)
			return false;
		char Buffer[260];
		strcpy_s(Buffer, 260, pDirectory);
		strcat_s(Buffer, 260, FileName.pString);
		arName << Buffer;
		return true;
	}

	ErrorCode LooseFileStream::DoCreateAsync(BSTSmartPointer<AsyncStream>& arStream) const
	{
		Location* pResult;
		return static_cast<LooseFileLocation*>(pLocation)->CreateAsyncStreamFromNames(DirName.pString, FileName.pString, arStream, pResult, (uiFlags & 1) != 0);
	}

	LooseFileAsyncBase::LooseFileAsyncBase()
	{
		Initialize(this);
	}

	void LooseFileAsyncBase::Initialize(LooseFileAsyncBase* apBase)
	{
		new (&apBase->Functor) FunctorType;
		apBase->Functor.uiStatus = 0;
		apBase->Functor.uiTransferred = 0;
		apBase->pCurrentBuffer = nullptr;
	}

	LooseFileAsyncBase::~LooseFileAsyncBase() { Functor.~FunctorType(); }

	void LooseFileAsyncBase::FunctorType::Process(BSSystemFile::ErrorCode, uint64_t auiTransferred)
	{
		uiTransferred = auiTransferred;
	}

	void LooseFileSBTraits::AsyncFunctor::Process(BSSystemFile::ErrorCode aeError, uint64_t auiTransferred)
	{
		eError = aeError;
		uiTransferred = auiTransferred;
	}

	LooseFileAsyncStream::LooseFileAsyncStream(const BSFixedString& arPrefix, const BSFixedString& arDirectory,
		const BSFixedString& arFile, uint64_t auiTotalSize, bool abWritable, Location* apLocation) :
		LooseFileStreamBase(arPrefix, arDirectory, arFile),
		AsyncStream(auiTotalSize, abWritable, (LooseFileAsyncBase::Initialize(this), apLocation)),
		LooseFileAsyncBase(Initialized{}),
		uiOpenCount(0) {}

	LooseFileAsyncStream::~LooseFileAsyncStream() = default;

	ErrorCode LooseFileAsyncStream::DoOpen()
	{
		if (InterlockedIncrement(reinterpret_cast<volatile LONG*>(&uiOpenCount)) != 1)
			return EC_NONE;
		return TranslateErrorCode(OpenFile((uiFlags & 1) != 0, true));
	}

	void LooseFileAsyncStream::DoClose()
	{
		if (InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(&uiOpenCount), -1) == 1)
		{
			BSSystemFile Empty;
			File = std::move(Empty);
		}
	}

	ErrorCode LooseFileAsyncStream::DoGetInfo(Info& arInfo) { return GetFileInfo(arInfo); }

	void LooseFileAsyncStream::DoClone(BSTSmartPointer<AsyncStream>& arStream) const
	{
		AutoMemContext Context(static_cast<MEM_CONTEXT>(8));
		void* pStorage = MemoryManager::Instance().Allocate(104, 0, false);
		arStream = pStorage ? new (pStorage) LooseFileAsyncChild(const_cast<LooseFileAsyncStream*>(this)) : nullptr;
	}

	ErrorCode LooseFileAsyncStream::DoStartRead(void* apBuffer, uint64_t auiBytes, uint64_t auiOffset) const
	{
		BSSystemFile::ErrorCode eError = File.DoReadAsync(apBuffer, static_cast<unsigned int>(auiBytes), static_cast<int64_t>(auiOffset), &Functor);
		File.uiFlags = (File.uiFlags & 0xa0000000) | eError;
		return TranslateErrorCode(eError);
	}

	ErrorCode LooseFileAsyncStream::DoStartWrite(const void* apBuffer, uint64_t auiBytes, uint64_t auiOffset) const
	{
		BSSystemFile::ErrorCode eError = File.DoWriteAsync(apBuffer, static_cast<unsigned int>(auiBytes), static_cast<int64_t>(auiOffset), &Functor);
		File.uiFlags = (File.uiFlags & 0xa0000000) | eError;
		return TranslateErrorCode(eError);
	}

	ErrorCode LooseFileAsyncBase::StartPacketAlignedBufferedRead(AsyncStream::PacketAlignedBuffer* apBuffer,
		uint64_t auiBytes, uint64_t auiOffset, BSSystemFile& arFile, unsigned int auiPacketSize) const
	{
		apBuffer->uiDataRequestSize = static_cast<unsigned int>(auiBytes);
		apBuffer->uiDataSize = 0;
		uint64_t uiPrefix = auiOffset & (static_cast<uint64_t>(auiPacketSize) - 1);
		uint64_t uiOffset = auiOffset - uiPrefix;
		apBuffer->uiResultOffset = uiOffset;
		uint64_t uiReadSize = (uint64_t(0) - auiPacketSize) & (auiBytes + uiPrefix + auiPacketSize - 1);
		if (apBuffer->uiBufferSize < uiReadSize)
		{
			pCurrentBuffer = nullptr;
			return EC_MEMORY_ERROR;
		}
		void* pPacketBuffer = apBuffer->pPacketBuffer;
		apBuffer->pDataStart = static_cast<char*>(pPacketBuffer) + uiPrefix;
		BSSystemFile::ErrorCode eError = arFile.DoReadAsync(pPacketBuffer, static_cast<unsigned int>(uiReadSize), static_cast<int64_t>(uiOffset), &Functor);
		arFile.uiFlags = (arFile.uiFlags & 0xa0000000) | eError;
		pCurrentBuffer = apBuffer;
		return TranslateErrorCode(eError);
	}

	ErrorCode LooseFileAsyncStream::DoStartPacketAlignedBufferedRead(PacketAlignedBuffer* apBuffer, uint64_t auiBytes, uint64_t auiOffset) const
	{
		return StartPacketAlignedBufferedRead(apBuffer, auiBytes, auiOffset, File, uiMinPacketSize);
	}

	ErrorCode LooseFileAsyncStream::DoWait(uint64_t& arTransferred, bool abWait) { return Wait(arTransferred, abWait); }

	ErrorCode LooseFileAsyncStream::DoTruncate(uint64_t auiSize) const
	{
		BSSystemFile::ErrorCode eError = TruncateFile(File, auiSize);
		File.uiFlags = (File.uiFlags & 0xa0000000) | eError;
		return TranslateErrorCode(eError);
	}

	ErrorCode LooseFileAsyncBase::Wait(uint64_t& arTransferred, bool abWait)
	{
		bool bComplete = WaitCompletion(Functor, abWait);
		uint64_t uiTransferred = bComplete ? Functor.uiTransferred : 0;
		arTransferred = uiTransferred;
		if (!bComplete)
			return EC_BUSY;
		AsyncStream::PacketAlignedBuffer* pBuffer = pCurrentBuffer;
		if (pBuffer && uiTransferred)
		{
			uint64_t uiPrefix = reinterpret_cast<uintptr_t>(pBuffer->pDataStart) - reinterpret_cast<uintptr_t>(pBuffer->pPacketBuffer);
			if (uiTransferred > uiPrefix)
			{
				unsigned int uiDataSize = static_cast<unsigned int>(uiTransferred - uiPrefix);
				if (uiDataSize > pBuffer->uiDataRequestSize)
					uiDataSize = pBuffer->uiDataRequestSize;
				pBuffer->uiDataSize = uiDataSize;
				pCurrentBuffer->uiResultOffset += arTransferred;
			}
			pCurrentBuffer = nullptr;
		}
		return EC_NONE;
	}

	LooseFileAsyncChild::LooseFileAsyncChild(LooseFileAsyncStream* apSource) :
		AsyncStream(apSource->uiTotalSize, (apSource->uiFlags & 1) != 0, apSource->uiMinPacketSize), spSource(apSource) {}

	LooseFileAsyncChild::LooseFileAsyncChild(const LooseFileAsyncChild& arChild) :
		AsyncStream(arChild.uiTotalSize, (arChild.uiFlags & 1) != 0, arChild.uiMinPacketSize), spSource(arChild.spSource) {}

	LooseFileAsyncChild::~LooseFileAsyncChild() = default;

	ErrorCode LooseFileAsyncChild::DoOpen()
	{
		AsyncStream* pSource = spSource.get();
		LONG iOld;
		do { iOld = pSource->uiFlags; }
		while (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&pSource->uiFlags), iOld | 8, iOld) != iOld);
		return pSource->DoOpen();
	}

	void LooseFileAsyncChild::DoClose()
	{
		AsyncStream* pSource = spSource.get();
		LONG iOld;
		do { iOld = pSource->uiFlags; }
		while (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&pSource->uiFlags), iOld & 0xfffffff1, iOld) != iOld);
		pSource->DoClose();
	}

	ErrorCode LooseFileAsyncChild::DoGetInfo(Info& arInfo) { return spSource->DoGetInfo(arInfo); }

	void LooseFileAsyncChild::DoClone(BSTSmartPointer<AsyncStream>& arStream) const
	{
		AutoMemContext Context(static_cast<MEM_CONTEXT>(8));
		void* pStorage = MemoryManager::Instance().Allocate(104, 0, false);
		arStream = pStorage ? new (pStorage) LooseFileAsyncChild(*this) : nullptr;
	}

	ErrorCode LooseFileAsyncChild::DoStartRead(void* apBuffer, uint64_t auiBytes, uint64_t auiOffset) const
	{
		BSSystemFile* pFile = &spSource->File;
		BSSystemFile::ErrorCode eError = pFile->DoReadAsync(apBuffer, static_cast<unsigned int>(auiBytes), static_cast<int64_t>(auiOffset), &Functor);
		pFile->uiFlags = (pFile->uiFlags & 0xa0000000) | eError;
		return TranslateErrorCode(eError);
	}

	ErrorCode LooseFileAsyncChild::DoStartWrite(const void* apBuffer, uint64_t auiBytes, uint64_t auiOffset) const
	{
		BSSystemFile* pFile = &spSource->File;
		BSSystemFile::ErrorCode eError = pFile->DoWriteAsync(apBuffer, static_cast<unsigned int>(auiBytes), static_cast<int64_t>(auiOffset), &Functor);
		pFile->uiFlags = (pFile->uiFlags & 0xa0000000) | eError;
		return TranslateErrorCode(eError);
	}

	ErrorCode LooseFileAsyncChild::DoStartPacketAlignedBufferedRead(PacketAlignedBuffer* apBuffer, uint64_t auiBytes, uint64_t auiOffset) const
	{
		return StartPacketAlignedBufferedRead(apBuffer, auiBytes, auiOffset, spSource->File, uiMinPacketSize);
	}

	ErrorCode LooseFileAsyncChild::DoWait(uint64_t& arTransferred, bool abWait) { return Wait(arTransferred, abWait); }

	ErrorCode LooseFileAsyncChild::DoTruncate(uint64_t auiSize) const
	{
		BSSystemFile* pFile = &spSource->File;
		BSSystemFile::ErrorCode eError = TruncateFile(*pFile, auiSize);
		pFile->uiFlags = (pFile->uiFlags & 0xa0000000) | eError;
		return TranslateErrorCode(eError);
	}
}
