#include "Gamebryo/CoreLibs/NiMain/NiColor.h"

const NiColor NiColor::BLACK(0.0f, 0.0f, 0.0f);
const NiColor NiColor::WHITE(1.0f, 1.0f, 1.0f);
const NiColorA NiColorA::BLACK(0.0f, 0.0f, 0.0f, 1.0f);
const NiColorA NiColorA::WHITE(1.0f, 1.0f, 1.0f, 1.0f);

void NiColor::Clamp()
{
	if (r > 1.0f)
		r = 1.0f;
	if (g > 1.0f)
		g = 1.0f;
	if (b > 1.0f)
		b = 1.0f;
}

void NiColor::Scale()
{
	float fMax = r;
	if (g > fMax)
		fMax = g;
	if (b > fMax)
		fMax = b;

	if (fMax > 1.0f)
	{
		float fInvMax = 1.0f / fMax;
		r *= fInvMax;
		g *= fInvMax;
		b *= fInvMax;
	}
}

void NiColorA::Clamp()
{
	if (r > 1.0f)
		r = 1.0f;
	if (g > 1.0f)
		g = 1.0f;
	if (b > 1.0f)
		b = 1.0f;
	if (a > 1.0f)
		a = 1.0f;
}

void NiColorA::Scale()
{
	// The maximum ignores alpha, but alpha still gets clamped.
	float fMax = r;
	if (g > fMax)
		fMax = g;
	if (b > fMax)
		fMax = b;

	if (fMax > 1.0f)
	{
		float fInvMax = 1.0f / fMax;
		r *= fInvMax;
		g *= fInvMax;
		b *= fInvMax;
	}

	if (a > 1.0f)
		a = 1.0f;
}
