#pragma once

#include <cstdint>

template<class T> class NiTMallocInterface;

template<class T, class Allocator>
class NiTArray
{
public:
	NiTArray(uint16_t usMaxSize = 0, uint16_t usGrowBy = 1) :
		m_pBase(nullptr), m_usMaxSize(0), m_usSize(0), m_usESize(0), m_usGrowBy(usGrowBy)
	{
		if (usMaxSize)
			SetSize(usMaxSize);
	}
	virtual ~NiTArray();
	uint32_t AddFirstEmpty(const T& kValue);
	void SetSize(uint32_t uiSize);

	T* m_pBase;
	uint16_t m_usMaxSize;
	uint16_t m_usSize;
	uint16_t m_usESize;
	uint16_t m_usGrowBy;
};

template<class T>
class NiTPrimitiveArray : public NiTArray<T, NiTMallocInterface<T>>
{
public:
	using NiTArray<T, NiTMallocInterface<T>>::NiTArray;
	~NiTPrimitiveArray() override;
};

static_assert(sizeof(NiTPrimitiveArray<uint32_t>) == 24);
