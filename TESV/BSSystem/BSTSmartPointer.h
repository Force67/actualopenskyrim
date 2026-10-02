#pragma once

struct BSTSmartPointerIntrusiveRefCount
{
	template <class T> static T* Acquire(T* apPointer)
	{
		if (apPointer)
			apPointer->IncRef();
		return apPointer;
	}
	template <class T> static void Release(T* apPointer)
	{
		if (apPointer && apPointer->DecRef() == 0)
			delete apPointer;
	}
};

template <class T, class Policy = BSTSmartPointerIntrusiveRefCount>
class BSTSmartPointer
{
public:
	BSTSmartPointer() : pPtr(nullptr) {}
	explicit BSTSmartPointer(T* apPointer) : pPtr(Policy::Acquire(apPointer)) {}
	BSTSmartPointer(const BSTSmartPointer& arPointer) : pPtr(Policy::Acquire(arPointer.pPtr)) {}
	~BSTSmartPointer() { Policy::Release(pPtr); }
	BSTSmartPointer& operator=(T* apPointer)
	{
		T* pOld = pPtr;
		if (apPointer != pOld)
		{
			Policy::Acquire(apPointer);
			pPtr = apPointer;
			Policy::Release(pOld);
		}
		return *this;
	}
	BSTSmartPointer& operator=(const BSTSmartPointer& arPointer) { return *this = arPointer.pPtr; }
	T* operator->() const { return pPtr; }
	T* get() const { return pPtr; }

	T* pPtr;
};
