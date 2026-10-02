#include "Gamebryo/CoreLibs/NiMain/NiFrustum.h"

NiFrustum::NiFrustum(bool bOrtho)
{
	m_fLeft = 0.0f;
	m_fRight = 0.0f;
	m_fTop = 0.0f;
	m_fBottom = 0.0f;
	m_fNear = 0.0f;
	m_fFar = 0.0f;
	m_bOrtho = bOrtho;
}

NiFrustum::NiFrustum(float fLeft, float fRight, float fTop, float fBottom, float fNear, float fFar,
	bool bOrtho) :
	m_fLeft(fLeft),
	m_fRight(fRight),
	m_fTop(fTop),
	m_fBottom(fBottom),
	m_fNear(fNear),
	m_fFar(fFar),
	m_bOrtho(bOrtho)
{}
