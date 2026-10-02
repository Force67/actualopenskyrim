#pragma once

#include "NiStream.h"
#include "Gamebryo/CoreLibs/NiSystem/NiBinaryStream.h"

template<class T>
void NiStreamLoadBinary(NiStream& kStream, T* pkValues, uint32_t uiCount)
{
	NiBinaryStream* pkInput = kStream.m_pkIstr;
	uint32_t uiSize = sizeof(T);
	pkInput->m_uiAbsoluteCurrentPos += pkInput->m_pfnRead(pkInput, pkValues, uiSize * uiCount, &uiSize, 1);
}

template<class T>
void NiStreamLoadBinary(NiStream& kStream, T& kValue)
{
	NiStreamLoadBinary(kStream, &kValue, 1);
}

template<class T>
void NiStreamSaveBinary(NiStream& kStream, const T* pkValues, uint32_t uiCount)
{
	NiBinaryStream* pkOutput = kStream.m_pkOstr;
	uint32_t uiSize = sizeof(T);
	pkOutput->m_uiAbsoluteCurrentPos += pkOutput->m_pfnWrite(pkOutput, pkValues, uiSize * uiCount, &uiSize, 1);
}

template<class T>
void NiStreamSaveBinary(NiStream& kStream, const T& kValue)
{
	NiStreamSaveBinary(kStream, &kValue, 1);
}
