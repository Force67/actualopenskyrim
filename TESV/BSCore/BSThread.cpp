#include "BSCore/BSThread.h"
#include "BSCore/BSAutoLock.h"

#include <new>

BSThreadEvent::Source* BSThreadEvent::pSourceS = nullptr;
alignas(BSThreadEvent::Source) unsigned char BSThreadEvent::cSourceBufferS[sizeof(Source)];

BSThreadEvent::Source::~Source()
{
	pSourceS = nullptr;
}

void BSThreadEvent::InitSDM()
{
	if (!pSourceS)
		pSourceS = new (cSourceBufferS) Source;
}

void BSThreadEvent::KillSDM()
{
	if (pSourceS)
	{
		pSourceS->~Source();
		pSourceS = nullptr;
	}
}

void BSThreadEvent::RegisterSink(BSTEventSink<ThreadEvent>* apSink)
{
	if (pSourceS)
		pSourceS->RegisterSink(apSink);
}

void BSThreadEvent::UnregisterSink(BSTEventSink<ThreadEvent>* apSink)
{
	if (pSourceS)
		pSourceS->UnregisterSink(apSink);
}

unsigned int BSThread::CallThreadProc()
{
	using namespace BSThreadEvent;
	if (pSourceS)
	{
		const ThreadEvent kEvent{this, BSTE_ON_STARTUP};
		pSourceS->Notify(kEvent);
	}
	const unsigned int uiResult = ThreadProc();
	if (pSourceS)
	{
		const ThreadEvent kEvent{this, BSTE_ON_SHUTDOWN};
		pSourceS->Notify(kEvent);
	}
	return uiResult;
}

unsigned int BSThread::ThreadProc()
{
	return 0;
}

void BSThread::OnWait()
{
}

BSStepThread::~BSStepThread()
{
	CloseHandle(hStopped);
	CloseHandle(hStep);
}

void BSStepThread::StepProc()
{
}

unsigned int BSStepThread::ThreadProc()
{
	while (!bQuit)
	{
		if (bStop)
		{
			bThreadIsActive = false;
			ReleaseSemaphore(hStopped, 1, nullptr);
			WaitForSingleObject(hStep, INFINITE);
			bStop = false;
			bThreadIsActive = true;
		}
		StepProc();
	}
	return 0;
}
