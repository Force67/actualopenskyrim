#pragma once

#include <cstdint>

template<class T> class NiTNewInterface;
template<class T> class NiTMallocInterface;

template<class T, class Allocator>
class NiTLargeArray
{
public:
	NiTLargeArray(uint32_t uiMaxSize = 0, uint32_t uiGrowBy = 1) :
		m_pBase(nullptr), m_uiMaxSize(0), m_uiSize(0), m_uiESize(0), m_uiGrowBy(uiGrowBy)
	{
		if (uiMaxSize)
			SetSize(uiMaxSize);
	}
	virtual ~NiTLargeArray();
	void SetSize(uint32_t uiSize);
	void RemoveAll();
	void SetAt(uint32_t uiIndex, const T& kValue)
	{
		if (uiIndex < m_uiSize)
		{
			if (kValue)
			{
				if (!m_pBase[uiIndex])
					++m_uiESize;
			}
			else if (m_pBase[uiIndex])
				--m_uiESize;
		}
		else
		{
			m_uiSize = uiIndex + 1;
			if (kValue)
				++m_uiESize;
		}
		m_pBase[uiIndex] = kValue;
	}
	T RemoveAt(uint32_t uiIndex);

	T* m_pBase;
	uint32_t m_uiMaxSize;
	uint32_t m_uiSize;
	uint32_t m_uiESize;
	uint32_t m_uiGrowBy;
};

template<class T>
class NiTLargeObjectArray : public NiTLargeArray<T, NiTNewInterface<T>>
{
public:
	using NiTLargeArray<T, NiTNewInterface<T>>::NiTLargeArray;
	~NiTLargeObjectArray() override;
};

template<class T>
class NiTLargePrimitiveArray : public NiTLargeArray<T, NiTMallocInterface<T>>
{
public:
	using NiTLargeArray<T, NiTMallocInterface<T>>::NiTLargeArray;
	~NiTLargePrimitiveArray() override;
};

static_assert(sizeof(NiTLargePrimitiveArray<uint32_t>) == 32);

class BSFixedString;
template<>
void NiTLargeArray<BSFixedString, NiTNewInterface<BSFixedString>>::SetAt(uint32_t uiIndex, const BSFixedString& kValue);
