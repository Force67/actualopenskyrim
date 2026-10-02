#include "Gamebryo/CoreLibs/NiMain/NiPlane.h"

#include <cmath>

NiPlane::NiPlane() :
	m_kNormal(0.0f, 0.0f, 0.0f),
	m_fConstant(0.0f)
{}

NiPlane::NiPlane(const NiPoint3& kNormal, float fConstant) :
	m_kNormal(kNormal),
	m_fConstant(fConstant)
{}

NiPlane::NiPlane(const NiPoint3& kNormal, const NiPoint3& kPoint) :
	m_kNormal(kNormal)
{
	// Add order is observable through NaN payload selection, keep as written.
	float fZTerm = kNormal.z * kPoint.z;
	m_fConstant = kNormal.y * kPoint.y + kNormal.x * kPoint.x + fZTerm;
}

NiPlane::NiPlane(const NiPoint3& kP0, const NiPoint3& kP1, const NiPoint3& kP2)
{
	float fUx = kP1.x - kP0.x;
	float fUy = kP1.y - kP0.y;
	float fUz = kP1.z - kP0.z;
	float fVx = kP2.x - kP1.x;
	float fVy = kP2.y - kP1.y;
	float fVz = kP2.z - kP1.z;

	float fNormalX = fVz * fUy - fVy * fUz;
	float fNormalY = fVx * fUz - fVz * fUx;
	float fNormalZ = fVy * fUx - fVx * fUy;

	float fLen = sqrtf(fNormalX * fNormalX + fNormalY * fNormalY + fNormalZ * fNormalZ);

	// Collinear points get a zero normal.
	if (!(fLen > 0.000001f))
	{
		fNormalX = 0.0f;
		fNormalY = 0.0f;
		fNormalZ = 0.0f;
	}
	else
	{
		float fInvLen = 1.0f / fLen;
		fNormalX *= fInvLen;
		fNormalY *= fInvLen;
		fNormalZ *= fInvLen;
	}

	m_kNormal.x = fNormalX;
	m_kNormal.y = fNormalY;
	m_kNormal.z = fNormalZ;
	float fZTerm = fNormalZ * kP0.z;
	m_fConstant = (fNormalX * kP0.x + fNormalY * kP0.y) + fZTerm;
}
