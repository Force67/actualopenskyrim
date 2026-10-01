#include <windows.h>

#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <ctime>

#include <strings.h>
#include <pthread.h>
#include <sys/resource.h>
#include <sys/syscall.h>

void ExitThread(DWORD dwExitCode)
{
	pthread_exit(reinterpret_cast<void*>(static_cast<uintptr_t>(dwExitCode)));
}

#include <sys/sysinfo.h>

extern "C" int* _errno()
{
	return &errno;
}

extern "C" void _invalid_parameter_noinfo()
{
	std::abort();
}

extern "C" int _stricmp(const char* apFirst, const char* apSecond)
{
	return strcasecmp(apFirst, apSecond);
}

extern "C" int _wcsicmp(const wchar_t* apFirst, const wchar_t* apSecond)
{
	return wcscasecmp(apFirst, apSecond);
}

int64_t _time64(int64_t* apTime)
{
	int64_t iTime = static_cast<int64_t>(time(nullptr));
	if (apTime)
		*apTime = iTime;
	return iTime;
}

DWORD GetTickCount()
{
	timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts))
		return 0;
	return static_cast<DWORD>(uint64_t(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000);
}

BOOL QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount)
{
	timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts))
		return 0;
	lpPerformanceCount->QuadPart = int64_t(ts.tv_sec) * 1000000000 + ts.tv_nsec;
	return 1;
}

BOOL QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency)
{
	lpFrequency->QuadPart = 1000000000;
	return 1;
}

BOOL GlobalMemoryStatusEx(MEMORYSTATUSEX* lpBuffer)
{
	struct sysinfo info;
	if (sysinfo(&info))
		return 0;
	const uint64_t unit = info.mem_unit;
	lpBuffer->dwMemoryLoad = 0;
	lpBuffer->ullTotalPhys = info.totalram * unit;
	lpBuffer->ullAvailPhys = info.freeram * unit;
	lpBuffer->ullTotalPageFile = (info.totalram + info.totalswap) * unit;
	lpBuffer->ullAvailPageFile = (info.freeram + info.freeswap) * unit;
	lpBuffer->ullTotalVirtual = UINT64_MAX;
	lpBuffer->ullAvailVirtual = UINT64_MAX;
	lpBuffer->ullAvailExtendedVirtual = 0;
	return 1;
}

int MessageBoxA(void*, const char* lpText, const char* lpCaption, unsigned int)
{
	fprintf(stderr, "%s: %s\n", lpCaption, lpText);
	return 1;
}

#include <map>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <unordered_map>

namespace
{
	struct Semaphore
	{
		std::mutex mutex;
		std::condition_variable changed;
		LONG count;
		LONG maximum;
	};
	struct Thread
	{
		std::mutex mutex;
		std::condition_variable changed;
		DWORD id = 0;
		bool finished = false;
		LPTHREAD_START_ROUTINE start;
		void* parameter;
	};
	std::unordered_map<HANDLE, std::shared_ptr<Thread>> threads;

	void ThreadFinished(void* pointer)
	{
		auto* thread = static_cast<Thread*>(pointer);
		std::lock_guard lock(thread->mutex);
		thread->finished = true;
		thread->changed.notify_all();
	}

	void* ThreadStart(void* pointer)
	{
		std::unique_ptr<std::shared_ptr<Thread>> argument(static_cast<std::shared_ptr<Thread>*>(pointer));
		auto thread = *argument;
		argument.reset();
		{
			std::lock_guard lock(thread->mutex);
			thread->id = static_cast<DWORD>(syscall(SYS_gettid));
			thread->changed.notify_all();
		}
		DWORD result;
		pthread_cleanup_push(ThreadFinished, thread.get());
		result = thread->start(thread->parameter);
		pthread_cleanup_pop(1);
		return reinterpret_cast<void*>(static_cast<uintptr_t>(result));
	}
	std::mutex handleMutex;
	std::unordered_map<HANDLE, std::shared_ptr<Semaphore>> semaphores;

