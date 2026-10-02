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
#include <sys/stat.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <dirent.h>
#include <fnmatch.h>
#include <unistd.h>

#include <vector>

DWORD GetFileAttributesA(const char* lpFileName)
{
	struct stat kLink, kFile;
	if (!lpFileName || lstat(lpFileName, &kLink) || stat(lpFileName, &kFile))
		return INVALID_FILE_ATTRIBUTES;
	DWORD ulAttributes = S_ISDIR(kFile.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : 0;
	if (S_ISLNK(kLink.st_mode))
		ulAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;
	if (!(kFile.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)))
		ulAttributes |= FILE_ATTRIBUTE_READONLY;
	const char* pcName = strrchr(lpFileName, '/');
	pcName = pcName ? pcName + 1 : lpFileName;
	if (pcName[0] == '.' && pcName[1] && strcmp(pcName, ".."))
		ulAttributes |= FILE_ATTRIBUTE_HIDDEN;
	return ulAttributes ? ulAttributes : FILE_ATTRIBUTE_NORMAL;
}

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

extern "C" int fopen_s(FILE** pFile, const char* filename, const char* mode)
{
	if (!pFile || !filename || !mode)
	{
		_invalid_parameter_noinfo();
		errno = EINVAL;
		return EINVAL;
	}
	*pFile = fopen(filename, mode);
	return *pFile ? 0 : errno;
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
#include <unordered_set>

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
		bool started = false;
		DWORD suspendCount = 0;
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
			std::unique_lock lock(thread->mutex);
			thread->id = static_cast<DWORD>(syscall(SYS_gettid));
			thread->changed.notify_all();
			thread->changed.wait(lock, [&] { return thread->suspendCount == 0; });
			thread->started = true;
		}
		DWORD result;
		pthread_cleanup_push(ThreadFinished, thread.get());
		result = thread->start(thread->parameter);
		pthread_cleanup_pop(1);
		return reinterpret_cast<void*>(static_cast<uintptr_t>(result));
	}
	const int processBasePriority = getpriority(PRIO_PROCESS, getpid());
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
	if (lpThreadAttributes || !lpStartAddress || (dwCreationFlags & ~DWORD(4)))
		return nullptr;
	pthread_attr_t attributes;
	if (pthread_attr_init(&attributes))
		return nullptr;
	int error = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	if (!error && dwStackSize)
		error = pthread_attr_setstacksize(&attributes, dwStackSize);
	auto thread = std::make_shared<Thread>();
	thread->suspendCount = dwCreationFlags == 4 ? 1 : 0;
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
	auto thread = FindThread(hThread);
	if (!thread)
		return DWORD(-1);
	std::lock_guard lock(thread->mutex);
	DWORD previous = thread->suspendCount;
	if (previous)
	{
		--thread->suspendCount;
		if (!thread->suspendCount)
			thread->changed.notify_all();
	}
	return previous;
}

DWORD SuspendThread(HANDLE hThread)
{
	auto thread = FindThread(hThread);
	if (!thread)
		return DWORD(-1);
	std::lock_guard lock(thread->mutex);
	if (thread->finished || thread->started || thread->suspendCount >= 127)
		return DWORD(-1);
	return thread->suspendCount++;
}

