#include "Gamebryo/CoreLibs/NiMain/NiPoint3.h"

#include <cfloat>
#include <cmath>

const NiPoint3 NiPoint3::ZERO(0.0f, 0.0f, 0.0f);

bool NiPoint3::IsOk() const
{
	if (x == FLT_MAX || !std::isfinite(x))
		return false;
	if (y == FLT_MAX || !std::isfinite(y))
		return false;
	if (z == FLT_MAX || !std::isfinite(z))
		return false;
	return true;
}

float NiPoint3::GetZAngleFromVector() const
{
	// A NaN angle would spin the wrap-around loops forever; here it just
	// falls through both of them. The zero check also catches NaN.
	float fAngle;

	if (y == 0.0f || std::isnan(y))
	{
		fAngle = x > 0.0f ? 1.5707964f : 4.712389f;
	}
	else
	{
		fAngle = atanf(x / y);
		if (y < 0.0f)
			fAngle += 3.1415927f;
	}

	while (fAngle < 0.0f)
		fAngle += 6.2831855f;
	while (fAngle > 6.2831855f)
		fAngle -= 6.2831855f;

	return fAngle;
}

float NiPoint3::GetUnclampedZAngleFromVector() const
{
	float fAngle;

	if (y == 0.0f || std::isnan(y))
	{
		fAngle = x > 0.0f ? 1.5707964f : 4.712389f;
	}
	else
	{
		fAngle = atanf(x / y);
		if (y < 0.0f)
			fAngle += 3.1415927f;
	}

	return fAngle;
}

void NiPoint3::UnitizeVectors(NiPoint3* pkV, unsigned int uiVerts, unsigned int uiStride, unsigned int uiEstimate)
{
	// A zero-length vector becomes NaN here: the division by the squared
	// length has no guard.
	for (unsigned int i = 0; i < uiVerts; i++)
	{
		NiPoint3* pkPoint = (NiPoint3*)((char*)pkV + i * uiStride);
		float fLenSq = pkPoint->x * pkPoint->x + pkPoint->y * pkPoint->y + pkPoint->z * pkPoint->z;
		float fScale = uiEstimate == 0 ? 0.0f : 1.0f / fLenSq;
		pkPoint->x *= fScale;
		pkPoint->y *= fScale;
		pkPoint->z *= fScale;
	}
}

void NiPoint3::PointsEqualFloatTimesPoints(NiPoint3* pkDst, float f, const NiPoint3* pkSrc, unsigned int uiVerts)
{
	for (unsigned int i = 0; i < uiVerts; i++)
	{
		pkDst->x = f * pkSrc->x;
		pkDst->y = f * pkSrc->y;
		pkDst->z = f * pkSrc->z;
		pkDst = (NiPoint3*)((char*)pkDst + sizeof(NiPoint3));
		pkSrc = (const NiPoint3*)((const char*)pkSrc + sizeof(NiPoint3));
	}
}

void NiPoint3::PointsPlusEqualFloatTimesPoints(NiPoint3* pkDst, float f, const NiPoint3* pkSrc, unsigned int uiVerts)
{
	for (unsigned int i = 0; i < uiVerts; i++)
	{
		pkDst->x += f * pkSrc->x;
		pkDst->y += f * pkSrc->y;
		pkDst->z += f * pkSrc->z;
		pkDst = (NiPoint3*)((char*)pkDst + sizeof(NiPoint3));
		pkSrc = (const NiPoint3*)((const char*)pkSrc + sizeof(NiPoint3));
	}
}
