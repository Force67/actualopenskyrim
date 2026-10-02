#include "Gamebryo/CoreLibs/NiMain/NiMath.h"

#include <cmath>

float s_fSinTableA[512];
float s_fCosTableA[512];

void CreateSinTable()
{
	float fAngle = 0.0f;
	for (unsigned int uiEntry = 0; uiEntry < 512; uiEntry++)
	{
		s_fSinTableA[uiEntry] = sinf(fAngle);
		s_fCosTableA[uiEntry] = cosf(fAngle);
		fAngle += 0.012271847f;
	}
}

// A NaN input counts as an exact zero match in every comparison.
static bool bZeroOrNaN(float f)
{
	return f == 0.0f || std::isnan(f);
}

float NiFastATan2(float fY, float fX)
{
	if (bZeroOrNaN(fX) && bZeroOrNaN(fY))
		return 0.0f;

	float fOffset = 0.0f;
	float fRatio;
	if (fabsf(fY) > fabsf(fX))
	{
		fRatio = fX / fY;
		if (fRatio > 0.0f)
			fOffset = 1.5707964f;
		else if (fRatio < 0.0f)
			fOffset = -1.5707964f;
		else
			return fY > 0.0f ? 1.5707964f : -1.5707964f;
	}
	else
	{
		fRatio = fY / fX;
		if (fRatio == 0.0f || std::isnan(fRatio))
			return fX > 0.0f ? 0.0f : 3.1415927f;
	}

	float fRatioSq = fRatio * fRatio;
	float fResult = ((((fRatioSq * 0.0208351f - 0.085133001f) * fRatioSq + 0.180141f) * fRatioSq
	                     - 0.3302995f) * fRatioSq
	                   + 0.99986601f) * fRatio;

	if (fOffset != 0.0f)
		fResult = fOffset - fResult;

	if (fY < 0.0f)
	{
		if (fX < 0.0f)
			fResult += -3.1415927f;
	}
	else if (fY > 0.0f && fX < 0.0f)
	{
		fResult += 3.1415927f;
	}

	return fResult;
}