uintptr_t SetThreadAffinityMask(HANDLE hThread, uintptr_t dwThreadAffinityMask)
{
	auto thread = FindThread(hThread);
	std::unique_lock<std::mutex> lock;
	DWORD id;
	if (hThread == GetCurrentThread())
		id = static_cast<DWORD>(syscall(SYS_gettid));
	else
	{
		if (!thread)
			return 0;
		lock = std::unique_lock(thread->mutex);
		if (thread->finished)
			return 0;
		id = thread->id;
	}
	cpu_set_t previousSet;
	if (sched_getaffinity(id, sizeof(previousSet), &previousSet))
		return 0;
	uintptr_t previous = 0;
	cpu_set_t requestedSet;
	CPU_ZERO(&requestedSet);
	for (unsigned int cpu = 0; cpu < sizeof(uintptr_t) * 8; ++cpu)
	{
		if (CPU_ISSET(cpu, &previousSet))
			previous |= uintptr_t(1) << cpu;
		if (dwThreadAffinityMask & (uintptr_t(1) << cpu))
			CPU_SET(cpu, &requestedSet);
	}
	if (!previous || sched_setaffinity(id, sizeof(requestedSet), &requestedSet))
		return 0;
	return previous;
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
	nice += processBasePriority;
	if (nice > 19)
		nice = 19;
	else if (nice < -20)
		nice = -20;
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

namespace
{
	// Only handles handed out by the file API below are descriptors; anything
	// else belongs to the semaphore and thread tables above.
	std::mutex fileHandleMutex;
	std::unordered_set<HANDLE> fileHandles;

	bool IsFileHandle(HANDLE hObject)
	{
		std::lock_guard lock(fileHandleMutex);
		return fileHandles.count(hObject) != 0;
	}

	void RememberFileHandle(HANDLE hObject)
	{
		std::lock_guard lock(fileHandleMutex);
		fileHandles.insert(hObject);
	}

	bool ForgetFileHandle(HANDLE hObject)
	{
		std::lock_guard lock(fileHandleMutex);
		return fileHandles.erase(hObject) != 0;
	}
}

BOOL CloseHandle(HANDLE hObject)
{
	if (IsFileHandle(hObject))
	{
		ForgetFileHandle(hObject);
		return ::close(reinterpret_cast<intptr_t>(hObject)) == 0;
	}
	std::lock_guard lock(handleMutex);
	return semaphores.erase(hObject) != 0 || threads.erase(hObject) != 0;
}

namespace
{
	thread_local DWORD tlastError = NO_ERROR;

	struct Completion
	{
		LPOVERLAPPED_COMPLETION_ROUTINE routine;
		DWORD errorCode;
		DWORD bytes;
		OVERLAPPED* overlapped;
	};
	thread_local std::vector<Completion> tcompletions;

	DWORD ErrnoToWinError(int iErrno)
	{
		switch (iErrno)
		{
		case ENOENT:
			return ERROR_FILE_NOT_FOUND;
		case ENOTDIR:
			return ERROR_PATH_NOT_FOUND;
		case EEXIST:
			return ERROR_ALREADY_EXISTS;
		case EACCES:
		case EPERM:
		case EROFS:
			return ERROR_ACCESS_DENIED;
		default:
			return ERROR_GEN_FAILURE;
		}
	}

	FILETIME TimespecToFileTime(const timespec& ts)
	{
		// 100 ns units since 1601-01-01; Unix epoch starts 11644473600 s later.
		uint64_t uiStamp = static_cast<uint64_t>(ts.tv_sec) * 10000000ULL +
			static_cast<uint64_t>(ts.tv_nsec) / 100ULL + 116444736000000000ULL;
		return FILETIME{ static_cast<DWORD>(uiStamp), static_cast<DWORD>(uiStamp >> 32) };
	}

	timespec FileTimeToTimespec(const FILETIME& ft)
	{
		uint64_t uiStamp = (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
		uint64_t uiUnix = uiStamp - 116444736000000000ULL;
		return timespec{ static_cast<time_t>(uiUnix / 10000000ULL), static_cast<long>(uiUnix % 10000000ULL) * 100 };
	}

	int FileHandleToFd(HANDLE hFile)
	{
		return static_cast<int>(reinterpret_cast<intptr_t>(hFile));
	}

	off_t OverlappedOffset(const OVERLAPPED& over)
	{
		return over.Pointer ? reinterpret_cast<off_t>(over.Pointer) :
			static_cast<off_t>((static_cast<uint64_t>(over.OffsetHigh) << 32) | over.Offset);
	}
}

DWORD GetLastError()
{
	return tlastError;
}

void SetLastError(DWORD dwErrCode)
{
	tlastError = dwErrCode;
}

HANDLE CreateFileA(const char* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, void* lpSecurityAttributes,
	DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	int iAccess = 0;
	if (dwDesiredAccess & GENERIC_READ)
		iAccess |= O_RDONLY;
	if (dwDesiredAccess & GENERIC_WRITE)
		iAccess |= O_WRONLY;
	if (!iAccess)
		iAccess = O_RDONLY;

	int iDisposition = 0;
	switch (dwCreationDisposition)
	{
	case CREATE_NEW:
		iDisposition = O_CREAT | O_EXCL;
		break;
	case CREATE_ALWAYS:
		iDisposition = O_CREAT | O_TRUNC;
		break;
	case OPEN_ALWAYS:
		iDisposition = O_CREAT;
		break;
	case TRUNCATE_EXISTING:
		iDisposition = O_TRUNC;
		break;
	default:
		break;
	}

	int fd = ::open(lpFileName, iAccess | iDisposition | O_CLOEXEC, 0644);
	if (fd < 0)
	{
		tlastError = ErrnoToWinError(errno);
		return INVALID_HANDLE_VALUE;
	}
	HANDLE hFile = reinterpret_cast<HANDLE>(static_cast<intptr_t>(fd));
	RememberFileHandle(hFile);
	return hFile;
}

BOOL ReadFile(HANDLE hFile, void* lpBuffer, DWORD nNumberOfBytesToRead, DWORD* lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
	int fd = FileHandleToFd(hFile);
	ssize_t n;
	if (lpOverlapped)
		n = ::pread(fd, lpBuffer, nNumberOfBytesToRead, OverlappedOffset(*lpOverlapped));
	else
		n = ::read(fd, lpBuffer, nNumberOfBytesToRead);
	if (n < 0)
	{
		tlastError = ErrnoToWinError(errno);
		return FALSE;
	}
	if (lpNumberOfBytesRead)
		*lpNumberOfBytesRead = static_cast<DWORD>(n);
	return TRUE;
}

BOOL WriteFile(HANDLE hFile, const void* lpBuffer, DWORD nNumberOfBytesToWrite, DWORD* lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	int fd = FileHandleToFd(hFile);
	ssize_t n;
	if (lpOverlapped)
		n = ::pwrite(fd, lpBuffer, nNumberOfBytesToWrite, OverlappedOffset(*lpOverlapped));
	else
		n = ::write(fd, lpBuffer, nNumberOfBytesToWrite);
	if (n < 0)
	{
		tlastError = ErrnoToWinError(errno);
		return FALSE;
	}
	if (lpNumberOfBytesWritten)
		*lpNumberOfBytesWritten = static_cast<DWORD>(n);
	return TRUE;
}

BOOL ReadFileEx(HANDLE hFile, void* lpBuffer, DWORD nNumberOfBytesToRead, LPOVERLAPPED lpOverlapped,
	LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	int fd = FileHandleToFd(hFile);
	ssize_t n = ::pread(fd, lpBuffer, nNumberOfBytesToRead, OverlappedOffset(*lpOverlapped));
	if (n < 0)
	{
		tlastError = ErrnoToWinError(errno);
		return FALSE;
	}
	tcompletions.push_back({ lpCompletionRoutine, NO_ERROR, static_cast<DWORD>(n), lpOverlapped });
	return TRUE;
}

BOOL WriteFileEx(HANDLE hFile, const void* lpBuffer, DWORD nNumberOfBytesToWrite, LPOVERLAPPED lpOverlapped,
	LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	int fd = FileHandleToFd(hFile);
	ssize_t n = ::pwrite(fd, lpBuffer, nNumberOfBytesToWrite, OverlappedOffset(*lpOverlapped));
	if (n < 0)
	{
		tlastError = ErrnoToWinError(errno);
		return FALSE;
	}
	tcompletions.push_back({ lpCompletionRoutine, NO_ERROR, static_cast<DWORD>(n), lpOverlapped });
	return TRUE;
}

BOOL SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove, LARGE_INTEGER* lpNewFilePointer, DWORD dwMoveMethod)
{
	int fd = FileHandleToFd(hFile);
	int iWhence = dwMoveMethod == FILE_END ? SEEK_END : dwMoveMethod == FILE_CURRENT ? SEEK_CUR : SEEK_SET;
	off_t pos = ::lseek(fd, liDistanceToMove.QuadPart, iWhence);
	if (pos < 0)
	{
		tlastError = ErrnoToWinError(errno);
		return FALSE;
	}
	if (lpNewFilePointer)
		lpNewFilePointer->QuadPart = pos;
	return TRUE;
}

BOOL GetFileSizeEx(HANDLE hFile, LARGE_INTEGER* lpFileSize)
{
	struct stat st;
	if (::fstat(FileHandleToFd(hFile), &st) != 0)
	{
		tlastError = ErrnoToWinError(errno);
		return FALSE;
	}
	lpFileSize->QuadPart = st.st_size;
	return TRUE;
}

BOOL SetEndOfFile(HANDLE hFile)
{
	int fd = FileHandleToFd(hFile);
	off_t pos = ::lseek(fd, 0, SEEK_CUR);
	if (pos < 0 || ::ftruncate(fd, pos) != 0)
	{
		tlastError = ErrnoToWinError(errno);
		return FALSE;
	}
	return TRUE;
}

BOOL FlushFileBuffers(HANDLE hFile)
{
	if (::fsync(FileHandleToFd(hFile)) != 0)
	{
		tlastError = ErrnoToWinError(errno);
		return FALSE;
	}
	return TRUE;
}

BOOL GetFileTime(HANDLE hFile, FILETIME* lpCreationTime, FILETIME* lpLastAccessTime, FILETIME* lpLastWriteTime)
{
	struct stat st;
	if (::fstat(FileHandleToFd(hFile), &st) != 0)
	{
		tlastError = ErrnoToWinError(errno);
		return FALSE;
	}
	if (lpLastAccessTime)
		*lpLastAccessTime = TimespecToFileTime(st.st_atim);
	if (lpLastWriteTime)
		*lpLastWriteTime = TimespecToFileTime(st.st_mtim);
	if (lpCreationTime)
		*lpCreationTime = TimespecToFileTime(st.st_ctim);
	return TRUE;
}

BOOL SetFileTime(HANDLE hFile, const FILETIME* lpCreationTime, const FILETIME* lpLastAccessTime, const FILETIME* lpLastWriteTime)
{
	timespec times[2];
	times[0] = lpLastAccessTime ? FileTimeToTimespec(*lpLastAccessTime) : timespec{ UTIME_NOW, 0 };
	times[1] = lpLastWriteTime ? FileTimeToTimespec(*lpLastWriteTime) : timespec{ UTIME_NOW, 0 };
	if (::futimens(FileHandleToFd(hFile), times) != 0)
	{
		tlastError = ErrnoToWinError(errno);
		return FALSE;
	}
	return TRUE;
}

BOOL DeleteFileA(const char* lpFileName)
{
	if (::unlink(lpFileName) != 0)
	{
		tlastError = ErrnoToWinError(errno);
		return FALSE;
	}
	return TRUE;
}

BOOL CopyFileA(const char* lpExistingFileName, const char* lpNewFileName, BOOL bFailIfExists)
{
	if (bFailIfExists && ::access(lpNewFileName, F_OK) == 0)
	{
		tlastError = ERROR_ALREADY_EXISTS;
		return FALSE;
	}
	int src = ::open(lpExistingFileName, O_RDONLY | O_CLOEXEC);
	if (src < 0)
	{
		tlastError = ErrnoToWinError(errno);
		return FALSE;
	}
	int dst = ::open(lpNewFileName, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
	if (dst < 0)
	{
		tlastError = ErrnoToWinError(errno);
		::close(src);
		return FALSE;
	}
	char buf[4096];
	BOOL bOk = TRUE;
	ssize_t n;
	while ((n = ::read(src, buf, sizeof(buf))) > 0)
	{
		ssize_t written = 0;
		while (written < n)
		{
			ssize_t w = ::write(dst, buf + written, n - written);
			if (w < 0)
			{
				bOk = FALSE;
				break;
			}
			written += w;
		}
		if (!bOk)
			break;
	}
	if (n < 0)
		bOk = FALSE;
	::close(src);
	::close(dst);
	if (!bOk)
		tlastError = ErrnoToWinError(errno);
	return bOk;
}

BOOL MoveFileA(const char* lpExistingFileName, const char* lpNewFileName)
{
	if (::rename(lpExistingFileName, lpNewFileName) != 0)
	{
		tlastError = ErrnoToWinError(errno);
		return FALSE;
	}
	return TRUE;
}

DWORD SleepEx(DWORD dwMilliseconds, BOOL bAlertable)
{
	if (bAlertable)
	{
		while (!tcompletions.empty())
		{
			Completion completion = tcompletions.back();
			tcompletions.pop_back();
			completion.routine(completion.errorCode, completion.bytes, completion.overlapped);
		}
	}
	Sleep(dwMilliseconds);
	return 0;
}

namespace
{
	struct FindState
	{
		DIR* pDir;
		char sDirectory[1024];
		char sPattern[256];
	};

	void FillFindData(const char* sPath, const char* sName, WIN32_FIND_DATAA* pFind)
	{
		memset(pFind, 0, sizeof(*pFind));
		struct stat st;
		if (::stat(sPath, &st) == 0)
		{
			if (S_ISDIR(st.st_mode))
				pFind->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			else
				pFind->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
			pFind->nFileSizeHigh = static_cast<DWORD>(st.st_size >> 32);
			pFind->nFileSizeLow = static_cast<DWORD>(st.st_size);
			pFind->ftCreationTime = TimespecToFileTime(st.st_ctim);
			pFind->ftLastAccessTime = TimespecToFileTime(st.st_atim);
			pFind->ftLastWriteTime = TimespecToFileTime(st.st_mtim);
		}
		else
		{
			pFind->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		}
		strcpy(pFind->cFileName, sName);
	}
}

HANDLE FindFirstFileA(const char* lpFileName, WIN32_FIND_DATAA* lpFindFileData)
{
	const char* sSlash = strrchr(lpFileName, '/');
	const char* sBackslash = strrchr(lpFileName, '\\');
	const char* sSplit = sSlash > sBackslash ? sSlash : sBackslash;
	char sDirectory[1024];
	const char* sPattern;
	if (sSplit)
	{
		size_t uDirLen = sSplit - lpFileName;
		if (uDirLen >= sizeof(sDirectory))
			uDirLen = sizeof(sDirectory) - 1;
		memcpy(sDirectory, lpFileName, uDirLen);
		sDirectory[uDirLen] = 0;
		sPattern = sSplit + 1;
	}
	else
	{
		strcpy(sDirectory, ".");
		sPattern = lpFileName;
	}
	if (!*sPattern)
		sPattern = "*";

	DIR* pDir = ::opendir(*sDirectory ? sDirectory : ".");
	if (!pDir)
	{
		tlastError = ErrnoToWinError(errno);
		return INVALID_HANDLE_VALUE;
	}

	auto* pState = new FindState;
	pState->pDir = pDir;
	strncpy(pState->sDirectory, *sDirectory ? sDirectory : ".", sizeof(pState->sDirectory) - 1);
	pState->sDirectory[sizeof(pState->sDirectory) - 1] = 0;
	strncpy(pState->sPattern, sPattern, sizeof(pState->sPattern) - 1);
	pState->sPattern[sizeof(pState->sPattern) - 1] = 0;

	char sPath[2048];
	while (dirent* pEntry = ::readdir(pDir))
	{
		if (!strcmp(pEntry->d_name, ".") || !strcmp(pEntry->d_name, ".."))
			continue;
		if (fnmatch(sPattern, pEntry->d_name, FNM_CASEFOLD) != 0)
			continue;
		snprintf(sPath, sizeof(sPath), "%s/%s", pState->sDirectory, pEntry->d_name);
		FillFindData(sPath, pEntry->d_name, lpFindFileData);
		return pState;
	}
	::closedir(pDir);
	delete pState;
	tlastError = ERROR_FILE_NOT_FOUND;
	return INVALID_HANDLE_VALUE;
}

BOOL FindNextFileA(HANDLE hFindFile, WIN32_FIND_DATAA* lpFindFileData)
{
	auto* pState = static_cast<FindState*>(hFindFile);
	if (!pState)
		return FALSE;
	char sPath[2048];
	while (dirent* pEntry = ::readdir(pState->pDir))
	{
		if (!strcmp(pEntry->d_name, ".") || !strcmp(pEntry->d_name, ".."))
			continue;
		if (fnmatch(pState->sPattern, pEntry->d_name, FNM_CASEFOLD) != 0)
			continue;
		snprintf(sPath, sizeof(sPath), "%s/%s", pState->sDirectory, pEntry->d_name);
		FillFindData(sPath, pEntry->d_name, lpFindFileData);
		return TRUE;
	}
	tlastError = ERROR_NO_MORE_FILES;
	return FALSE;
}

BOOL FindClose(HANDLE hFindFile)
{
	auto* pState = static_cast<FindState*>(hFindFile);
	if (!pState)
		return FALSE;
	::closedir(pState->pDir);
	delete pState;
	return TRUE;
}

BOOL CreateDirectoryA(const char* lpPathName, void* lpSecurityAttributes)
{
	if (::mkdir(lpPathName, 0755) != 0)
	{
		tlastError = ErrnoToWinError(errno);
		return FALSE;
	}
	return TRUE;
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

#include "BSCore/BSCore.h"
#include <atomic>

namespace
{
	std::mutex threadInitMutex;
	std::condition_variable threadInitChanged;
	int threadInitEpoch = (-2147483647 - 1);
}

void BSCore::InitThreadHeader(int* apGuard)
{
	std::unique_lock lock(threadInitMutex);
	std::atomic_ref<int> guard(*apGuard);
	while (guard.load(std::memory_order_acquire) == -1)
		threadInitChanged.wait(lock);
	if (!guard.load(std::memory_order_relaxed))
		guard.store(-1, std::memory_order_release);
	else
		iThreadInitEpochS = threadInitEpoch;
}

void BSCore::InitThreadFooter(int* apGuard)
{
	{
		std::lock_guard lock(threadInitMutex);
		threadInitEpoch = static_cast<int>(static_cast<unsigned int>(threadInitEpoch) + 1);
		std::atomic_ref<int>(*apGuard).store(threadInitEpoch, std::memory_order_release);
		iThreadInitEpochS = threadInitEpoch;
	}
	threadInitChanged.notify_all();
}

void BSCore::InitThreadAbort(int* apGuard)
{
	{
		std::lock_guard lock(threadInitMutex);
		std::atomic_ref<int>(*apGuard).store(0, std::memory_order_release);
	}
	threadInitChanged.notify_all();
}

extern "C" int _strnicmp(const char* apFirst, const char* apSecond, size_t auiCount)
{
	return strncasecmp(apFirst, apSecond, auiCount);
}

extern "C" int strcpy_s(char* apDest, size_t auiSize, const char* apSource)
{
	if (!apDest || !auiSize || !apSource)
	{
		if (apDest && auiSize)
			*apDest = 0;
		_invalid_parameter_noinfo();
		return EINVAL;
	}
	size_t uiLength = strlen(apSource);
	if (uiLength >= auiSize)
	{
		*apDest = 0;
		_invalid_parameter_noinfo();
		return ERANGE;
	}
	for (size_t i = 0; i <= uiLength; ++i)
		apDest[i] = apSource[i];
	return 0;
}

extern "C" int strcat_s(char* apDest, size_t auiSize, const char* apSource)
{
	if (!apDest || !auiSize || !apSource)
	{
		if (apDest && auiSize)
			*apDest = 0;
		_invalid_parameter_noinfo();
		return EINVAL;
	}
	size_t uiLength = strnlen(apDest, auiSize);
	size_t uiSourceLength = strlen(apSource);
	if (uiLength >= auiSize || uiSourceLength >= auiSize - uiLength)
	{
		*apDest = 0;
		_invalid_parameter_noinfo();
		return ERANGE;
	}
	for (size_t i = 0; i <= uiSourceLength; ++i)
		apDest[uiLength + i] = apSource[i];
	return 0;
}

extern "C" char* _getcwd(char* apBuffer, int aiSize)
{
	if (aiSize < 0 || (apBuffer && !aiSize))
	{
		_invalid_parameter_noinfo();
		errno = EINVAL;
		return nullptr;
	}
	return getcwd(apBuffer, static_cast<size_t>(aiSize));
}

static char MainModule;

HMODULE GetModuleHandleA(const char* lpModuleName)
{
	return lpModuleName ? nullptr : &MainModule;
}

DWORD GetModuleFileNameA(HMODULE hModule, char* lpFilename, DWORD nSize)
{
	if ((hModule && hModule != &MainModule) || !lpFilename || !nSize)
		return 0;
	ssize_t iLength = readlink("/proc/self/exe", lpFilename, nSize);
	if (iLength < 0)
		return 0;
	if (static_cast<DWORD>(iLength) == nSize)
	{
		lpFilename[nSize - 1] = 0;
		return nSize;
	}
	lpFilename[iLength] = 0;
	return static_cast<DWORD>(iLength);
}

extern "C" int _stat64i32(const char* apFilename, struct _stat64i32* apStat)
{
	if (!apFilename || !apStat)
	{
		errno = EINVAL;
		return -1;
	}
	struct stat kStat;
	if (stat(apFilename, &kStat))
		return -1;
	apStat->st_dev = static_cast<uint32_t>(kStat.st_dev);
	apStat->st_ino = static_cast<uint16_t>(kStat.st_ino);
	apStat->st_mode = static_cast<uint16_t>(kStat.st_mode);
	apStat->st_nlink = static_cast<int16_t>(kStat.st_nlink);
	apStat->st_uid = static_cast<int16_t>(kStat.st_uid);
	apStat->st_gid = static_cast<int16_t>(kStat.st_gid);
	apStat->st_rdev = static_cast<uint32_t>(kStat.st_rdev);
	apStat->st_size = static_cast<int32_t>(kStat.st_size);
	apStat->st_atime = kStat.st_atim.tv_sec;
	apStat->st_mtime = kStat.st_mtim.tv_sec;
	apStat->st_ctime = kStat.st_ctim.tv_sec;
	return 0;
}

void GetSystemInfo(SYSTEM_INFO* lpSystemInfo)
{
	long iProcessors = sysconf(_SC_NPROCESSORS_ONLN);
	long iPageSize = sysconf(_SC_PAGESIZE);
	DWORD uiProcessors = iProcessors > 0 ? static_cast<DWORD>(iProcessors) : 1;
	*lpSystemInfo = {};
	lpSystemInfo->wProcessorArchitecture = 9;
	lpSystemInfo->dwPageSize = iPageSize > 0 ? static_cast<DWORD>(iPageSize) : 4096;
	lpSystemInfo->lpMinimumApplicationAddress = reinterpret_cast<void*>(uintptr_t(0x10000));
	lpSystemInfo->lpMaximumApplicationAddress = reinterpret_cast<void*>((uintptr_t(1) << 47) - 1);
	lpSystemInfo->dwActiveProcessorMask = uiProcessors >= sizeof(uintptr_t) * 8 ? ~uintptr_t(0) : (uintptr_t(1) << uiProcessors) - 1;
	lpSystemInfo->dwNumberOfProcessors = uiProcessors;
	lpSystemInfo->dwProcessorType = 8664;
	lpSystemInfo->dwAllocationGranularity = 0x10000;
	lpSystemInfo->wProcessorLevel = 6;
}

void RaiseException(DWORD dwExceptionCode, DWORD, DWORD, const ULONG_PTR*)
{
	if (dwExceptionCode != 0x406D1388)
		std::abort();
}

extern "C" int _splitpath_s(const char* path, char* drive, size_t driveSize, char* dir, size_t dirSize, char* fname, size_t fnameSize, char* ext, size_t extSize)
{
	char* outputs[] = {drive, dir, fname, ext};
	size_t sizes[] = {driveSize, dirSize, fnameSize, extSize};
	const auto clear = [&]
	{
		for (unsigned int i = 0; i < 4; ++i)
			if (outputs[i] && sizes[i])
				outputs[i][0] = 0;
	};
	bool valid = path != nullptr;
	for (unsigned int i = 0; i < 4; ++i)
		valid = valid && ((outputs[i] == nullptr) == (sizes[i] == 0));
	if (!valid)
	{
		clear();
		_invalid_parameter_noinfo();
		errno = EINVAL;
		return EINVAL;
	}
	size_t driveLength = path[0] && path[1] == ':' ? 2 : 0;
	const char* start = path + driveLength;
	const char* file = start;
	for (const char* cursor = start; *cursor; ++cursor)
		if (*cursor == '/' || *cursor == '\\')
			file = cursor + 1;
	const char* end = file + std::strlen(file);
	const char* extension = std::strrchr(file, '.');
	if (!extension)
		extension = end;
	const char* components[] = {path, start, file, extension};
	size_t lengths[] = {driveLength, static_cast<size_t>(file - start), static_cast<size_t>(extension - file), static_cast<size_t>(end - extension)};
	for (unsigned int i = 0; i < 4; ++i)
	{
		if (outputs[i] && sizes[i] <= lengths[i])
		{
			clear();
			errno = ERANGE;
			return ERANGE;
		}
	}
	for (unsigned int i = 0; i < 4; ++i)
	{
		if (outputs[i])
		{
			std::memcpy(outputs[i], components[i], lengths[i]);
			outputs[i][lengths[i]] = 0;
		}
	}
	return 0;
}

extern "C" int strncpy_s(char* apDest, size_t auiSize, const char* apSource, size_t auiCount)
{
	if (!apDest || !auiSize || !apSource)
	{
		if (apDest && auiSize)
			*apDest = 0;
		_invalid_parameter_noinfo();
		errno = EINVAL;
		return EINVAL;
	}
	bool bTruncate = auiCount == SIZE_MAX;
	size_t uiLimit = bTruncate ? auiSize - 1 : (auiCount < auiSize ? auiCount : auiSize);
	size_t uiLength = 0;
	while (uiLength < uiLimit && apSource[uiLength])
		++uiLength;
	if (!bTruncate && uiLength == auiSize)
	{
		*apDest = 0;
		_invalid_parameter_noinfo();
		errno = ERANGE;
		return ERANGE;
	}
	for (size_t i = 0; i < uiLength; ++i)
		apDest[i] = apSource[i];
	apDest[uiLength] = 0;
	return bTruncate && apSource[uiLength] ? 80 : 0;
}
