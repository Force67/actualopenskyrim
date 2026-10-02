#include "NiStream.h"
#include "Gamebryo/CoreLibs/NiSystem/NiMemoryDefines.h"

template<class T, class Allocator>
NiTArray<T, Allocator>::~NiTArray()
{
	_NiFree(m_pBase);
}

template<class T>
NiTPrimitiveArray<T>::~NiTPrimitiveArray() = default;

template NiTArray<NiStream::PostProcessFunction, NiTMallocInterface<NiStream::PostProcessFunction>>::~NiTArray();
template NiTPrimitiveArray<NiStream::PostProcessFunction>::~NiTPrimitiveArray();
