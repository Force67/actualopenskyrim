#pragma once

#include "Gamebryo/CoreLibs/NiMain/NiPoint3.h"

class NiStream;

class NiPlane
{
public:
	NiPlane();
	NiPlane(const NiPoint3& kNormal, float fConstant);
	NiPlane(const NiPoint3& kNormal, const NiPoint3& kPoint);
	NiPlane(const NiPoint3& kP0, const NiPoint3& kP1, const NiPoint3& kP2);

	// Points p on the plane satisfy Dot(m_kNormal, p) == m_fConstant.
	NiPoint3 m_kNormal;
	float m_fConstant;

	void LoadBinary(NiStream& stream);
	void SaveBinary(NiStream& stream);
};
static_assert(sizeof(NiPlane) == 0x10);
