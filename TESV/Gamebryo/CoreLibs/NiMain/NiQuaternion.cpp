#include "Gamebryo/CoreLibs/NiMain/NiQuaternion.h"

#include "Gamebryo/CoreLibs/NiMain/NiMatrix3.h"

#include <cmath>

NiQuaternion::NiQuaternion(float fW, float fX, float fY, float fZ) :
	m_fW(fW),
	m_fX(fX),
	m_fY(fY),
	m_fZ(fZ)
{}

void NiQuaternion::FromAngleAxis(float fAngle, const NiPoint3& kAxis)
{
	float fHalf = fAngle * 0.5f;
	float fSin = sinf(fHalf);
	m_fW = cosf(fHalf);
	m_fX = fSin * kAxis.x;
	m_fY = fSin * kAxis.y;
	m_fZ = fSin * kAxis.z;
}

void NiQuaternion::ToAngleAxis(float& fAngle, NiPoint3& kAxis) const
{
	// Length of the vector part; below a small threshold the axis is
	// meaningless and the angle and axis are cleared instead.
	float fLength = sqrtf((m_fX * m_fX + m_fY * m_fY) + m_fZ * m_fZ);
	if (fLength < 0.001f)
	{
		fAngle = 0.0f;
		kAxis.x = 0.0f;
		kAxis.y = 0.0f;
		kAxis.z = 0.0f;
		return;
	}

	if (m_fW <= -1.0f)
		fAngle = 3.1415927f;
	else if (m_fW >= 1.0f)
		fAngle = 0.0f;
	else
		fAngle = acosf(m_fW);
	fAngle = fAngle + fAngle;

	float fInvLength = 1.0f / fLength;
	float fX = m_fX;
	float fY = m_fY;
	float fZ = m_fZ;
	kAxis.x = fInvLength * fX;
	kAxis.y = fInvLength * fY;
	kAxis.z = fInvLength * fZ;
}

void NiQuaternion::FromEulerAnglesXYZ(float fXAngle, float fYAngle, float fZAngle)
{
	// The x and z half-angles are negated, the y half-angle is not.
	float fSinX = sinf(fXAngle * -0.5f);
	float fCosX = cosf(fXAngle * -0.5f);
	float fSinY = sinf(fYAngle * 0.5f);
	float fCosY = cosf(fYAngle * 0.5f);
	float fSinZ = sinf(fZAngle * -0.5f);
	float fCosZ = cosf(fZAngle * -0.5f);

	m_fW = (fSinZ * fSinY) * fSinX + (fCosZ * fCosY) * fCosX;
	m_fX = (fCosZ * fCosY) * fSinX - (fSinZ * fSinY) * fCosX;
	m_fY = (fSinX * fCosY) * fSinZ + (fSinY * fCosX) * fCosZ;
	m_fZ = (fCosY * fCosX) * fSinZ - (fSinY * fSinX) * fCosZ;
}

void NiQuaternion::FromRotation(const NiMatrix3& xMat)
{
	float fTrace = (xMat.m_pEntry[0][0] + xMat.m_pEntry[1][1]) + xMat.m_pEntry[2][2];
	if (fTrace > 0.0f)
	{
		float fRoot = sqrtf(fTrace + 1.0f);
		float fScale = 0.5f / fRoot;
		m_fW = fRoot * 0.5f;
		m_fX = (xMat.m_pEntry[2][1] - xMat.m_pEntry[1][2]) * fScale;
		m_fY = (xMat.m_pEntry[0][2] - xMat.m_pEntry[2][0]) * fScale;
		m_fZ = (xMat.m_pEntry[1][0] - xMat.m_pEntry[0][1]) * fScale;
	}
	else
	{
		// Largest diagonal element decides which component the root is
		// solved from, so no subtraction underflows.
		static const unsigned int auiNext[3] = { 1, 2, 0 };

		unsigned int uiLargest = (xMat.m_pEntry[1][1] > xMat.m_pEntry[0][0]) ? 1u : 0u;
		if (xMat.m_pEntry[2][2] > xMat.m_pEntry[uiLargest][uiLargest])
			uiLargest = 2;
		unsigned int uiNext = auiNext[uiLargest];
		unsigned int uiPrev = auiNext[uiNext];

		float fRoot = sqrtf((xMat.m_pEntry[uiLargest][uiLargest]
		                     - xMat.m_pEntry[uiNext][uiNext])
		                - xMat.m_pEntry[uiPrev][uiPrev] + 1.0f);
		float fScale = 0.5f / fRoot;

		float afXYZ[3];
		afXYZ[uiLargest] = fRoot * 0.5f;
		afXYZ[uiNext] = (xMat.m_pEntry[uiLargest][uiNext] + xMat.m_pEntry[uiNext][uiLargest]) * fScale;
		afXYZ[uiPrev] = (xMat.m_pEntry[uiLargest][uiPrev] + xMat.m_pEntry[uiPrev][uiLargest]) * fScale;

		m_fW = (xMat.m_pEntry[uiPrev][uiNext] - xMat.m_pEntry[uiNext][uiPrev]) * fScale;
		m_fX = afXYZ[0];
		m_fY = afXYZ[1];
		m_fZ = afXYZ[2];
	}
}

