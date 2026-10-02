#include "BSResource/BSResourceCacheDrive.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/BSTArrayAlg.h"
#include "BSCore/BSAutoLock.h"
#include "BSResource/BSResource.h"
#include "BSSystem/BSSystemDir.h"

#include <cstring>

#include <new>

template <>
BSTEventSource<BSResource::CacheEvent>::~BSTEventSource() {}

namespace
{
	template <class T>
	unsigned int FindCacheSink(const BSTArray<T>& arArray, T apSink)
	{
		for (unsigned int ui = 0; ui < arArray.QSize(); ++ui)
			if (arArray[ui] == apSink)
				return ui;
		return 0xFFFFFFFF;
	}
}

template <>
void BSTEventSource<BSResource::CacheEvent>::Notify(const BSResource::CacheEvent& arEvent)
{
	BSAutoLock<BSSpinLock> kAutoLock(mLock);
	const unsigned char cPrevious = cNotifying;
	cNotifying = 1;
	if (!cPrevious && pPendingRegistersA.QSize())
	{
		for (auto pIt = pPendingRegistersA.Begin(), pEnd = pPendingRegistersA.End(); pIt != pEnd; ++pIt)
		{
			const auto& pSink = *pIt;
			if (FindCacheSink(pSinksA, pSink) == 0xFFFFFFFF)
				pSinksA.Add(pSink);
		}
		pPendingRegistersA.Clear(false);
	}
	BSEvent::NotifyControl eControl = BSEvent::kContinue;
	for (auto pIt = pSinksA.Begin(), pEnd = pSinksA.End(); pIt != pEnd; ++pIt)
	{
		if (eControl != BSEvent::kContinue)
			break;
		auto* pSink = *pIt;
		if (FindCacheSink(pPendingUnregistersA, pSink) == 0xFFFFFFFF)
			eControl = pSink->ProcessEvent(arEvent, this);
	}
	cNotifying = cPrevious;
	if (!cPrevious && pPendingUnregistersA.QSize())
	{
		for (auto pIt = pPendingUnregistersA.Begin(), pEnd = pPendingUnregistersA.End(); pIt != pEnd; ++pIt)
		{
			const unsigned int uiIndex = FindCacheSink(pSinksA, *pIt);
			if (uiIndex != 0xFFFFFFFF)
				pSinksA.RemoveFast(uiIndex);
		}
		pPendingUnregistersA.Clear(false);
	}
}

namespace BSResource
{
	inline BSTEventSource<CacheEvent> CacheEventSource;
	BSTEventSource<CacheEvent>* QCacheEventSource() { return &CacheEventSource; }
	unsigned int CachePauseEvent::uiCount = 0;
	unsigned int PauseBlockMutex::uiCount = 0;

	CachePauseEvent::CachePauseEvent()
	{
		MyEvent = uiCount ? OpenEventA(0, FALSE, "CachePause") : CreateEventA(nullptr, FALSE, FALSE, "CachePause");
		++uiCount;
	}
	CachePauseEvent::~CachePauseEvent() { --uiCount; CloseHandle(MyEvent); }
	void CachePauseEvent::Signal() const { SetEvent(MyEvent); }
	void CachePauseEvent::Wait() const { WaitForSingleObject(MyEvent, INFINITE); }
	PauseBlockMutex::PauseBlockMutex()
	{
		MyMutex = uiCount ? OpenMutexA(0, FALSE, "CacheBlock") : CreateMutexA(nullptr, FALSE, "CacheBlock");
		++uiCount;
	}
	PauseBlockMutex::~PauseBlockMutex() { --uiCount; CloseHandle(MyMutex); }
	void PauseBlockMutex::Release() { ReleaseMutex(MyMutex); }
	void PauseBlockMutex::Wait() { WaitForSingleObject(MyMutex, INFINITE); }

	CacheDrive::CacheDrive(bool abInitialize) : uiMinimumAsyncPacketSize(65536), pPartitions(nullptr),
		uiNumPartitions(0), bAsyncSupported(false), bAvailable(SetupDrive(abInitialize)) {}
	CacheDrive::CacheDrive(unsigned int auiPacketSize, bool abAsyncSupported, bool abInitialize) :
		uiMinimumAsyncPacketSize(auiPacketSize), pPartitions(nullptr), uiNumPartitions(0),
		bAsyncSupported(abAsyncSupported), bAvailable(SetupDrive(abInitialize)) {}
	CacheDrive::~CacheDrive() = default;


