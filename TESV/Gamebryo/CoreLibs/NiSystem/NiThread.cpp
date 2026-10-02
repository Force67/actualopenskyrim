#include "NiThread.h"
#include "NiMemoryDefines.h"
#include "BSCore/MemoryManager.h"

#include <new>

bool NiThread::SetPriority(Priority ePriority)
{
	return SystemSetPriority(ePriority);
}

int NiThread::Resume()
{
	return SystemResume();
}

bool NiThread::WaitForCompletion()
{
	return SystemWaitForCompletion();
}

NiThread::~NiThread()
{
	SystemWaitForCompletion();
	m_pkProcedure = nullptr;
	if (m_hThread)
		CloseHandle(m_hThread);
	m_hThread = nullptr;
	m_uiThreadID = 0;
	_NiFree(m_pcName);
}

NiThread* NiThread::Create(NiThreadProcedure* pkProcedure, unsigned int uiStackSize)
{
	NiThread* pkThread = static_cast<NiThread*>(MemoryManager::Instance().Allocate(sizeof(NiThread), 0, false));
	if (pkThread)
		new (pkThread) NiThread(pkProcedure, uiStackSize);
	if (!pkThread || pkThread->SystemCreateThread())
		return pkThread;
	delete pkThread;
	return nullptr;
}

int NiThread::Suspend()
{
	return SystemSuspend();
}
