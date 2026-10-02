#include "BSCore/MemoryManager.h"
#include "BSCore/BSCore.h"

#include <atomic>
#include <cstdlib>
#include <new>
#include "BSCore/BSAutoLock.h"

template <>
BSTEventSource<MemoryManagement::PMPEvent>::~BSTEventSource()
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
void BSTEventSource<MemoryManagement::PMPEvent>::Notify(const MemoryManagement::PMPEvent& arEvent)
{
	BSAutoLock<BSSpinLock> kAutoLock(mLock);
	const unsigned char cPrevious = cNotifying;
	cNotifying = 1;
	if (!cPrevious && pPendingRegistersA.QSize())
	{
		for (auto pIt = pPendingRegistersA.Begin(), pEnd = pPendingRegistersA.End(); pIt != pEnd; ++pIt)
		{
			const auto& pSink = *pIt;
			if (FindSink(pSinksA, pSink) == 0xFFFFFFFF)
				pSinksA.Add(pSink);
		}
		pPendingRegistersA.Clear(false);
	}
	BSEvent::NotifyControl eControl = BSEvent::kContinue;
	for (auto pIt = pSinksA.Begin(), pEnd = pSinksA.End(); pIt != pEnd; ++pIt)
	{
		if (eControl != BSEvent::kContinue)
			break;
		auto* pSink = *pIt;
		if (FindSink(pPendingUnregistersA, pSink) == 0xFFFFFFFF)
			eControl = pSink->ProcessEvent(arEvent, this);
	}
	cNotifying = cPrevious;
	if (!cPrevious && pPendingUnregistersA.QSize())
	{
		for (auto pIt = pPendingUnregistersA.Begin(), pEnd = pPendingUnregistersA.End(); pIt != pEnd; ++pIt)
		{
			const unsigned int uiIndex = FindSink(pSinksA, *pIt);
			if (uiIndex != 0xFFFFFFFF)
				pSinksA.RemoveFast(uiIndex);
		}
		pPendingUnregistersA.Clear(false);
	}
}

template <>
void BSTEventSource<MemoryManagement::PMPEvent>::RegisterSink(BSTEventSink<MemoryManagement::PMPEvent>* const apSink)
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
void BSTEventSource<MemoryManagement::PMPEvent>::UnregisterSink(BSTEventSink<MemoryManagement::PMPEvent>* const apSink)
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

alignas(8) unsigned char MemoryManagement::PMPEventSource::cSourceBufferS[88];
int MemoryManagement::PMPEventSource::iSourceInitS;

template <>
MemoryManagement::PMPEventSource* BSTSingletonImplicit<MemoryManagement::PMPEventSource>::QInstance()
{
	using MemoryManagement::PMPEventSource;
	if (std::atomic_ref<int>(PMPEventSource::iSourceInitS).load(std::memory_order_acquire) > BSCore::iThreadInitEpochS)
	{
		BSCore::InitThreadHeader(&PMPEventSource::iSourceInitS);
		if (PMPEventSource::iSourceInitS == -1)
		{
			try
			{
				auto* pSource = new (PMPEventSource::cSourceBufferS) PMPEventSource;
				pSource->cNotifying = 0;
				std::atexit(PMPEventSource::DestroySingleton);
				BSCore::InitThreadFooter(&PMPEventSource::iSourceInitS);
			}
			catch (...)
			{
				BSCore::InitThreadAbort(&PMPEventSource::iSourceInitS);
				throw;
			}
		}
	}
	return reinterpret_cast<PMPEventSource*>(PMPEventSource::cSourceBufferS);
}

void MemoryManagement::PMPEventSource::DestroySingleton()
{
	reinterpret_cast<PMPEventSource*>(cSourceBufferS)->~PMPEventSource();
}
