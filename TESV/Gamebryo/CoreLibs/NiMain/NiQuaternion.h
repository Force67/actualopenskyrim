#pragma once

#include "Gamebryo/CoreLibs/NiMain/NiPoint3.h"

class NiMatrix3;
class NiStream;

class NiQuaternion
{
public:
	// NOTE: Variable declaration order affects assembly language code. Do
	// not change.
	float m_fW;
	float m_fX;
	float m_fY;
	float m_fZ;

	NiQuaternion() = default;
	NiQuaternion(float fW, float fX, float fY, float fZ);

	void FromAngleAxis(float fAngle, const NiPoint3& kAxis);
	void ToAngleAxis(float& fAngle, NiPoint3& kAxis) const;
	void FromEulerAnglesXYZ(float fXAngle, float fYAngle, float fZAngle);
	void FromRotation(const NiMatrix3& xMat);

	void Slerp(float fT, const NiQuaternion& xqStart, const NiQuaternion& xqEnd);
	void Squad(float fT, const NiQuaternion& xqP, const NiQuaternion& xqA,
		const NiQuaternion& xqB, const NiQuaternion& xqQ);
	static void Multiply(const NiQuaternion& xqA, NiQuaternion& xqResult,
		const NiQuaternion& xqB);
	static void Log(NiQuaternion& xqResult, const NiQuaternion& xqSrc);
	static void Exp(NiQuaternion& xqResult, const NiQuaternion& xqSrc);
	static void Intermediate(NiQuaternion& xqResult, const NiQuaternion& xqQ0,
		const NiQuaternion& xqA, const NiQuaternion& xqQ1);

	void LoadBinary(NiStream& stream);
	void SaveBinary(NiStream& stream);
};
static_assert(sizeof(NiQuaternion) == 0x10);

#include "Gamebryo/CoreLibs/NiMain/NiQuaternion.inl"