	std::shared_ptr<Thread> FindThread(HANDLE handle)
	{
		std::lock_guard lock(handleMutex);
		const auto it = threads.find(handle);
		return it == threads.end() ? nullptr : it->second;
	}

	std::shared_ptr<Semaphore> FindSemaphore(HANDLE handle)
	{
		std::lock_guard lock(handleMutex);
		const auto it = semaphores.find(handle);
		return it == semaphores.end() ? nullptr : it->second;
	}
}

HANDLE GetCurrentThread()
{
	return reinterpret_cast<HANDLE>(intptr_t(-2));
}

HANDLE CreateThread(void* lpThreadAttributes, size_t dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress,
	void* lpParameter, DWORD dwCreationFlags, DWORD* lpThreadId)
{
	if (lpThreadAttributes || !lpStartAddress || dwCreationFlags)
		return nullptr;
	pthread_attr_t attributes;
	if (pthread_attr_init(&attributes))
		return nullptr;
	int error = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	if (!error && dwStackSize)
		error = pthread_attr_setstacksize(&attributes, dwStackSize);
	auto thread = std::make_shared<Thread>();
	thread->start = lpStartAddress;
	thread->parameter = lpParameter;
	auto* argument = new std::shared_ptr<Thread>(thread);
	pthread_t native;
	if (!error)
		error = pthread_create(&native, &attributes, ThreadStart, argument);
	pthread_attr_destroy(&attributes);
	if (error)
	{
		delete argument;
		return nullptr;
	}
	{
		std::unique_lock lock(thread->mutex);
		thread->changed.wait(lock, [&] { return thread->id != 0; });
		if (lpThreadId)
			*lpThreadId = thread->id;
	}
	HANDLE handle = thread.get();
	std::lock_guard lock(handleMutex);
	threads.emplace(handle, std::move(thread));
	return handle;
}

DWORD ResumeThread(HANDLE hThread)
{
	return FindThread(hThread) ? 0 : DWORD(-1);
}

BOOL SetThreadPriority(HANDLE hThread, int nPriority)
{
	int nice;
	switch (nPriority)
	{
	case -15: nice = 19; break;
	case -2: nice = 2; break;
	case -1: nice = 1; break;
	case 0: nice = 0; break;
	case 1: nice = -1; break;
	case 2: nice = -2; break;
	case 15: nice = -20; break;
	default: return 0;
	}
	auto thread = FindThread(hThread);
	DWORD id;
	if (hThread == GetCurrentThread())
		id = static_cast<DWORD>(syscall(SYS_gettid));
	else
	{
		if (!thread)
			return 0;
		std::lock_guard lock(thread->mutex);
		if (thread->finished)
			return 0;
		id = thread->id;
		return setpriority(PRIO_PROCESS, id, nice) == 0;
	}
	return setpriority(PRIO_PROCESS, id, nice) == 0;
}

HANDLE CreateSemaphoreW(void*, LONG lInitialCount, LONG lMaximumCount, const wchar_t* lpName)
{
	if (lpName || lMaximumCount <= 0 || lInitialCount < 0 || lInitialCount > lMaximumCount)
		return nullptr;
	auto semaphore = std::make_shared<Semaphore>();
	semaphore->count = lInitialCount;
	semaphore->maximum = lMaximumCount;
	HANDLE handle = semaphore.get();
	std::lock_guard lock(handleMutex);
	semaphores.emplace(handle, std::move(semaphore));
	return handle;
}

