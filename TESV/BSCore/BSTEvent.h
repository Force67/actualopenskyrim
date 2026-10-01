#pragma once

#include "BSCore/BSTArray.h"
#include "BSCore/BSSpinLock.h"

namespace BSEvent
{
	enum NotifyControl : int
	{
		kContinue,
		kStop,
	};
}

template <class Event>
class BSTEventSource;

template <class Event>
class BSTEventSink
{
public:
	virtual ~BSTEventSink() = default;
	virtual BSEvent::NotifyControl ProcessEvent(const Event& arEvent, BSTEventSource<Event>* apSource) = 0;
};

template <class Event>
class BSTEventSource
{
public:
	~BSTEventSource();
	void Notify(const Event& arEvent);
	void RegisterSink(BSTEventSink<Event>* apSink);
	void UnregisterSink(BSTEventSink<Event>* apSink);

	BSTArray<BSTEventSink<Event>*> pSinksA;
	BSTArray<BSTEventSink<Event>*> pPendingRegistersA;
	BSTArray<BSTEventSink<Event>*> pPendingUnregistersA;
	BSSpinLock mLock;
	unsigned char cNotifying;
};
