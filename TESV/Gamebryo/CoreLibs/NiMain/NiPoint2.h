#pragma once

class NiPoint2
{
public:
	float x, y;

	NiPoint2(float fX = 0.0f, float fY = 0.0f);

	float& operator[](int i);
	const float& operator[](int i) const;

	bool operator==(const NiPoint2& pt) const;
	bool operator!=(const NiPoint2& pt) const;
	NiPoint2 operator+(const NiPoint2& pt) const;
	NiPoint2 operator-(const NiPoint2& pt) const;
	float operator*(const NiPoint2& pt) const;
	NiPoint2 operator*(float fScalar) const;
	NiPoint2 operator/(float fScalar) const;
	NiPoint2 operator-() const;
	NiPoint2& operator+=(const NiPoint2& pt);
	NiPoint2& operator-=(const NiPoint2& pt);
	NiPoint2& operator*=(float fScalar);
	NiPoint2& operator/=(float fScalar);

	float SqrLength() const;
	float Dot(const NiPoint2& pt) const;

	static NiPoint2 ComponentProduct(const NiPoint2& p0, const NiPoint2& p1);

	void LoadBinary(class NiStream& stream);
	void SaveBinary(class NiStream& stream);
	static void LoadBinary(class NiStream& stream, NiPoint2* pkValues, unsigned int uiNumValues);
	static void SaveBinary(class NiStream& stream, NiPoint2* pkValues, unsigned int uiNumValues);
};
static_assert(sizeof(NiPoint2) == 0x8);

#include "Gamebryo/CoreLibs/NiMain/NiPoint2.inl"
