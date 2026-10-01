#include "BSCore/BSThread.h"
#include "BSCore/MemoryContextTracker.h"

unsigned int WINAPI cThreadProc(BSThread* apThread)
{
	return apThread->CallThreadProc();
}

void BSThread::Exit(size_t auiReturnCode)
{
	ExitThread(static_cast<DWORD>(auiReturnCode));
}

void BSThread::SetName(const char*)
{
}

void BSThread::SetThreadProcessor(unsigned int)
{
}

BSThread::BSThread()
{
	InitializeCriticalSection(&CriticalSection);
	m_ThreadHandle = nullptr;
	m_ParentHandle = nullptr;
	m_ThreadID = 0;
	m_ParentID = 0;
	bThreadIsActive = false;
}

BSThread::~BSThread()
{
	Close();
	DeleteCriticalSection(&CriticalSection);
}

void BSThread::Close()
{
	if (!m_ThreadHandle)
		return;
	bThreadIsActive = false;
	EnterCriticalSection(&CriticalSection);
	if (GetCurrentThreadId() != m_ThreadID)
	{
		while (WaitForSingleObject(m_ThreadHandle, 10) == WAIT_TIMEOUT)
			OnWait();
	}
	CloseHandle(m_ThreadHandle);
	m_ThreadHandle = nullptr;
	LeaveCriticalSection(&CriticalSection);
}

bool BSThread::Initialize(StackSize aeStackSize, const char*)
{
	AutoMemContext context(static_cast<MEM_CONTEXT>(16));
	const bool bNewThread = m_ThreadHandle == nullptr;
	if (bNewThread)
	{
		m_ParentHandle = GetCurrentThread();
		m_ParentID = GetCurrentThreadId();
		m_ThreadHandle = CreateThread(nullptr, static_cast<size_t>(aeStackSize),
			reinterpret_cast<LPTHREAD_START_ROUTINE>(cThreadProc), this, 0, reinterpret_cast<DWORD*>(&m_ThreadID));
		EnterCriticalSection(&CriticalSection);
		if (m_ThreadHandle)
			::SetThreadPriority(m_ThreadHandle, -1);
		bThreadIsActive = true;
		ResumeThread(m_ThreadHandle);
		LeaveCriticalSection(&CriticalSection);
	}
	return bNewThread;
}

void BSThread::SetThreadPriority(int aiPriority)
{
	if (m_ThreadHandle)
		::SetThreadPriority(m_ThreadHandle, aiPriority);
}
