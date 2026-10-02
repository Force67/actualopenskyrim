#include "BSCore/BSCoreMessage.h"

#include <cstring>
#include <windows.h>

thread_local constinit BSCoreMessage::ContextType BSCoreMessage::eMessageContextS;
thread_local constinit WARNING_TYPES BSCoreMessage::eWarningContextS;
bool BSCoreMessage::bMessageContextDisabled[CONTEXT_COUNT];
bool BSCoreMessage::bWarningsDisabled[MAX_WARNING_TYPES];
const char* BSCoreMessage::WarningContextStrings[MAX_WARNING_TYPES] = {
	"DEFAULT", "COMBAT", "ANIMATION", "AI", "SCRIPTS", "SAVELOAD", "DIALOGUE", "QUESTS",
	"PACKAGES", "EDITOR", "MODELS", "TEXTURES", "PLUGINS", "MASTERFILE", "FORMS", "MAGIC",
	"SHADERS", "RENDERING", "PATHFINDING", "MENUS", "AUDIO", "CELLS", "HAVOK", "FACEGEN",
	"WATER", "INGAME", "MEMORY", "PERFORMANCE", "JOBS", "SYSTEM", "BNET",
};

void BSCoreMessage::SetWarningDisabled(WARNING_TYPES aeType, bool abDisabled)
{
	bWarningsDisabled[aeType] = abDisabled;
}

void BSCoreMessage::ClearAllWarningContexts()
{
	std::memset(bWarningsDisabled, 0, sizeof(bWarningsDisabled));
}

WARNING_TYPES BSCoreMessage::GetWarningContextFromName(const char* apName)
{
	for (int i = 0; i < MAX_WARNING_TYPES; ++i)
	{
		if (!_stricmp(WarningContextStrings[i], apName))
			return static_cast<WARNING_TYPES>(i);
	}
	return MAX_WARNING_TYPES;
}

bool BSCoreMessage::IsWarningDisabled(WARNING_TYPES aeType)
{
	return bWarningsDisabled[aeType];
}

bool BSCoreMessage::CheckWarningContexts(WARNING_TYPES aeType)
{
	return bWarningsDisabled[aeType];
}

void BSCoreMessage::SetMessageContextDisabled(ContextType aeType, bool abDisabled)
{
	bMessageContextDisabled[aeType] = abDisabled;
}

bool BSCoreMessage::IsMessageContextDisabled(ContextType aeType)
{
	return bMessageContextDisabled[aeType];
}
