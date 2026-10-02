#pragma once

#include "NiProcessorAffinity.h"

#include <cstdint>
#include <windows.h>

class NiThreadProcedure;

class NiThread
{
	friend class NiStream;
public:
	enum Priority
	{
		IDLE, LOWEST, BELOW_NORMAL, NORMAL, ABOVE_NORMAL, HIGHEST, TIME_CRITICAL, NUM_PRIORITIES
	};
	enum Status
	{
		RUNNING, SUSPENDED, COMPLETE
	};

	virtual ~NiThread();
	static NiThread* Create(NiThreadProcedure* pkProcedure, unsigned int uiStackSize = 0xffffffff);
	bool SetPriority(Priority ePriority);
	int Suspend();
	int Resume();
	bool WaitForCompletion();

protected:
	NiThread(NiThreadProcedure* pkProcedure, unsigned int uiStackSize) :
		m_uiThreadID(0), m_kAffinity{0xffffffff, 0xffffffff},
		m_uiStackSize(uiStackSize == 0xffffffff ? 0 : uiStackSize), m_pkProcedure(pkProcedure),
		m_ePriority(IDLE), m_eStatus(SUSPENDED), m_uiReturnValue(0xffffffff),
		m_hThread(nullptr), m_pcName(nullptr)
	{
	}
	bool SystemCreateThread();
	bool SystemSetPriority(Priority ePriority);
	bool SystemSetAffinity(const NiProcessorAffinity& kAffinity);
	int SystemSuspend();
	int SystemResume();
	bool SystemWaitForCompletion();
	static DWORD WINAPI ThreadProc(void* pvArg);

	DWORD m_uiThreadID;
	NiProcessorAffinity m_kAffinity;
	unsigned int m_uiStackSize;
	NiThreadProcedure* m_pkProcedure;
	Priority m_ePriority;
	volatile Status m_eStatus;
	volatile unsigned int m_uiReturnValue;
	HANDLE m_hThread;
	char* m_pcName;
};
static_assert(sizeof(NiThread) == 64);

void SetThreadName(DWORD uiThreadID, const char* const pcName);
