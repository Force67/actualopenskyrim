#include "BSCore/BSCoreMessage.h"
#include "BSCore/BSCore.h"

#include <atomic>
#include <cstdlib>

constinit BSCoreMessage::NullHandlerStorage BSCoreMessage::kNullHandlerS;
int BSCoreMessage::iNullHandlerInitS;
BSCoreMessage* BSCoreMessage::pHandler;

BSCoreMessage::BSCoreMessage() {}
BSCoreMessage::~BSCoreMessage() {}

void BSCoreMessage::DebugString(const char*, va_list&) {}
void BSCoreMessage::Error(const char*, va_list&) {}
int BSCoreMessage::Warning(WARNING_TYPES, const char*, va_list&) { return 0; }
BSCoreMessage::MsgReturn BSCoreMessage::Message(MsgStyle, const char*, const char*, unsigned int, unsigned int)
{
	return static_cast<MsgReturn>(0);
}

BSCoreMessage& BSCoreMessage::QInstance()
{
	if (pHandler)
		return *pHandler;
	if (std::atomic_ref<int>(iNullHandlerInitS).load(std::memory_order_acquire) > BSCore::iThreadInitEpochS)
	{
		BSCore::InitThreadHeader(&iNullHandlerInitS);
		if (iNullHandlerInitS == -1)
		{
			std::atexit(DestroyNullHandler);
			BSCore::InitThreadFooter(&iNullHandlerInitS);
		}
	}
	return kNullHandlerS.Handler;
}

void BSCoreMessage::DestroyNullHandler()
{
	kNullHandlerS.Handler.BSCoreMessage::~BSCoreMessage();
}

void BSCoreMessage::Assert(const char*, unsigned int, const char*, ...) {}
void BSCoreMessage::Assert_S(const char*, unsigned int, const char*, ...) { QInstance(); }

void BSCoreMessage::DebugString_S(const char* apMessage, ...)
{
	va_list kList;
	va_start(kList, apMessage);
	QInstance().DebugString(apMessage, kList);
	va_end(kList);
}

void BSCoreMessage::DebugString_S(ContextType aeContext, const char* apMessage, ...)
{
	va_list kList;
	va_start(kList, apMessage);
	const ContextType ePrevious = eMessageContextS;
	eMessageContextS = aeContext;
	QInstance().DebugString(apMessage, kList);
	eMessageContextS = ePrevious;
	va_end(kList);
}

void BSCoreMessage::Error_S(const char* apMessage, ...)
{
	va_list kList;
	va_start(kList, apMessage);
	QInstance().Error(apMessage, kList);
	va_end(kList);
}

int BSCoreMessage::Warning_S(const char* apMessage, ...)
{
	va_list kList;
	va_start(kList, apMessage);
	const int iResult = QInstance().Warning(eWarningContextS, apMessage, kList);
	va_end(kList);
	return iResult;
}

int BSCoreMessage::Warning_S(WARNING_TYPES aeContext, const char* apMessage, ...)
{
	va_list kList;
	va_start(kList, apMessage);
	const int iResult = QInstance().Warning(aeContext, apMessage, kList);
	va_end(kList);
	return iResult;
}
