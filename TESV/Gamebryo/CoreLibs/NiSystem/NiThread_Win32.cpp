#include "NiThread.h"
#include "NiThreadProcedure.h"

#include <cstddef>

bool NiThread::SystemSetPriority(Priority ePriority)
{
	static_assert(offsetof(NiThread, m_ePriority) == 32);
	static_assert(offsetof(NiThread, m_eStatus) == 36);
	static_assert(offsetof(NiThread, m_hThread) == 48);
	if (m_ePriority == ePriority)
		return true;
	int iPriority;
	switch (ePriority)
	{
	case IDLE: iPriority = -15; break;
	case LOWEST: iPriority = -2; break;
	case BELOW_NORMAL: iPriority = -1; break;
	case NORMAL: iPriority = 0; break;
	case ABOVE_NORMAL: iPriority = 1; break;
	case HIGHEST: iPriority = 2; break;
	case TIME_CRITICAL: iPriority = 15; break;
	default: return false;
	}
	if (!SetThreadPriority(m_hThread, iPriority))
		return false;
	m_ePriority = ePriority;
	return true;
}

int NiThread::SystemResume()
{
	if (!m_hThread)
		return -1;
	DWORD uiPrevious = ResumeThread(m_hThread);
	if (uiPrevious != 0xffffffff)
		m_eStatus = uiPrevious > 1 ? SUSPENDED : RUNNING;
	return static_cast<int>(uiPrevious);
}

bool NiThread::SystemWaitForCompletion()
{
	if (m_eStatus != RUNNING)
		return false;
	WaitForSingleObject(m_hThread, 0xffffffff);
	return true;
}

bool NiThread::SystemCreateThread()
{
	static_assert(offsetof(NiThread, m_uiThreadID) == 8);
	static_assert(offsetof(NiThread, m_kAffinity) == 12);
	static_assert(offsetof(NiThread, m_uiStackSize) == 20);
	static_assert(offsetof(NiThread, m_pkProcedure) == 24);
	static_assert(offsetof(NiThread, m_uiReturnValue) == 40);
	static_assert(offsetof(NiThread, m_pcName) == 56);
	if (!m_pkProcedure)
		return false;
	m_hThread = CreateThread(nullptr, m_uiStackSize, ThreadProc, this, 4, &m_uiThreadID);
	if (!m_hThread)
		return false;
	m_ePriority = NORMAL;
	m_eStatus = SUSPENDED;
	return true;
}

DWORD WINAPI NiThread::ThreadProc(void* pvArg)
{
	NiThread* pkThread = static_cast<NiThread*>(pvArg);
	pkThread->m_uiReturnValue = pkThread->m_pkProcedure->ThreadProcedure(pkThread);
	pkThread->m_eStatus = COMPLETE;
	return pkThread->m_uiReturnValue;
}

bool NiThread::SystemSetAffinity(const NiProcessorAffinity& kAffinity)
{
	static_assert(offsetof(NiProcessorAffinity, m_uiAffinityMask) == 4);
	return SetThreadAffinityMask(m_hThread, kAffinity.m_uiAffinityMask) != 0;
}

int NiThread::SystemSuspend()
{
	if (!m_hThread)
		return -1;
	DWORD uiPrevious = SuspendThread(m_hThread);
	if (uiPrevious != 0xffffffff)
		m_eStatus = SUSPENDED;
	return static_cast<int>(uiPrevious);
}

void SetThreadName(DWORD uiThreadID, const char* const pcName)
{
	struct ThreadNameInfo
	{
		DWORD uiType;
		const char* pcName;
		DWORD uiThreadID;
		DWORD uiFlags;
	};
	ThreadNameInfo kInfo;
	kInfo.uiType = 0x1000;
	kInfo.pcName = pcName;
	kInfo.uiThreadID = uiThreadID;
	kInfo.uiFlags = 0;
	static_assert(sizeof(kInfo) == 24);
	__try
	{
		RaiseException(0x406D1388, 0, sizeof(kInfo) / sizeof(DWORD), reinterpret_cast<const ULONG_PTR*>(&kInfo));
	}
	__except(EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}
