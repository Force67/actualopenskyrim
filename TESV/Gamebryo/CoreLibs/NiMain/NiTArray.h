#pragma once

#include <cstdint>

template<class T> class NiTMallocInterface;

template<class T, class Allocator>
class NiTArray
{
public:
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
class NiTPrimitiveArray : public NiTArray<T, NiTMallocInterface<T>> {};

static_assert(sizeof(NiTPrimitiveArray<uint32_t>) == 24);
