#include "Gamebryo/CoreLibs/NiMain/NiTransform.h"

#include <cmath>

// Scalar comparisons follow ucomiss semantics: an unordered pair counts as
// equal, so NaN entries never make the transforms differ.
static bool bScalarsEqual(float fA, float fB)
{
	return !(fA > fB) && !(fA < fB);
}

void NiTransform::MakeIdentity()
{
	m_Rotate.m_pEntry[0][0] = 1.0f;
	m_Rotate.m_pEntry[0][1] = 0.0f;
	m_Rotate.m_pEntry[0][2] = 0.0f;
	m_Rotate.m_pEntry[1][0] = 0.0f;
	m_Rotate.m_pEntry[1][1] = 1.0f;
	m_Rotate.m_pEntry[1][2] = 0.0f;
	m_Rotate.m_pEntry[2][0] = 0.0f;
	m_Rotate.m_pEntry[2][1] = 0.0f;
	m_Rotate.m_pEntry[2][2] = 1.0f;
	m_Translate = NiPoint3::ZERO;
	m_fScale = 1.0f;
}

bool NiTransform::operator!=(const NiTransform& xform) const
{
	return !(bScalarsEqual(m_Rotate.m_pEntry[0][0], xform.m_Rotate.m_pEntry[0][0])
	         && bScalarsEqual(m_Rotate.m_pEntry[0][1], xform.m_Rotate.m_pEntry[0][1])
	         && bScalarsEqual(m_Rotate.m_pEntry[0][2], xform.m_Rotate.m_pEntry[0][2])
	         && bScalarsEqual(m_Rotate.m_pEntry[1][0], xform.m_Rotate.m_pEntry[1][0])
	         && bScalarsEqual(m_Rotate.m_pEntry[1][1], xform.m_Rotate.m_pEntry[1][1])
	         && bScalarsEqual(m_Rotate.m_pEntry[1][2], xform.m_Rotate.m_pEntry[1][2])
	         && bScalarsEqual(m_Rotate.m_pEntry[2][0], xform.m_Rotate.m_pEntry[2][0])
	         && bScalarsEqual(m_Rotate.m_pEntry[2][1], xform.m_Rotate.m_pEntry[2][1])
	         && bScalarsEqual(m_Rotate.m_pEntry[2][2], xform.m_Rotate.m_pEntry[2][2])
	         && bScalarsEqual(m_Translate.x, xform.m_Translate.x)
	         && bScalarsEqual(m_Translate.y, xform.m_Translate.y)
	         && bScalarsEqual(m_Translate.z, xform.m_Translate.z)
	         && bScalarsEqual(m_fScale, xform.m_fScale));
}

bool NiTransform::IsIdentity() const
{
	if (!bScalarsEqual(m_Rotate.m_pEntry[0][0], 1.0f)
	    || !bScalarsEqual(m_Rotate.m_pEntry[0][1], 0.0f)
	    || !bScalarsEqual(m_Rotate.m_pEntry[0][2], 0.0f)
	    || !bScalarsEqual(m_Rotate.m_pEntry[1][0], 0.0f)
	    || !bScalarsEqual(m_Rotate.m_pEntry[1][1], 1.0f)
	    || !bScalarsEqual(m_Rotate.m_pEntry[1][2], 0.0f)
	    || !bScalarsEqual(m_Rotate.m_pEntry[2][0], 0.0f)
	    || !bScalarsEqual(m_Rotate.m_pEntry[2][1], 0.0f)
	    || !bScalarsEqual(m_Rotate.m_pEntry[2][2], 1.0f)
	    || !bScalarsEqual(m_Translate.x, 0.0f)
	    || !bScalarsEqual(m_Translate.y, 0.0f)
	    || !bScalarsEqual(m_Translate.z, 0.0f))
	{
		return false;
	}

	// The epsilon test runs in double precision.
	return !(fabsf(m_fScale - 1.0f) > 0.0001);
}
