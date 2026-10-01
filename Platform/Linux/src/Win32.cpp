#include <windows.h>

#include <cstdio>

#include <sys/sysinfo.h>

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
