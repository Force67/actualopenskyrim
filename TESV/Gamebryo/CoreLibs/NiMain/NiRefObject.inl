#include <windows.h>

inline NiRefObject::NiRefObject() :
	m_uiRefCount(0)
{
	InterlockedIncrement(reinterpret_cast<volatile LONG*>(&ms_uiObjects));
}

inline unsigned int NiRefObject::IncRefCount()
{
	return InterlockedIncrement(reinterpret_cast<volatile LONG*>(&m_uiRefCount));
}

inline unsigned int NiRefObject::DecRefCount()
{
	const unsigned int uiCount = InterlockedDecrement(reinterpret_cast<volatile LONG*>(&m_uiRefCount));
	if (!uiCount)
		DeleteThis();
	return uiCount;
}

inline unsigned int NiRefObject::GetRefCount() const
{
	return m_uiRefCount;
}

inline unsigned int NiRefObject::GetTotalObjectCount()
{
	return ms_uiObjects;
}
