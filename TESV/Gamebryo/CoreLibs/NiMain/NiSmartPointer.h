#pragma once


template<class T>
class NiPointer
{
public:
	NiPointer() : m_pObject(nullptr) {}
	NiPointer(T* pkObject) : m_pObject(pkObject)
	{
		if (m_pObject)
			m_pObject->IncRefCount();
	}
	NiPointer(const NiPointer& kOther) : NiPointer(kOther.m_pObject) {}
	~NiPointer()
	{
		if (m_pObject)
			m_pObject->DecRefCount();
	}
	NiPointer& operator=(T* pkObject)
	{
		T* pkOld = m_pObject;
		if (pkOld != pkObject)
		{
			if (pkObject)
				pkObject->IncRefCount();
			m_pObject = pkObject;
			if (pkOld)
				pkOld->DecRefCount();
		}
		return *this;
	}
	NiPointer& operator=(const NiPointer& kOther) { return operator=(kOther.m_pObject); }
	operator T*() const { return m_pObject; }
	T* operator->() const { return m_pObject; }

	T* m_pObject;
};
