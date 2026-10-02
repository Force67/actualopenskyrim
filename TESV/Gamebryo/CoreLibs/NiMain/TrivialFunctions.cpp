#include "Gamebryo/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo/CoreLibs/NiMain/NiStream.h"

bool NiObject::RegisterStreamables(NiStream& kStream)
{
	return kStream.RegisterSaveObject(this);
}
