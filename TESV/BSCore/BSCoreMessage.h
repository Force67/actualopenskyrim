#pragma once

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
	enum ContextType : int
	{
		CT_DEFAULT,
		CT_NETWORK,
		CT_UI,
		CONTEXT_COUNT,
	};
	static bool IsWarningDisabled(WARNING_TYPES aeType);
	bool CheckWarningContexts(WARNING_TYPES aeType);
	static void SetMessageContextDisabled(ContextType aeType, bool abDisabled);
	static bool IsMessageContextDisabled(ContextType aeType);
	static void SetWarningDisabled(WARNING_TYPES aeType, bool abDisabled);
	void ClearAllWarningContexts();
	static WARNING_TYPES GetWarningContextFromName(const char* apName);

	static bool bMessageContextDisabled[CONTEXT_COUNT];
	static bool bWarningsDisabled[MAX_WARNING_TYPES];
	static const char* WarningContextStrings[MAX_WARNING_TYPES];
};
