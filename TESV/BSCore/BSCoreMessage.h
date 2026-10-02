#pragma once

#include "BSCore/BSTEvent.h"

#include <cstdarg>

enum WARNING_TYPES : int
{
	WARN_DEFAULT,
	WARN_COMBAT,
	WARN_ANIMATION,
	WARN_AI,
	WARN_SCRIPTS,
	WARN_SAVELOAD,
	WARN_DIALOGUE,
	WARN_QUESTS,
	WARN_PACKAGES,
	WARN_EDITOR,
	WARN_MODELS,
	WARN_TEXTURES,
	WARN_PLUGINS,
	WARN_MASTERFILE,
	WARN_FORMS,
	WARN_MAGIC,
	WARN_SHADERS,
	WARN_RENDERING,
	WARN_PATHFINDING,
	WARN_MENUS,
	WARN_AUDIO,
	WARN_CELLS,
	WARN_HAVOK,
	WARN_FACEGEN,
	WARN_WATER,
	WARN_INGAME_MESSAGE,
	WARN_MEMORY,
	WARN_PERFORMANCE,
	WARN_JOBS,
	WARN_SYSTEM,
	WARN_BNET,
	MAX_WARNING_TYPES,
};

class BSCoreMessage
{
public:
	virtual ~BSCoreMessage();
	virtual void DebugString(const char* apMessage, va_list& arList);
	virtual void Error(const char* apMessage, va_list& arList);
	virtual int Warning(WARNING_TYPES aeContext, const char* apMessage, va_list& arList);
	enum MsgStyle : int;
	enum MsgReturn : int;
	virtual MsgReturn Message(MsgStyle aeStyle, const char* apMessage, const char* apCaption, unsigned int auiButtons, unsigned int auiDefault);
	void Assert(const char* apFileName, unsigned int auiLine, const char* apMessage, ...);
	static void Assert_S(const char* apFileName, unsigned int auiLine, const char* apMessage, ...);
	static void DebugString_S(const char* apMessage, ...);
	static void Error_S(const char* apMessage, ...);
	static int Warning_S(const char* apMessage, ...);
	static int Warning_S(WARNING_TYPES aeContext, const char* apMessage, ...);
	static BSCoreMessage& QInstance();
	static void DestroyNullHandler();
	union NullHandlerStorage;
	static NullHandlerStorage kNullHandlerS;
	static int iNullHandlerInitS;
	static BSCoreMessage* pHandler;
	struct Event;
	class MessageSource;
	static void RegisterSink(BSTEventSink<Event>* apSink);
	static void UnregisterSink(BSTEventSink<Event>* apSink);

	enum ContextType : int
	{
		CT_DEFAULT,
		CT_NETWORK,
		CT_UI,
		CONTEXT_COUNT,
	};
	static void DebugString_S(ContextType aeContext, const char* apMessage, ...);
	static bool IsWarningDisabled(WARNING_TYPES aeType);
	bool CheckWarningContexts(WARNING_TYPES aeType);
	static void SetMessageContextDisabled(ContextType aeType, bool abDisabled);
	static bool IsMessageContextDisabled(ContextType aeType);
	static void SetWarningDisabled(WARNING_TYPES aeType, bool abDisabled);
	void ClearAllWarningContexts();
	static WARNING_TYPES GetWarningContextFromName(const char* apName);

	static thread_local constinit ContextType eMessageContextS;
	static thread_local constinit WARNING_TYPES eWarningContextS;
	static bool bMessageContextDisabled[CONTEXT_COUNT];
	static bool bWarningsDisabled[MAX_WARNING_TYPES];
	static const char* WarningContextStrings[MAX_WARNING_TYPES];

protected:
	BSCoreMessage();

private:
	struct NullHandlerTag {};
	constexpr BSCoreMessage(NullHandlerTag) {}
};

union BSCoreMessage::NullHandlerStorage
{
	BSCoreMessage Handler;
	constexpr NullHandlerStorage() : Handler(NullHandlerTag{}) {}
	~NullHandlerStorage() {}
};
static_assert(sizeof(BSCoreMessage) == 8);
static_assert(sizeof(BSCoreMessage::NullHandlerStorage) == 8);

class BSCoreMessage::MessageSource : public BSTEventSource<Event>
{
public:
	static MessageSource& QInstance();
	static void DestroySingleton();
	alignas(8) static unsigned char cSourceBufferS[sizeof(BSTEventSource<Event>)];
	static int iSourceInitS;
};
static_assert(sizeof(BSCoreMessage::MessageSource) == 0x58);
static_assert(offsetof(BSCoreMessage::MessageSource, mLock) == 0x48);
static_assert(offsetof(BSCoreMessage::MessageSource, cNotifying) == 0x50);

template <> BSTEventSource<BSCoreMessage::Event>::~BSTEventSource();
template <> void BSTEventSource<BSCoreMessage::Event>::RegisterSink(BSTEventSink<BSCoreMessage::Event>* apSink);
template <> void BSTEventSource<BSCoreMessage::Event>::UnregisterSink(BSTEventSink<BSCoreMessage::Event>* apSink);
