#include "Gamebryo/CoreLibs/NiMain/NiRefObject.h"

volatile unsigned int NiRefObject::ms_uiObjects = 0;

NiRefObject::~NiRefObject()
{
	InterlockedDecrement(reinterpret_cast<volatile LONG*>(&ms_uiObjects));
}

void NiRefObject::DeleteThis()
{
	delete this;
}