BOOL ReleaseSemaphore(HANDLE hSemaphore, LONG lReleaseCount, LONG* lpPreviousCount)
{
	auto semaphore = FindSemaphore(hSemaphore);
	if (!semaphore || lReleaseCount <= 0)
		return 0;
	std::lock_guard lock(semaphore->mutex);
	if (lReleaseCount > semaphore->maximum - semaphore->count)
		return 0;
	if (lpPreviousCount)
		*lpPreviousCount = semaphore->count;
	semaphore->count += lReleaseCount;
	semaphore->changed.notify_all();
	return 1;
}

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
	auto semaphore = FindSemaphore(hHandle);
	if (!semaphore)
	{
		auto thread = FindThread(hHandle);
		if (!thread)
			return WAIT_FAILED;
		std::unique_lock lock(thread->mutex);
		const auto ready = [&] { return thread->finished; };
		if (dwMilliseconds == INFINITE)
			thread->changed.wait(lock, ready);
		else if (!thread->changed.wait_for(lock, std::chrono::milliseconds(dwMilliseconds), ready))
			return WAIT_TIMEOUT;
		return WAIT_OBJECT_0;
	}
	std::unique_lock lock(semaphore->mutex);
	const auto ready = [&] { return semaphore->count != 0; };
	if (dwMilliseconds == INFINITE)
		semaphore->changed.wait(lock, ready);
	else if (!semaphore->changed.wait_for(lock, std::chrono::milliseconds(dwMilliseconds), ready))
		return WAIT_TIMEOUT;
	--semaphore->count;
	return WAIT_OBJECT_0;
}

BOOL CloseHandle(HANDLE hObject)
{
	std::lock_guard lock(handleMutex);
	return semaphores.erase(hObject) != 0 || threads.erase(hObject) != 0;
}
#include <sys/mman.h>

static std::map<uintptr_t, size_t> virtualReservations;
static std::mutex virtualReservationLock;

