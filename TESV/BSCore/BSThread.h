#pragma once

#include "BSCore/BSTEvent.h"

#include <windows.h>

#include <cstddef>

class BSThread
{
public:
	enum StackSize : int
	{
		SS_16K = 0x4000,
		SS_32K = 0x8000,
		SS_64K = 0x10000,
		SS_128K = 0x20000,
		SS_256K = 0x40000,
		SS_512K = 0x80000,
		SS_1024K = 0x100000,
		INHERIT_MAIN = 0,
	};
	BSThread();
	virtual ~BSThread();
	virtual unsigned int ThreadProc();
	virtual void OnWait();
	bool Initialize(StackSize aeStackSize, const char* apName = nullptr);
	void SetThreadPriority(int aiPriority);
	void Close();
	unsigned int CallThreadProc();
	[[noreturn]] void Exit(size_t auiReturnCode);
	void SetName(const char* apName);
	void SetThreadProcessor(unsigned int auiProcessor);

	CRITICAL_SECTION CriticalSection;
	HANDLE m_ThreadHandle;
	HANDLE m_ParentHandle;
	unsigned int m_ThreadID;
	unsigned int m_ParentID;
	volatile bool bThreadIsActive;
};
static_assert(sizeof(BSThread) == 0x50);
static_assert(offsetof(BSThread, CriticalSection) == 0x8);
static_assert(offsetof(BSThread, m_ThreadHandle) == 0x30);
static_assert(offsetof(BSThread, m_ThreadID) == 0x40);
static_assert(offsetof(BSThread, bThreadIsActive) == 0x48);

class BSStepThread : public BSThread
{
public:
	~BSStepThread() override;
	unsigned int ThreadProc() override;
	virtual void StepProc();

	HANDLE hStep;
	HANDLE hStopped;
	volatile bool bStop;
	volatile bool bQuit;
};
static_assert(sizeof(BSStepThread) == 0x68);
static_assert(offsetof(BSStepThread, hStep) == 0x50);
static_assert(offsetof(BSStepThread, hStopped) == 0x58);
static_assert(offsetof(BSStepThread, bStop) == 0x60);
static_assert(offsetof(BSStepThread, bQuit) == 0x61);

namespace BSThreadEvent
{
	enum Event
	{
		BSTE_ON_STARTUP,
		BSTE_ON_SHUTDOWN,
	};
	struct ThreadEvent
	{
		const BSThread* pThread;
		const Event eInstance;
	};
	static_assert(sizeof(ThreadEvent) == 0x10);
	static_assert(offsetof(ThreadEvent, eInstance) == 8);

	class Source : public BSTEventSource<ThreadEvent>
	{
	public:
		Source() { cNotifying = 0; }
		virtual ~Source();
		unsigned char cSingletonPadding[8];
	};
	static_assert(sizeof(BSTEventSource<ThreadEvent>) == 0x58);
	static_assert(offsetof(BSTEventSource<ThreadEvent>, pSinksA) == 0);
	static_assert(offsetof(BSTEventSource<ThreadEvent>, mLock) == 0x48);
	static_assert(offsetof(BSTEventSource<ThreadEvent>, cNotifying) == 0x50);
	static_assert(sizeof(Source) == 0x68);
	extern Source* pSourceS;
	alignas(Source) extern unsigned char cSourceBufferS[sizeof(Source)];

	void InitSDM();
	void KillSDM();
	void RegisterSink(BSTEventSink<ThreadEvent>* apSink);
	void UnregisterSink(BSTEventSink<ThreadEvent>* apSink);
}

unsigned int WINAPI cThreadProc(BSThread* apThread);