	bool CacheDrive::RegisterTask(const BSTSmartPointer<Task>& arTask, unsigned int auiPriority)
	{
		if (spImpl.pPtr)
			WaitForSingleObject(spImpl->ExitSema.hSemaphore, INFINITE);
		if (bAvailable)
		{
			void* pStorage = MemoryManager::Instance().Allocate(sizeof(Impl), 0, false);
			Impl* pImpl = pStorage ? new (pStorage) Impl(*this, arTask, auiPriority) : nullptr;
			spImpl = pImpl;
			pImpl = spImpl.pPtr;
			pImpl->Initialize(BSThread::SS_16K, "BSResource::CacheDrive thread");
			unsigned int uiPriority = pImpl->uiInitialTargetPriority;
			if (uiPriority != ~0u)
				pImpl->spTask->OnBeginInitial(uiPriority, &pImpl->uiCurrentPriority);
		}
		return bAvailable;
	}

	void CacheDrive::Pause(bool abBlocking)
	{
		if (!spImpl.pPtr)
			return;
		if (spImpl->uiState == 4)
		{
			spImpl = nullptr;
			return;
		}
		Impl* pImpl = spImpl.pPtr;
		if (GetCurrentThreadId() == pImpl->m_ThreadID)
			return;
		unsigned int uiFlags;
		do { uiFlags = pImpl->uiPauseFlags; }
		while (static_cast<unsigned int>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&pImpl->uiPauseFlags), uiFlags | 1, uiFlags)) != uiFlags);
		if (abBlocking && !pImpl->bBlocked)
		{
			HANDLE hMutex = pImpl->BlockingMutex.MyMutex;
			pImpl->bBlocked = true;
			WaitForSingleObject(hMutex, INFINITE);
		}
	}

	void CacheDrive::Resume()
	{
		if (spImpl.pPtr)
		{
			if (spImpl->uiState == 4)
				spImpl = nullptr;
			else
				spImpl->Resume();
		}
	}

	void CacheDrive::Exit()
	{
		if (!spImpl.pPtr)
			return;
		if (spImpl->uiState == 4)
			spImpl = nullptr;
		else
		{
			Impl* pImpl = spImpl.pPtr;
			pImpl->Resume();
			if (pImpl->spTask.pPtr)
				pImpl->spTask->bCancel = true;
			WaitForSingleObject(pImpl->ExitSema.hSemaphore, INFINITE);
			pImpl->Close();
		}
	}

	unsigned int CacheDrive::QNumPartitions() const { return uiNumPartitions; }

	ErrorCode CacheDrive::DoCreateStream(const char* apPath, BSTSmartPointer<Stream>& arStream, Location*& arLocation, bool abWritable)
	{
		unsigned int uiCount = uiNumPartitions;
		LooseFileLocation* pPartition = pPartitions;
		ErrorCode eError = EC_NOT_EXIST;
		while (uiCount-- && eError != EC_NONE)
		{
			eError = apPath ? pPartition->DoCreateStream(apPath, arStream, arLocation, abWritable) : EC_INVALID_PATH;
			++pPartition;
		}
		return eError;
	}

	ErrorCode CacheDrive::DoCreateAsyncStream(const char* apPath, BSTSmartPointer<AsyncStream>& arStream, Location*& arLocation, bool abWritable)
	{
		unsigned int uiCount = uiNumPartitions;
		LooseFileLocation* pPartition = pPartitions;
		ErrorCode eError = EC_NOT_EXIST;
		while (uiCount-- && eError != EC_NONE)
		{
			eError = apPath ? pPartition->DoCreateAsyncStream(apPath, arStream, arLocation, abWritable) : EC_INVALID_PATH;
			++pPartition;
		}
		return eError;
	}

	ErrorCode CacheDrive::DoTraversePrefix(const char* apPath, LocationTraverser& arTraverser)
	{
		unsigned int uiCount = uiNumPartitions;
		LooseFileLocation* pPartition = pPartitions;
		while (uiCount--)
			(pPartition++)->DoTraversePrefix(apPath, arTraverser);
		return EC_NONE;
	}

	ErrorCode CacheDrive::DoGetInfo(const char* apPath, Info& arInfo)
	{
		unsigned int uiCount = uiNumPartitions;
		LooseFileLocation* pPartition = pPartitions;
		ErrorCode eError = EC_NOT_EXIST;
		while (uiCount-- && eError != EC_NONE)
			eError = (pPartition++)->DoGetInfo(apPath, arInfo);
		return eError;
	}

	ErrorCode CacheDrive::DoGetInfo(const char* apPath, Info& arInfo, Location*& arLocation)
	{
		unsigned int uiCount = uiNumPartitions;
		LooseFileLocation* pPartition = pPartitions;
		ErrorCode eError = EC_NOT_EXIST;
		while (uiCount-- && eError != EC_NONE)
		{
			eError = pPartition->DoGetInfo(apPath, arInfo);
			if (eError == EC_NONE)
				arLocation = pPartition;
			++pPartition;
		}
		return eError;
	}

	ErrorCode CacheDrive::DoDelete(const char* apPath)
	{
		unsigned int uiCount = uiNumPartitions;
		LooseFileLocation* pPartition = pPartitions;
		ErrorCode eError = EC_NOT_EXIST;
		while (uiCount-- && eError != EC_NONE)
			eError = (pPartition++)->DoDelete(apPath);
		return eError;
	}
	unsigned int CacheDrive::DoGetMinimumAsyncPacketSize() const { return uiMinimumAsyncPacketSize; }

	ErrorCode CacheDrive::CreateStreamInternal(const char* apPath, BSTSmartPointer<Stream>& arStream, LooseFileLocation*& arLocation)
	{
		unsigned int uiCount = uiNumPartitions;
		LooseFileLocation* pPartition = pPartitions;
		ErrorCode eError = EC_NOT_EXIST;
		Location* pLocation;
		while (uiCount-- && eError != EC_NONE)
		{
			eError = apPath ? pPartition->DoCreateStream(apPath, arStream, pLocation, false) : EC_INVALID_PATH;
			if (eError == EC_NONE)
				arLocation = pPartition;
			++pPartition;
		}
		return eError;
	}

	CacheDrive::Op::Op(const BSFixedString& arPath, Location& arSource) : uiRefCount(0), uiFileSize(0),
		Path(arPath), pSourceLocation(&arSource), pDestLocation(nullptr), uiPriority(~0u)
	{
		Location* pLocation = nullptr;
		if (!arPath.pString || arSource.DoCreateAsyncStream(arPath.pString, spAsyncSource, pLocation, false) != EC_NONE)
		{
			pLocation = nullptr;
			if (arPath.pString)
				arSource.DoCreateStream(arPath.pString, spSource, pLocation, false);
		}
	}
	CacheDrive::Op::~Op() = default;
	void CacheDrive::Op::OnValid(CacheDrive&) {}
	void CacheDrive::Op::OnInit(CacheDrive&) {}
	void CacheDrive::Op::OnPacketWrite(CacheDrive&, uint64_t) {}
	void CacheDrive::Op::OnComplete(CacheDrive&) {}
	void CacheDrive::Op::OnError(CacheDrive&) {}

	CacheDrive::Task::Task(const char* apCleanFiles) : uiRefCount(0), pCleanFilesList(apCleanFiles),
		uiMaximumPacketAlign(0), uiMaximumBufferSize(0), bCancel(false), bInitialCachingCompleted(false) {}
	CacheDrive::Task::~Task() = default;
	void CacheDrive::Task::OnBeginInitial(unsigned int, volatile unsigned int*)
	{
		while (!bInitialCachingCompleted)
			Sleep(20);
	}
	void CacheDrive::Task::OnAccumulateTotalsComplete() {}
	void CacheDrive::Task::OnValidateFilesComplete(uint64_t) {}
	void CacheDrive::Task::OnValidateSpaceComplete() {}
	void CacheDrive::Task::OnUpdateTotalProcessed(uint64_t) {}
	void CacheDrive::Task::OnBeginCaching() {}
	void CacheDrive::Task::OnInitialCachingComplete() {}
	void CacheDrive::Task::InitialCachingComplete()
	{
		if (!bInitialCachingCompleted)
		{
			OnInitialCachingComplete();
			bInitialCachingCompleted = true;
		}
	}

	void CacheDrive::Task::AddOp(const BSTSmartPointer<Op>& arOp, unsigned int auiPriority)
	{
		arOp->uiPriority = auiPriority;
		Ops.Add(arOp);
	}

	void CacheDrive::Task::SortOps() { QuickSort(Ops, OpSortFunctor{}); }

	void CacheDrive::Task::AccumulateTotals(uint64_t& arTotal)
	{
		SortOps();
		Info FileInfo = {};
		BSTSmartPointer<Op>* pCurrent = Ops.iSize ? Ops.QBuffer() : nullptr;
		arTotal = 0;
		while (pCurrent != (Ops.iSize ? Ops.QBuffer() + Ops.iSize : nullptr))
		{
			StreamBase* pStream = pCurrent->pPtr->spAsyncSource.pPtr;
			if (!pStream)
				pStream = pCurrent->pPtr->spSource.pPtr;
			if (pStream && pStream->DoGetInfo(FileInfo) == EC_NONE)
			{
				pCurrent->pPtr->uiFileSize = FileInfo.uiFileSize;
				arTotal += FileInfo.uiFileSize;
			}
			++pCurrent;
		}
		OnAccumulateTotalsComplete();
	}

	LooseFileLocation* CacheDrive::QPartition(unsigned int auiIndex) const
	{
		return auiIndex < uiNumPartitions ? pPartitions + auiIndex : nullptr;
	}

	unsigned int CacheDrive::GetPartitionInfo(LooseFileLocation** apPartitions, unsigned int auiMax)
	{
		unsigned int uiCount = uiNumPartitions < auiMax ? uiNumPartitions : auiMax;
		LooseFileLocation* pCurrent = pPartitions;
		for (unsigned int ui = 0; ui < uiCount; ++ui)
			apPartitions[ui] = pCurrent++;
		return uiCount;
	}

	void CleanFilesFunctor::operator()(const char* apName) const
	{
		unsigned int uiCount = rCacheDrive->uiNumPartitions;
		for (unsigned int ui = 0; ui < uiCount; ++ui)
			rCacheDrive->QPartition(ui)->DoDelete(apName);
	}

	void CacheDrive::Task::CleanFilesFromDrive(ValidateContext& arContext)
	{
		if (pCleanFilesList)
		{
			CleanFilesFunctor Functor{arContext.rCacheDrive};
			TraverseNameTextList(pCleanFilesList, Functor);
		}
	}

	bool CacheDrive::Task::ValidateSpace(ValidateContext& arContext)
	{
		auto Current = Ops.Begin();
		while (Current != Ops.End())
		{
			uint64_t uiSmallest = ~uint64_t(0);
			uint64_t uiSize = Current.pCurrent->pPtr->uiFileSize;
			unsigned int uiPartition = ~0u;
			for (unsigned int ui = 0; ui < arContext.uiNumPartitions; ++ui)
			{
				uint64_t uiFree = arContext.uiPartitionFreeSpace[ui];
				if (uiFree >= uiSize && uiFree < uiSmallest)
				{
					uiSmallest = uiFree;
					uiPartition = ui;
				}
			}
			LooseFileLocation* pDest = nullptr;
			if (uiPartition != ~0u)
			{
				pDest = arContext.pPartitionsA[uiPartition];
				arContext.uiPartitionFreeSpace[uiPartition] -= uiSize;
			}
			if (pDest)
			{
				Location* pSource = Current.pCurrent->pPtr->pSourceLocation;
				if (pSource->DoGetMinimumAsyncPacketSize() > uiMaximumPacketAlign)
					uiMaximumPacketAlign = pSource->DoGetMinimumAsyncPacketSize();
				if (pSource->DoQBufferHint() > uiMaximumBufferSize)
					uiMaximumBufferSize = pSource->DoQBufferHint();
				Current.pCurrent->pPtr->pDestLocation = pDest;
				++Current;
			}
			else
			{
				arContext.uiTotalSize -= Current.pCurrent->pPtr->uiFileSize;
				Current = Ops.RemoveAt(Current);
			}
		}
		OnValidateSpaceComplete();
		return Ops.iSize != 0;
	}

	bool CacheDrive::Task::ValidateFiles(ValidateContext& arContext)
	{
		Info SourceInfo = {}, DestInfo = {};
		auto Current = Ops.Begin();
		arContext.bFullyCached = false;
		uint64_t uiInitialSize = 0;
		bool bFoundCached = false;
		while (Current != Ops.End())
		{
			StreamBase* pSource = Current.pCurrent->pPtr->spAsyncSource.pPtr;
			if (!pSource)
				pSource = Current.pCurrent->pPtr->spSource.pPtr;
			if (!pSource || pSource->DoGetInfo(SourceInfo) != EC_NONE)
			{
				unsigned int uiCount = arContext.rCacheDrive->uiNumPartitions;
				for (unsigned int ui = 0; ui < uiCount; ++ui)
					arContext.rCacheDrive->QPartition(ui)->DoDelete(Current.pCurrent->pPtr->Path.pString);
				Current = Ops.RemoveAt(Current);
				arContext.uiTotalSize -= SourceInfo.uiFileSize;
				continue;
			}
			bool bCached = false;
			unsigned int uiCount = arContext.rCacheDrive->uiNumPartitions;
			for (unsigned int ui = 0; ui < uiCount; ++ui)
			{
				LooseFileLocation* pPartition = arContext.rCacheDrive->QPartition(ui);
				if (bCached || pPartition->DoGetInfo(Current.pCurrent->pPtr->Path.pString, DestInfo) != EC_NONE ||
					!InfoEqual(DestInfo, SourceInfo) || DestInfo.uiFileSize != SourceInfo.uiFileSize)
					pPartition->DoDelete(Current.pCurrent->pPtr->Path.pString);
				else
				{
					OnValid(*Current.pCurrent, *arContext.rCacheDrive);
					CacheEvent Event{SETTARGET, &Current.pCurrent->pPtr->Path, 100.0f, &pPartition->Prefix};
					CacheEventSource.Notify(Event);
					bCached = true;
					bFoundCached = true;
				}
			}
			if (bCached)
			{
				Current = Ops.RemoveAt(Current);
				arContext.uiTotalSize -= SourceInfo.uiFileSize;
			}
			else
			{
				Op* pOp = Current.pCurrent->pPtr;
				if (pOp->uiPriority < arContext.uiInitialTargetPriority)
					uiInitialSize += SourceInfo.uiFileSize;
				++Current;
				pOp->uiFileSize = SourceInfo.uiFileSize;
			}
		}
		bool bRemaining = Ops.iSize != 0;
		if (!bRemaining)
		{
			if (bFoundCached)
				arContext.bFullyCached = true;
			const BSFixedString& EmptyValue = BSFixedString::QEmptyString();
			const BSFixedString& EmptyName = BSFixedString::QEmptyString();
			CacheEvent Event{ALLDONE, &EmptyName, 0.0f, &EmptyValue};
			CacheEventSource.Notify(Event);
		}
		OnValidateFilesComplete(uiInitialSize);
		return bRemaining;
	}

	void CacheDrive::Task::OnValid(const BSTSmartPointer<Op>& arOp, CacheDrive& arDrive)
	{
		BSTSmartPointer<Stream> Stream;
		Op* pOp = arOp.pPtr;
		ErrorCode eError;
		if (pOp->pDestLocation)
		{
			Location* pLocation = nullptr;
			eError = pOp->Path.pString ? pOp->pDestLocation->DoCreateStream(pOp->Path.pString, Stream, pLocation, false) : EC_INVALID_PATH;
		}
		else
		{
			const char* pPath = pOp->Path.pString;
			unsigned int uiCount = arDrive.uiNumPartitions;
			LooseFileLocation* pPartition = arDrive.pPartitions;
			eError = EC_NOT_EXIST;
			while (uiCount-- && eError != EC_NONE)
			{
				Location* pLocation;
				eError = pPath ? pPartition->DoCreateStream(pPath, Stream, pLocation, false) : EC_INVALID_PATH;
				if (eError == EC_NONE)
					pOp->pDestLocation = pPartition;
				++pPartition;
			}
		}
		if (eError == EC_NONE)
			RegisterStream(arOp->Path.pString, Stream);
		CacheEvent Event{COPIED, &arOp->Path, 100.0f, &BSFixedString::QEmptyString()};
		CacheEventSource.Notify(Event);
		arOp->OnValid(arDrive);
	}

	void CacheDrive::CacheContext::TestProgress()
	{
		if (*ruiPauseFlags & 1)
		{
			unsigned int uiFlags;
			do { uiFlags = *ruiPauseFlags; }
			while (static_cast<unsigned int>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(ruiPauseFlags), uiFlags | 2, uiFlags)) != uiFlags);
			BlockingMutex.Release();
			do { PauseEvent.Wait(); } while (*ruiPauseFlags & 1);
			BlockingMutex.Wait();
			do { uiFlags = *ruiPauseFlags; }
			while (static_cast<unsigned int>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(ruiPauseFlags), uiFlags & ~2u, uiFlags)) != uiFlags);
		}
	}

	ErrorCode StandardFunctor::Read(void* apBuffer, uint64_t auiBytes, uint64_t& arRead) const
	{
		return spSource->QWritable() ? EC_UNSUPPORTED : spSource->DoRead(apBuffer, auiBytes, arRead);
	}

	ErrorCode AsyncFunctor::Read(void* apBuffer, uint64_t auiBytes, uint64_t& arRead) const
	{
		ErrorCode eError = spSource->QWritable() ? EC_UNSUPPORTED : spSource->DoStartRead(apBuffer, auiBytes, uiStreamPos);
		arRead = 0;
		if (eError != EC_NONE)
			return eError;
		eError = spSource->DoWait(arRead, true);
		if (eError == EC_NONE)
			uiStreamPos += arRead;
		else
			arRead = 0;
		return eError;
	}

	template <class Functor>
	ErrorCode CacheDrive::Task::CopyStream(CacheContext& arContext, const BSTSmartPointer<Stream>& arDest,
		const Functor& arSource, const BSTSmartPointer<Op>& arOp)
	{
		ErrorCode eError = arSource.Open();
		if (eError != EC_NONE)
			return eError;
		uint64_t uiRemaining = arSource.uiStreamSize;
		uint64_t uiBufferSize = arContext.uiBufferSize;
		uint64_t uiRead = 0, uiWritten = 0;
		arOp->OnInit(*arContext.rCacheDrive);
		while (uiRemaining && eError == EC_NONE && !bCancel)
		{
			if (arSource.Read(arContext.pBuffer, uiBufferSize, uiRead) == EC_NONE &&
				arDest->QWritable() && arDest->DoWrite(arContext.pBuffer, uiRead, uiWritten) == EC_NONE && uiWritten == uiRead)
			{
				arOp->OnPacketWrite(*arContext.rCacheDrive, uiRead);
				uiRemaining -= uiRead;
			}
			else
			{
				arOp->OnError(*arContext.rCacheDrive);
				eError = EC_FILE_ERROR;
			}
			if (!bInitialCachingCompleted)
				OnUpdateTotalProcessed(eError == EC_NONE ? uiRead : uiRemaining);
			arContext.TestProgress();
		}
		if (eError == EC_NONE && !bCancel)
			arOp->OnComplete(*arContext.rCacheDrive);
		unsigned int uiFlags;
		do { uiFlags = arDest->uiFlags; }
		while (static_cast<unsigned int>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&arDest->uiFlags), uiFlags & ~14u, uiFlags)) != uiFlags);
		arDest->DoClose();
		arSource.Close();
		return eError;
	}

	ErrorCode CacheDrive::Task::SetDestStreamSize(const BSTSmartPointer<Stream>& arStream, uint64_t auiSize)
	{
		unsigned int uiFlags;
		do { uiFlags = arStream->uiFlags; }
		while (static_cast<unsigned int>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&arStream->uiFlags), uiFlags | 8, uiFlags)) != uiFlags);
		ErrorCode eError = arStream->DoOpen();
		if (eError == EC_NONE)
		{
			uint64_t uiPosition = 0;
			eError = arStream->DoSeek(static_cast<int64_t>(auiSize), SM_SET, uiPosition);
			if (eError == EC_NONE)
				eError = arStream->DoSetEndOfStream();
			if (eError == EC_NONE)
				eError = arStream->DoSeek(0, SM_SET, uiPosition);
			if (eError != EC_NONE)
			{
				do { uiFlags = arStream->uiFlags; }
				while (static_cast<unsigned int>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&arStream->uiFlags), uiFlags & ~14u, uiFlags)) != uiFlags);
				arStream->DoClose();
			}
		}
		return eError;
	}

	ErrorCode CacheDrive::Task::CopyStreamASync(CacheContext& arContext, const BSTSmartPointer<AsyncStream>& arDest,
		const AsyncFunctor& arSource, const BSTSmartPointer<Op>& arOp)
	{
		ErrorCode eError = arSource.Open();
		if (eError != EC_NONE)
			return eError;
		uint64_t uiRemaining = arSource.uiStreamSize;
		uint64_t uiBufferSize = uint64_t(arContext.uiBufferSize) >> 1;
		void* pBuffers[2] = {arContext.pBuffer, static_cast<char*>(arContext.pBuffer) + uiBufferSize};
		uint64_t uiRead[2];
		uint64_t uiWritten = 0, uiWritePosition = 0;
		unsigned int uiCurrent = 0;
		bool bWriting = false;
		arOp->OnInit(*arContext.rCacheDrive);
		eError = arSource.spSource->QWritable() ? EC_UNSUPPORTED : arSource.spSource->DoStartRead(pBuffers[0], uiBufferSize, arSource.uiStreamPos);
		if (eError == EC_NONE)
		{
			while (uiRemaining && eError == EC_NONE && !bCancel)
			{
				arContext.TestProgress();
				unsigned int uiSlot = uiCurrent;
				eError = arSource.spSource->DoWait(uiRead[uiSlot], true);
				if (eError != EC_NONE)
					uiRead[uiSlot] = 0;
				else
					arSource.uiStreamPos += uiRead[uiSlot];
				uiRemaining -= uiRead[uiSlot];
				if (bWriting)
				{
					arContext.TestProgress();
					eError = arDest->DoWait(uiWritten, true);
					uiWritePosition += uiRead[1 - uiCurrent];
					if (!bInitialCachingCompleted)
						OnUpdateTotalProcessed(uiWritten);
					arOp->OnPacketWrite(*arContext.rCacheDrive, uiWritten);
				}
				if (eError != EC_NONE)
				{
					eError = EC_FILE_ERROR;
					continue;
				}
				unsigned int uiNext = 1 - uiCurrent;
				if (uiRemaining && (arSource.spSource->QWritable() ||
					arSource.spSource->DoStartRead(pBuffers[uiNext], uiBufferSize, arSource.uiStreamPos) != EC_NONE))
				{
					eError = EC_FILE_ERROR;
					continue;
				}
				if (uiRead[uiSlot])
				{
					arContext.TestProgress();
					eError = arDest->QWritable() ? arDest->DoStartWrite(pBuffers[uiSlot], (uiRead[uiSlot] + 511) & ~uint64_t(511), uiWritePosition) : EC_UNSUPPORTED;
					bWriting = true;
					if (eError != EC_NONE)
						eError = EC_FILE_ERROR;
					else
						uiCurrent = uiNext;
				}
			}
			if (!bCancel)
			{
				arDest->DoWait(uiWritten, true);
				if (uiWritten)
					arOp->OnPacketWrite(*arContext.rCacheDrive, uiWritten);
				if (uiWritten != uiRead[1 - uiCurrent])
					eError = arDest->DoTruncate(uiWritten - uiRead[1 - uiCurrent]);
			}
			if (eError == EC_NONE && !bCancel)
				arOp->OnComplete(*arContext.rCacheDrive);
		}
		unsigned int uiFlags;
		do { uiFlags = arDest->uiFlags; }
		while (static_cast<unsigned int>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&arDest->uiFlags), uiFlags & ~14u, uiFlags)) != uiFlags);
		arDest->DoClose();
		arSource.Close();
		return eError;
	}

	void CacheDrive::Task::Cache(CacheContext& arContext)
	{
		auto Current = Ops.Begin();
		BSTSmartPointer<AsyncStream> AsyncDest;
		BSTSmartPointer<Stream> Dest;
		ErrorCode eError = EC_FILE_ERROR;
		if (!bInitialCachingCompleted)
			OnBeginCaching();
		while (Current != Ops.End() && !bCancel)
		{
			unsigned int uiPriority = Current.pCurrent->pPtr->uiPriority;
			*arContext.ruiCurrentPriority = uiPriority;
			if (uiPriority >= arContext.uiInitialTargetPriority)
				InitialCachingComplete();
			arContext.TestProgress();
			Op* pOp = Current.pCurrent->pPtr;
			LooseFileLocation* pDest = pOp->pDestLocation;
			char Directory[260];
			char* pEnd = Directory + pDest->Prefix.QLength();
			const char* pName = pOp->Path.pString;
			char cSeparator = static_cast<char>(QPathSeparator());
			strcpy_s(Directory, sizeof(Directory), pDest->Prefix.pString);
			while (*pName)
			{
				char cChar = *pName++;
				if (cChar == '/' || cChar == '\\')
				{
					*pEnd = 0;
					BSSystemDir::MakeDir(Directory);
					*pEnd = cSeparator;
				}
				else
					*pEnd = cChar;
				++pEnd;
			}
			pOp = Current.pCurrent->pPtr;
			Location* pLocation = nullptr;
			if (pOp->Path.pString && pOp->pDestLocation->DoCreateAsyncStream(pOp->Path.pString, AsyncDest, pLocation, true) == EC_NONE)
			{
				pOp = Current.pCurrent->pPtr;
				if (pOp->spAsyncSource.pPtr)
				{
					unsigned int uiFlags;
					do { uiFlags = AsyncDest->uiFlags; }
					while (static_cast<unsigned int>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&AsyncDest->uiFlags), uiFlags | 8, uiFlags)) != uiFlags);
					AsyncDest->DoOpen();
					uint64_t uiSize = Current.pCurrent->pPtr->uiFileSize;
					AsyncFunctor Source(Current.pCurrent->pPtr->spAsyncSource, uiSize);
					eError = CopyStreamASync(arContext, AsyncDest, Source, *Current.pCurrent);
				}
				else if (pOp->spSource.pPtr)
				{
					Location* pSyncLocation = nullptr;
					if (pOp->Path.pString && pOp->pDestLocation->DoCreateStream(pOp->Path.pString, Dest, pSyncLocation, true) == EC_NONE)
					{
						eError = SetDestStreamSize(Dest, Current.pCurrent->pPtr->uiFileSize);
						if (eError == EC_NONE)
						{
							uint64_t uiSize = Current.pCurrent->pPtr->uiFileSize;
							StandardFunctor Source(Current.pCurrent->pPtr->spSource, uiSize);
							eError = CopyStream(arContext, Dest, Source, *Current.pCurrent);
						}
					}
				}
			}
			else
			{
				pOp = Current.pCurrent->pPtr;
				Location* pSyncLocation = nullptr;
				if (pOp->Path.pString && pOp->pDestLocation->DoCreateStream(pOp->Path.pString, Dest, pSyncLocation, true) == EC_NONE)
				{
					eError = SetDestStreamSize(Dest, Current.pCurrent->pPtr->uiFileSize);
					if (Current.pCurrent->pPtr->spAsyncSource.pPtr && eError == EC_NONE)
					{
						uint64_t uiSize = Current.pCurrent->pPtr->uiFileSize;
						AsyncFunctor Source(Current.pCurrent->pPtr->spAsyncSource, uiSize);
						eError = CopyStream(arContext, Dest, Source, *Current.pCurrent);
					}
					if (Current.pCurrent->pPtr->spSource.pPtr && eError == EC_NONE)
					{
						uint64_t uiSize = Current.pCurrent->pPtr->uiFileSize;
						StandardFunctor Source(Current.pCurrent->pPtr->spSource, uiSize);
						eError = CopyStream(arContext, Dest, Source, *Current.pCurrent);
					}
				}
			}
			if (eError != EC_NONE)
				arContext.rCacheDrive->DoDelete(Current.pCurrent->pPtr->Path.pString);
			else
			{
				Info SourceInfo = {};
				StreamBase* pSource = Current.pCurrent->pPtr->spAsyncSource.pPtr;
				if (!pSource)
					pSource = Current.pCurrent->pPtr->spSource.pPtr;
				if (pSource && pSource->DoGetInfo(SourceInfo) == EC_NONE)
				{
					char Name[260];
					strcpy_s(Name, sizeof(Name), Current.pCurrent->pPtr->pDestLocation->Prefix.pString);
					strcat_s(Name, sizeof(Name), Current.pCurrent->pPtr->Path.pString);
					BSSystemFile::SetTime(Name, &SourceInfo.CreateTime,
						&SourceInfo.ModifyTime, &SourceInfo.ModifyTime);
				}
				OnValid(*Current.pCurrent, *arContext.rCacheDrive);
			}
			Op* pRelease = Current.pCurrent->pPtr;
			if (pRelease)
			{
				(*Current).pPtr = nullptr;
				if (pRelease->DecRef() == 0)
					delete pRelease;
			}
			++Current;
		}
		InitialCachingComplete();
		const BSFixedString& EmptyValue = BSFixedString::QEmptyString();
		const BSFixedString& EmptyName = BSFixedString::QEmptyString();
		CacheEvent Event{ALLDONE, &EmptyName, 0.0f, &EmptyValue};
		CacheEventSource.Notify(Event);
	}

	CacheDrive::Impl::Impl(CacheDrive& arOwner, const BSTSmartPointer<Task>& arTask, unsigned int auiPriority) :
		uiRefCount(0), rOwner(&arOwner), spTask(arTask), uiState(0), uiInitialTargetPriority(auiPriority),
		uiCurrentPriority(0), uiPauseFlags(0), bBlocked(false) {}
	CacheDrive::Impl::~Impl() = default;
	unsigned int CacheDrive::Impl::ThreadProc()
	{
		Task* pTask = spTask.pPtr;
		if (pTask)
		{
			CacheDrive* pOwner = rOwner;
			ValidateContext Validation;
			Validation.rCacheDrive = pOwner;
			Validation.uiInitialTargetPriority = uiInitialTargetPriority;
			Validation.bFullyCached = false;
			Validation.uiNumPartitions = 0;
			pTask->AccumulateTotals(Validation.uiTotalSize);
			if (pOwner->GetShouldValidate(Validation.uiTotalSize))
			{
				spTask->CleanFilesFromDrive(Validation);
				bool bCache = spTask->ValidateFiles(Validation);
				if (bCache)
				{
					CacheDrive* pDrive = Validation.rCacheDrive;
					Validation.uiNumPartitions = pDrive->GetPartitionInfo(Validation.pPartitionsA, 2);
					unsigned int uiCount = Validation.uiNumPartitions;
					for (unsigned int ui = 0; ui < uiCount; ++ui)
					{
						uint64_t uiFree = 0;
						Validation.pPartitionsA[ui]->GetLocationFreeSpace(uiFree);
						Validation.uiPartitionFreeSpace[ui] = uiFree;
					}
					bCache = spTask->ValidateSpace(Validation);
				}
				Validation.rCacheDrive->PostValidate(Validation.uiTotalSize);
				if (bCache)
				{
					unsigned int uiPriority = uiInitialTargetPriority;
					CacheDrive* pDrive = rOwner;
					Task* pCurrentTask = spTask.pPtr;
					unsigned int uiSize = pCurrentTask->uiMaximumBufferSize > 0x40000 ? pCurrentTask->uiMaximumBufferSize : 0x40000;
					CacheContext Context{pDrive, nullptr, nullptr, uiSize, BSTSmartPointer<Task>(pCurrentTask),
						uiPriority, &uiCurrentPriority, {}, {}, &uiPauseFlags};
					Context.BlockingMutex.Wait();
					void* pBuffer = VirtualAlloc(nullptr, uiSize, MEM_RESERVE, PAGE_READWRITE);
					Context.pBuffer = pBuffer;
					if (pBuffer && VirtualAlloc(pBuffer, uiSize, MEM_COMMIT, PAGE_READWRITE))
						pCurrentTask->Cache(Context);
					if (pBuffer)
						VirtualFree(pBuffer, 0, MEM_RELEASE);
				}
				if (bCache || Validation.bFullyCached)
					RegisterLocation(rOwner, 0);
			}
			spTask->InitialCachingComplete();
		}
		uiState = 4;
		ReleaseSemaphore(ExitSema.hSemaphore, 1, nullptr);
		return 0;
	}

	void CacheDrive::Impl::Resume()
	{
		if (GetCurrentThreadId() == m_ThreadID)
			return;
		unsigned int uiFlags;
		do { uiFlags = uiPauseFlags; }
		while (static_cast<unsigned int>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&uiPauseFlags), uiFlags & ~1u, uiFlags)) != uiFlags);
		PauseEvent.Signal();
		if (bBlocked)
		{
			BlockingMutex.Release();
			bBlocked = false;
		}
	}
}

template unsigned int ArrayPartitionRecursive<BSTSmartPointer<BSResource::CacheDrive::Op>, BSResource::CacheDrive::Task::OpSortFunctor, BSTArrayHeapAllocator>(
	BSTArray<BSTSmartPointer<BSResource::CacheDrive::Op>>&, const BSResource::CacheDrive::Task::OpSortFunctor&, unsigned int, unsigned int);
