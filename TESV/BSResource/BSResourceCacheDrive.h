#pragma once

#include "BSResource/BSResourceLooseFiles.h"
#include "BSResource/BSResourcestream.h"
#include "BSCore/BSTArray.h"
#include "BSCore/BSTEvent.h"
#include "BSCore/BSThread.h"
#include "BSCore/BSSemaphore.h"

class ScrapHeap;

namespace BSResource
{
	enum CacheEventType : int { ADDOP, COPIED, SETTARGET = 3, ALLDONE };
	class CacheEvent
	{
	public:
		CacheEventType eType;
		const BSFixedString* rFileName;
		float fValue;
		const BSFixedString* rStringValue;
	};
	extern BSTEventSource<CacheEvent> CacheEventSource;
	BSTEventSource<CacheEvent>* QCacheEventSource();

	class CachePauseEvent
	{
	public:
		CachePauseEvent();
		~CachePauseEvent();
		void Signal() const;
		void Wait() const;
		HANDLE MyEvent;
		static unsigned int uiCount;
	};

	class PauseBlockMutex
	{
	public:
		PauseBlockMutex();
		~PauseBlockMutex();
		void Release();
		void Wait();
		HANDLE MyMutex;
		static unsigned int uiCount;
	};

	template <class Pointer>
	class FunctorBase
	{
	public:
		FunctorBase(const Pointer& arSource, uint64_t auiSize) : spSource(arSource), uiStreamSize(auiSize) {}
		ErrorCode Open() const
		{
			unsigned int uiFlags;
			do { uiFlags = spSource->uiFlags; }
			while (static_cast<unsigned int>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&spSource->uiFlags), uiFlags | 8, uiFlags)) != uiFlags);
			return spSource->DoOpen();
		}
		void Close() const
		{
			unsigned int uiFlags;
			do { uiFlags = spSource->uiFlags; }
			while (static_cast<unsigned int>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&spSource->uiFlags), uiFlags & ~14u, uiFlags)) != uiFlags);
			spSource->DoClose();
		}
		Pointer spSource;
		uint64_t uiStreamSize;
	};

	class AsyncFunctor : public FunctorBase<BSTSmartPointer<AsyncStream>>
	{
	public:
		AsyncFunctor(const BSTSmartPointer<AsyncStream>& arSource, uint64_t auiSize) : FunctorBase(arSource, auiSize), uiStreamPos(0) {}
		ErrorCode Read(void* apBuffer, uint64_t auiBytes, uint64_t& arRead) const;
		mutable uint64_t uiStreamPos;
	};

	class StandardFunctor : public FunctorBase<BSTSmartPointer<Stream>>
	{
	public:
		using FunctorBase::FunctorBase;
		ErrorCode Read(void* apBuffer, uint64_t auiBytes, uint64_t& arRead) const;
	};

	class CacheDrive : public Location
	{
	public:
		class Op;
		class Task;
		class Impl;
		struct ValidateContext;
		struct CacheContext;

		explicit CacheDrive(bool abInitialize);
		CacheDrive(unsigned int auiPacketSize, bool abAsyncSupported, bool abInitialize);
		~CacheDrive() override;
		bool RegisterTask(const BSTSmartPointer<Task>& arTask, unsigned int auiPriority);
		void Pause(bool abBlocking);
		void Resume();
		void Exit();
		unsigned int QNumPartitions() const;
		LooseFileLocation* QPartition(unsigned int auiIndex) const;
		unsigned int GetPartitionInfo(LooseFileLocation** apPartitions, unsigned int auiMax);
		ErrorCode CreateStreamInternal(const char*, BSTSmartPointer<Stream>&, LooseFileLocation*&);
		ErrorCode DoCreateStream(const char*, BSTSmartPointer<Stream>&, Location*&, bool) override;
		ErrorCode DoCreateAsyncStream(const char*, BSTSmartPointer<AsyncStream>&, Location*&, bool) override;
		ErrorCode DoTraversePrefix(const char*, LocationTraverser&) override;
		ErrorCode DoGetInfo(const char*, Info&) override;
		ErrorCode DoGetInfo(const char*, Info&, Location*&) override;
		ErrorCode DoDelete(const char*) override;
		unsigned int DoGetMinimumAsyncPacketSize() const override;
		static bool SetupDrive(bool);
		bool GetShouldValidate(uint64_t);
		void PostValidate(uint64_t);
		void FlushCache();

		BSTSmartPointer<Impl> spImpl;
		unsigned int uiMinimumAsyncPacketSize;
		LooseFileLocation* pPartitions;
		unsigned int uiNumPartitions;
		bool bAsyncSupported;
		bool bAvailable;
	};

	class CacheDrive::Op
	{
	public:
		Op(const BSFixedString& arPath, Location& arSource);
		virtual ~Op();
		virtual void OnValid(CacheDrive&);
		virtual void OnInit(CacheDrive&);
		virtual void OnPacketWrite(CacheDrive&, uint64_t);
		virtual void OnComplete(CacheDrive&);
		virtual void OnError(CacheDrive&);
		unsigned int IncRef() { return static_cast<unsigned int>(InterlockedIncrement(reinterpret_cast<volatile LONG*>(&uiRefCount))); }
		unsigned int DecRef() { return static_cast<unsigned int>(InterlockedDecrement(reinterpret_cast<volatile LONG*>(&uiRefCount))); }
		volatile unsigned int uiRefCount;
		uint64_t uiFileSize;
		BSFixedString Path;
		BSTSmartPointer<AsyncStream> spAsyncSource;
		BSTSmartPointer<Stream> spSource;
		Location* pSourceLocation;
		LooseFileLocation* pDestLocation;
		unsigned int uiPriority;
	};

	class CacheDrive::Task
	{
	public:
		explicit Task(const char* apCleanFiles);
		virtual ~Task();
		virtual void OnBeginInitial(unsigned int, volatile unsigned int*);
		virtual void OnAccumulateTotalsComplete();
		virtual void OnValidateFilesComplete(uint64_t);
		virtual void OnValidateSpaceComplete();
		virtual void OnUpdateTotalProcessed(uint64_t);
		virtual void OnBeginCaching();
		virtual void OnInitialCachingComplete();
		void AddOp(const BSTSmartPointer<Op>&, unsigned int);
		void AccumulateTotals(uint64_t&);
		void CleanFilesFromDrive(ValidateContext&);
		bool ValidateFiles(ValidateContext&);
		bool ValidateSpace(ValidateContext&);
		void Cache(CacheContext&);
		template <class Functor>
		ErrorCode CopyStream(CacheContext&, const BSTSmartPointer<Stream>&, const Functor&, const BSTSmartPointer<Op>&);
		ErrorCode CopyStreamASync(CacheContext&, const BSTSmartPointer<AsyncStream>&, const AsyncFunctor&, const BSTSmartPointer<Op>&);
		struct OpSortFunctor
		{
			int operator()(const BSTSmartPointer<Op>& arFirst, const BSTSmartPointer<Op>& arSecond) const
			{
				unsigned int uiFirst = arFirst->uiPriority;
				unsigned int uiSecond = arSecond->uiPriority;
				return uiFirst < uiSecond ? -1 : (uiFirst > uiSecond ? 1 : 0);
			}
		};
		void SortOps();
		void InitialCachingComplete();
		static ErrorCode SetDestStreamSize(const BSTSmartPointer<Stream>&, uint64_t);
		static void OnValid(const BSTSmartPointer<Op>&, CacheDrive&);
		static bool InfoEqual(const Info&, const Info&);
		unsigned int IncRef() { return static_cast<unsigned int>(InterlockedIncrement(reinterpret_cast<volatile LONG*>(&uiRefCount))); }
		unsigned int DecRef() { return static_cast<unsigned int>(InterlockedDecrement(reinterpret_cast<volatile LONG*>(&uiRefCount))); }
		volatile unsigned int uiRefCount;
		BSTArray<BSTSmartPointer<Op>> Ops;
		const char* pCleanFilesList;
		unsigned int uiMaximumPacketAlign;
		unsigned int uiMaximumBufferSize;
		volatile bool bCancel;
		volatile bool bInitialCachingCompleted;
	};

	class CacheDrive::Impl : public BSThread
	{
	public:
		Impl(CacheDrive&, const BSTSmartPointer<Task>&, unsigned int);
		~Impl() override;
		unsigned int ThreadProc() override;
		void Resume();
		unsigned int IncRef() { return static_cast<unsigned int>(InterlockedIncrement(reinterpret_cast<volatile LONG*>(&uiRefCount))); }
		unsigned int DecRef() { return static_cast<unsigned int>(InterlockedDecrement(reinterpret_cast<volatile LONG*>(&uiRefCount))); }
		alignas(8) volatile unsigned int uiRefCount;
		CacheDrive* rOwner;
		BSSemaphore ExitSema;
		BSTSmartPointer<Task> spTask;
		volatile unsigned int uiState;
		unsigned int uiInitialTargetPriority;
		unsigned int uiCurrentPriority;
		CachePauseEvent PauseEvent;
		PauseBlockMutex BlockingMutex;
		volatile unsigned int uiPauseFlags;
		bool bBlocked;
	};

	struct CacheDrive::ValidateContext
	{
		uint64_t uiPartitionFreeSpace[2];
		LooseFileLocation* pPartitionsA[2];
		CacheDrive* rCacheDrive;
		uint64_t uiTotalSize;
		uint64_t uiInitialTargetPriority;
		bool bFullyCached;
		unsigned int uiNumPartitions;
	};

	struct CacheDrive::CacheContext
	{
		CacheDrive* rCacheDrive;
		ScrapHeap* pScrapHeap;
		void* pBuffer;
		unsigned int uiBufferSize;
		BSTSmartPointer<Task> spTask;
		unsigned int uiInitialTargetPriority;
		volatile unsigned int* ruiCurrentPriority;
		CachePauseEvent PauseEvent;
		PauseBlockMutex BlockingMutex;
		volatile unsigned int* ruiPauseFlags;
		void TestProgress();
	};

	struct CleanFilesFunctor
	{
		void operator()(const char* apName) const;
		CacheDrive* rCacheDrive;
	};

	template <class Functor>
	void TraverseNameTextList(const char* apText, Functor& arFunctor)
	{
		char Name[260];
		while (*apText)
		{
			while (*apText == '\t' || *apText == ' ')
				++apText;
			if (!*apText)
				return;
			char* pEnd = Name;
			while (*apText && *apText != ',')
				*pEnd++ = *apText++;
			*pEnd = 0;
			arFunctor(Name);
			if (*apText)
				++apText;
		}
	}

	static_assert(sizeof(CacheEvent) == 32);
	static_assert(sizeof(CacheDrive::ValidateContext) == 64);
	static_assert(offsetof(CacheDrive::ValidateContext, uiTotalSize) == 40);
	static_assert(offsetof(CacheDrive::ValidateContext, uiNumPartitions) == 60);
	static_assert(sizeof(CacheDrive::CacheContext) == 80);
	static_assert(offsetof(CacheDrive::CacheContext, spTask) == 32);
	static_assert(offsetof(CacheDrive::CacheContext, ruiPauseFlags) == 72);

	static_assert(sizeof(CacheDrive) == 48);
	static_assert(offsetof(CacheDrive, spImpl) == 16);
	static_assert(offsetof(CacheDrive, pPartitions) == 32);
	static_assert(offsetof(CacheDrive, uiNumPartitions) == 40);
	static_assert(sizeof(CacheDrive::Op) == 72);
	static_assert(offsetof(CacheDrive::Op, uiRefCount) == 8);
	static_assert(offsetof(CacheDrive::Op, uiFileSize) == 16);
	static_assert(offsetof(CacheDrive::Op, uiPriority) == 64);
	static_assert(sizeof(CacheDrive::Task) == 64);
	static_assert(offsetof(CacheDrive::Task, Ops) == 16);
	static_assert(offsetof(CacheDrive::Task, pCleanFilesList) == 40);
	static_assert(offsetof(CacheDrive::Task, bCancel) == 56);
	static_assert(sizeof(CacheDrive::Impl) == 152);
	static_assert(offsetof(CacheDrive::Impl, uiRefCount) == 80);
	static_assert(offsetof(CacheDrive::Impl, rOwner) == 88);
	static_assert(offsetof(CacheDrive::Impl, spTask) == 104);
	static_assert(offsetof(CacheDrive::Impl, uiPauseFlags) == 144);
}

