#pragma once

template <class T>
struct BSTSingletonImplicit
{
	static T* QInstance();
};

template <class T>
class BSTSingletonExplicit
{
public:
	static T& QInstance() { return *QinstancePtr(); }
	static bool QInitialized() { return QinstancePtr() != nullptr; }

protected:
	BSTSingletonExplicit() { QinstancePtr() = static_cast<T*>(this); }
	~BSTSingletonExplicit() { QinstancePtr() = nullptr; }

private:
	static T*& QinstancePtr()
	{
		static T* pInstanceS;
		return pInstanceS;
	}
};