void NiQuaternion::Slerp(float fT, const NiQuaternion& xqStart, const NiQuaternion& xqEnd)
{
	float fCos = xqEnd.m_fX * xqStart.m_fX;
	fCos = fCos + xqEnd.m_fW * xqStart.m_fW;
	fCos = fCos + xqStart.m_fY * xqEnd.m_fY;
	fCos = fCos + xqStart.m_fZ * xqEnd.m_fZ;
	fCos *= 0.82279688f;

	float fWarp = 1.0f - fCos;
	fWarp = fWarp * fWarp * 0.58549219f;

	float fParam;
	if (fT > 0.5f)
	{
		float fRemain = 1.0f - fT;
		fParam = 1.0f - (((fRemain + fRemain) - 3.0f) * (fWarp * fRemain)
		                  + 1.0f + fWarp) * fRemain;
	}
	else
	{
		fParam = (((fT + fT) - 3.0f) * (fWarp * fT) + 1.0f + fWarp) * fT;
	}

	float fW = (xqEnd.m_fW - xqStart.m_fW) * fParam + xqStart.m_fW;
	float fX = (xqEnd.m_fX - xqStart.m_fX) * fParam + xqStart.m_fX;
	float fY = (xqEnd.m_fY - xqStart.m_fY) * fParam + xqStart.m_fY;
	float fZ = (xqEnd.m_fZ - xqStart.m_fZ) * fParam + xqStart.m_fZ;

	float fInvLen = 1.0f / sqrtf(((fW * fW + fX * fX) + fY * fY) + fZ * fZ);
	m_fW = fW * fInvLen;
	m_fX = fX * fInvLen;
	m_fY = fY * fInvLen;
	m_fZ = fZ * fInvLen;
}

void NiQuaternion::Squad(float fT, const NiQuaternion& xqP, const NiQuaternion& xqA,
	const NiQuaternion& xqB, const NiQuaternion& xqQ)
{
	NiQuaternion xk1, xk2;
	xk1.Slerp(fT, xqP, xqQ);
	xk2.Slerp(fT, xqA, xqB);
	Slerp((fT + fT) * (1.0f - fT), xk1, xk2);
}

void NiQuaternion::Multiply(const NiQuaternion& xqA, NiQuaternion& xqResult,
	const NiQuaternion& xqB)
{
	xqResult.m_fW = xqA.m_fW * xqB.m_fW - xqB.m_fX * xqA.m_fX - xqB.m_fY * xqA.m_fY - xqB.m_fZ * xqA.m_fZ;
	xqResult.m_fX = xqB.m_fW * xqA.m_fX + xqB.m_fX * xqA.m_fW + xqB.m_fZ * xqA.m_fY - xqB.m_fY * xqA.m_fZ;
	xqResult.m_fY = xqB.m_fY * xqA.m_fW + xqB.m_fW * xqA.m_fY + xqB.m_fX * xqA.m_fZ - xqB.m_fZ * xqA.m_fX;
	xqResult.m_fZ = xqB.m_fW * xqA.m_fZ + xqB.m_fZ * xqA.m_fW + xqB.m_fY * xqA.m_fX - xqB.m_fX * xqA.m_fY;
}

void NiQuaternion::Log(NiQuaternion& xqResult, const NiQuaternion& xqSrc)
{
	float fW = xqSrc.m_fW;
	float fAngle;
	if (fW <= -1.0f)
		fAngle = 3.1415927f;
	else if (fW >= 1.0f)
		fAngle = 0.0f;
	else
		fAngle = acosf(fW);

	float fSin = sinf(fAngle);
	float fScale = 1.0f;
	if (fabsf(fSin) >= 0.001f)
		fScale = fAngle / fSin;

	float fX = xqSrc.m_fX;
	float fY = xqSrc.m_fY;
	float fZ = xqSrc.m_fZ;

	xqResult.m_fW = 0.0f;
	xqResult.m_fX = fScale * fX;
	xqResult.m_fY = fScale * fY;
	xqResult.m_fZ = fScale * fZ;
}

void NiQuaternion::Exp(NiQuaternion& xqResult, const NiQuaternion& xqSrc)
{
	float fX = xqSrc.m_fX;
	float fY = xqSrc.m_fY;
	float fZ = xqSrc.m_fZ;
	float fLength = sqrtf((fX * fX + fY * fY) + fZ * fZ);
	float fSin = sinf(fLength);
	float fScale = 1.0f;
	if (fabsf(fSin) >= 0.001f)
		fScale = fSin / fLength;

	xqResult.m_fW = cosf(fLength);
	xqResult.m_fX = fX * fScale;
	xqResult.m_fY = fY * fScale;
	xqResult.m_fZ = fZ * fScale;
}

void NiQuaternion::Intermediate(NiQuaternion& xqResult, const NiQuaternion& xqQ0,
	const NiQuaternion& xqA, const NiQuaternion& xqQ1)
{
	NiQuaternion xkConj(xqA.m_fW, -xqA.m_fX, -xqA.m_fY, -xqA.m_fZ);
	NiQuaternion xk1, xk2;
	Multiply(xkConj, xk1, xqQ1);
	Log(xk1, xk1);
	Multiply(xkConj, xk2, xqQ0);
	Log(xk2, xk2);

	xk1.m_fW = (xk1.m_fW + xk2.m_fW) * -0.25f;
	xk1.m_fX = (xk1.m_fX + xk2.m_fX) * -0.25f;
	xk1.m_fY = (xk1.m_fY + xk2.m_fY) * -0.25f;
	xk1.m_fZ = (xk1.m_fZ + xk2.m_fZ) * -0.25f;

	Exp(xk2, xk1);
	Multiply(xqA, xqResult, xk2);
}