void* VirtualAlloc(void* lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect)
{
	if (!dwSize || flProtect != PAGE_READWRITE)
		return nullptr;
	std::lock_guard<std::mutex> lock(virtualReservationLock);
	const size_t uiPageSize = static_cast<size_t>(sysconf(_SC_PAGESIZE));
	if ((flAllocationType & MEM_RESERVE) || !lpAddress)
	{
		if (lpAddress)
			return nullptr;
		const size_t uiSize = (dwSize + 0xFFFF) & ~size_t(0xFFFF);
		void* pMapping = mmap(nullptr, uiSize + 0x10000, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (pMapping == MAP_FAILED)
			return nullptr;
		const auto uiMapping = reinterpret_cast<uintptr_t>(pMapping);
		const auto uiBase = (uiMapping + 0xFFFF) & ~uintptr_t(0xFFFF);
		if (uiBase != uiMapping)
			munmap(pMapping, uiBase - uiMapping);
		munmap(reinterpret_cast<void*>(uiBase + uiSize), 0x10000 - (uiBase - uiMapping));
		lpAddress = reinterpret_cast<void*>(uiBase);
		if ((flAllocationType & MEM_COMMIT) && mprotect(lpAddress, (dwSize + uiPageSize - 1) & ~(uiPageSize - 1), PROT_READ | PROT_WRITE))
		{
			munmap(lpAddress, uiSize);
			return nullptr;
		}
		virtualReservations.emplace(uiBase, uiSize);
		return lpAddress;
	}
	if (flAllocationType != MEM_COMMIT)
		return nullptr;
	const auto uiAddress = reinterpret_cast<uintptr_t>(lpAddress);
	const auto uiStart = uiAddress & ~(uiPageSize - 1);
	const auto uiEnd = (uiAddress + dwSize + uiPageSize - 1) & ~(uiPageSize - 1);
	auto it = virtualReservations.upper_bound(uiStart);
	if (it == virtualReservations.begin())
		return nullptr;
	--it;
	if (uiEnd < uiStart || uiEnd > it->first + it->second)
		return nullptr;
	return mprotect(reinterpret_cast<void*>(uiStart), uiEnd - uiStart, PROT_READ | PROT_WRITE) ? nullptr : reinterpret_cast<void*>(uiStart);
}

BOOL VirtualFree(void* lpAddress, size_t dwSize, DWORD dwFreeType)
{
	std::lock_guard<std::mutex> lock(virtualReservationLock);
	const auto uiAddress = reinterpret_cast<uintptr_t>(lpAddress);
	if (dwFreeType == MEM_RELEASE)
	{
		auto it = virtualReservations.find(uiAddress);
		if (dwSize || it == virtualReservations.end())
			return 0;
		if (munmap(lpAddress, it->second))
			return 0;
		virtualReservations.erase(it);
		return 1;
	}
	if (dwFreeType != MEM_DECOMMIT || !dwSize)
		return 0;
	const size_t uiPageSize = static_cast<size_t>(sysconf(_SC_PAGESIZE));
	const auto uiStart = uiAddress & ~(uiPageSize - 1);
	const auto uiEnd = (uiAddress + dwSize + uiPageSize - 1) & ~(uiPageSize - 1);
	auto it = virtualReservations.upper_bound(uiStart);
	if (it == virtualReservations.begin())
		return 0;
	--it;
	if (uiEnd < uiStart || uiEnd > it->first + it->second)
		return 0;
	if (mprotect(reinterpret_cast<void*>(uiStart), uiEnd - uiStart, PROT_NONE))
		return 0;
	return madvise(reinterpret_cast<void*>(uiStart), uiEnd - uiStart, MADV_DONTNEED) == 0;
}

void InitializeCriticalSection(CRITICAL_SECTION* lpCriticalSection)
{
	*lpCriticalSection = {};
	lpCriticalSection->LockCount = -1;
	lpCriticalSection->LockSemaphore = new std::recursive_mutex;
}

void DeleteCriticalSection(CRITICAL_SECTION* lpCriticalSection)
{
	delete static_cast<std::recursive_mutex*>(lpCriticalSection->LockSemaphore);
}

void EnterCriticalSection(CRITICAL_SECTION* lpCriticalSection)
{
	static_cast<std::recursive_mutex*>(lpCriticalSection->LockSemaphore)->lock();
	lpCriticalSection->OwningThread = reinterpret_cast<void*>(static_cast<uintptr_t>(GetCurrentThreadId()));
	++lpCriticalSection->RecursionCount;
}

void LeaveCriticalSection(CRITICAL_SECTION* lpCriticalSection)
{
	if (!--lpCriticalSection->RecursionCount)
		lpCriticalSection->OwningThread = nullptr;
	static_cast<std::recursive_mutex*>(lpCriticalSection->LockSemaphore)->unlock();
}

struct TlsSlot
{
	uint64_t generation = 0;
	bool allocated = false;
};
struct TlsValue
{
	uint64_t generation = 0;
	void* value = nullptr;
};
static TlsSlot tlsSlots[1088];
static std::mutex tlsLock;
static thread_local TlsValue tlsValues[1088];

DWORD TlsAlloc()
{
	std::lock_guard<std::mutex> lock(tlsLock);
	for (DWORD i = 0; i < 1088; ++i)
	{
		if (!tlsSlots[i].allocated)
		{
			tlsSlots[i].allocated = true;
			++tlsSlots[i].generation;
			return i;
		}
	}
	return TLS_OUT_OF_INDEXES;
}

BOOL TlsFree(DWORD dwTlsIndex)
{
	std::lock_guard<std::mutex> lock(tlsLock);
	if (dwTlsIndex >= 1088 || !tlsSlots[dwTlsIndex].allocated)
		return 0;
	tlsSlots[dwTlsIndex].allocated = false;
	return 1;
}

void* TlsGetValue(DWORD dwTlsIndex)
{
	std::lock_guard<std::mutex> lock(tlsLock);
	if (dwTlsIndex >= 1088 || !tlsSlots[dwTlsIndex].allocated || tlsValues[dwTlsIndex].generation != tlsSlots[dwTlsIndex].generation)
		return nullptr;
	return tlsValues[dwTlsIndex].value;
}

BOOL TlsSetValue(DWORD dwTlsIndex, void* lpTlsValue)
{
	std::lock_guard<std::mutex> lock(tlsLock);
	if (dwTlsIndex >= 1088 || !tlsSlots[dwTlsIndex].allocated)
		return 0;
	tlsValues[dwTlsIndex] = { tlsSlots[dwTlsIndex].generation, lpTlsValue };
	return 1;
}
