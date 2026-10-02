#pragma once

class NiPoint3
{
public:
	float x, y, z;

	NiPoint3(float fX = 0.0f, float fY = 0.0f, float fZ = 0.0f);

	float& operator[](int i);
	const float& operator[](int i) const;

	bool operator==(const NiPoint3& pt) const;
	bool operator!=(const NiPoint3& pt) const;
	NiPoint3 operator+(const NiPoint3& pt) const;
	NiPoint3 operator-(const NiPoint3& pt) const;
	float operator*(const NiPoint3& pt) const;
	NiPoint3 operator*(float fScalar) const;
	NiPoint3 operator/(float fScalar) const;
	NiPoint3 operator-() const;
	NiPoint3& operator+=(const NiPoint3& pt);
	NiPoint3& operator-=(const NiPoint3& pt);
	NiPoint3& operator*=(float fScalar);
	NiPoint3& operator/=(float fScalar);

	float SqrLength() const;
	float Dot(const NiPoint3& pt) const;

	static NiPoint3 ComponentProduct(const NiPoint3& p0, const NiPoint3& p1);

	// True when no component is FLT_MAX, NaN or infinite.
	bool IsOk() const;

	// Angle around Z in [0, 2*pi], measured from +Y. The origin maps to
	// 3*pi/2.
	float GetZAngleFromVector() const;
	float GetUnclampedZAngleFromVector() const;

	// Scales every vector by 1/len^2 (yes, squared) while uiEstimate is
	// non-zero, and zeroes every vector when it is zero.
	static void UnitizeVectors(NiPoint3* pkV, unsigned int uiVerts, unsigned int uiStride, unsigned int uiEstimate);

	// dst[i] = f * src[i]
	static void PointsEqualFloatTimesPoints(NiPoint3* pkDst, float f, const NiPoint3* pkSrc, unsigned int uiVerts);
	// dst[i] += f * src[i]
	static void PointsPlusEqualFloatTimesPoints(NiPoint3* pkDst, float f, const NiPoint3* pkSrc, unsigned int uiVerts);

	static const NiPoint3 ZERO;

	void LoadBinary(class NiStream& stream);
	void SaveBinary(class NiStream& stream);
	static void LoadBinary(class NiStream& stream, NiPoint3* pkValues, unsigned int uiNumValues);
	static void SaveBinary(class NiStream& stream, NiPoint3* pkValues, unsigned int uiNumValues);
};
static_assert(sizeof(NiPoint3) == 0xC);

#include "Gamebryo/CoreLibs/NiMain/NiPoint3.inl"
