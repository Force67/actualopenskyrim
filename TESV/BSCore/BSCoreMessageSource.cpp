#include "BSCore/BSCoreMessage.h"
#include "BSCore/BSCore.h"
#include "BSCore/BSAutoLock.h"

#include <atomic>
#include <cstdlib>
#include <new>

alignas(BSCoreMessage::MessageSource) unsigned char BSCoreMessage::MessageSource::cSourceBufferS[sizeof(BSTEventSource<Event>)];
int BSCoreMessage::MessageSource::iSourceInitS = 0;

BSCoreMessage::MessageSource& BSCoreMessage::MessageSource::QInstance()
{
	if (std::atomic_ref<int>(iSourceInitS).load(std::memory_order_acquire) > BSCore::iThreadInitEpochS)
	{
		BSCore::InitThreadHeader(&iSourceInitS);
		if (iSourceInitS == -1)
		{
			try
			{
				auto* pSource = new (cSourceBufferS) MessageSource;
				pSource->cNotifying = 0;
				std::atexit(DestroySingleton);
				BSCore::InitThreadFooter(&iSourceInitS);
			}
			catch (...)
			{
				BSCore::InitThreadAbort(&iSourceInitS);
				throw;
			}
		}
	}
	return *reinterpret_cast<MessageSource*>(cSourceBufferS);
}

void BSCoreMessage::MessageSource::DestroySingleton()
{
	reinterpret_cast<MessageSource*>(cSourceBufferS)->~MessageSource();
}

template <>
BSTEventSource<BSCoreMessage::Event>::~BSTEventSource()
{
}

namespace
{
	template <class T>
	unsigned int FindSink(const BSTArray<T>& arArray, T apSink)
	{
		for (unsigned int ui = 0; ui < arArray.QSize(); ++ui)
			if (arArray[ui] == apSink)
				return ui;
		return 0xFFFFFFFF;
	}
}

template <>
void BSTEventSource<BSCoreMessage::Event>::RegisterSink(BSTEventSink<BSCoreMessage::Event>* const apSink)
{
	if (!apSink)
		return;
	BSAutoLock<BSSpinLock> kAutoLock(mLock);
	auto& rSinks = cNotifying ? pPendingRegistersA : pSinksA;
	if (FindSink(rSinks, apSink) == 0xFFFFFFFF)
		rSinks.Add(apSink);
	const unsigned int uiIndex = FindSink(pPendingUnregistersA, apSink);
	if (uiIndex != 0xFFFFFFFF)
		pPendingUnregistersA.RemoveFast(uiIndex);
}

template <>
void BSTEventSource<BSCoreMessage::Event>::UnregisterSink(BSTEventSink<BSCoreMessage::Event>* const apSink)
{
	if (!apSink)
		return;
	BSAutoLock<BSSpinLock> kAutoLock(mLock);
	if (cNotifying)
	{
		if (FindSink(pPendingUnregistersA, apSink) == 0xFFFFFFFF)
			pPendingUnregistersA.Add(apSink);
	}
	else
	{
		const unsigned int uiIndex = FindSink(pSinksA, apSink);
		if (uiIndex != 0xFFFFFFFF)
			pSinksA.RemoveFast(uiIndex);
	}
	const unsigned int uiIndex = FindSink(pPendingRegistersA, apSink);
	if (uiIndex != 0xFFFFFFFF)
		pPendingRegistersA.RemoveFast(uiIndex);
}

void BSCoreMessage::RegisterSink(BSTEventSink<Event>* apSink)
{
	MessageSource::QInstance().RegisterSink(apSink);
}

void BSCoreMessage::UnregisterSink(BSTEventSink<Event>* apSink)
{
	MessageSource::QInstance().UnregisterSink(apSink);
}
