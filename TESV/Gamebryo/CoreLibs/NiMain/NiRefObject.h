#pragma once

// Base of reference counted objects. The last DecRefCount deletes the object
// through DeleteThis.
class NiRefObject
{
public:
	NiRefObject();
	virtual ~NiRefObject();

	unsigned int IncRefCount();
	unsigned int DecRefCount();
	unsigned int GetRefCount() const;

	static unsigned int GetTotalObjectCount();

protected:
	virtual void DeleteThis();

private:
	volatile unsigned int m_uiRefCount;
	static volatile unsigned int ms_uiObjects;
};
static_assert(sizeof(NiRefObject) == 0x10);

#include "Gamebryo/CoreLibs/NiMain/NiRefObject.inl"
