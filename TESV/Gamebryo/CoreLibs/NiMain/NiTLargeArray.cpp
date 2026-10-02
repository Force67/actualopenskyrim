#include "NiStream.h"
#include "Gamebryo/CoreLibs/NiSystem/NiMemoryDefines.h"
#include <type_traits>

template<class T, class Allocator>
NiTLargeArray<T, Allocator>::~NiTLargeArray()
{
	if constexpr (std::is_same_v<Allocator, NiTMallocInterface<T>>)
		_NiFree(m_pBase);
	else if (m_pBase)
	{
		T* pkBase = m_pBase;
		auto* pstAllocation = reinterpret_cast<size_t*>(pkBase) - 1;
		for (size_t stCount = *pstAllocation; stCount; --stCount)
			pkBase[stCount - 1].~T();
		::operator delete[](pstAllocation);
	}
}

template NiTLargeArray<NiPointer<NiObject>, NiTNewInterface<NiPointer<NiObject>>>::~NiTLargeArray();
template NiTLargeArray<BSFixedString, NiTNewInterface<BSFixedString>>::~NiTLargeArray();
template NiTLargeArray<uint32_t, NiTMallocInterface<uint32_t>>::~NiTLargeArray();

template<class T>
NiTLargeObjectArray<T>::~NiTLargeObjectArray() = default;

template<class T>
NiTLargePrimitiveArray<T>::~NiTLargePrimitiveArray() = default;

template NiTLargeObjectArray<NiPointer<NiObject>>::~NiTLargeObjectArray();
template NiTLargeObjectArray<BSFixedString>::~NiTLargeObjectArray();
template NiTLargePrimitiveArray<uint32_t>::~NiTLargePrimitiveArray();
