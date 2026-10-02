#pragma once

#include "Gamebryo/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo/CoreLibs/NiMain/NiPoint3.h"

class NiStream;

class NiTransform
{
public:
	// NOTE: Variable declaration order affects assembly language code. Do
	// not change.
	NiMatrix3 m_Rotate;
	NiPoint3 m_Translate;
	float m_fScale;

	void MakeIdentity();
	bool operator!=(const NiTransform& xform) const;
	bool IsIdentity() const;

	void LoadBinary(NiStream& stream);
	void SaveBinary(NiStream& stream);
};
static_assert(sizeof(NiTransform) == 0x34);
