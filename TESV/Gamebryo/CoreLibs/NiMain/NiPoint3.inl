#pragma once

inline NiPoint3::NiPoint3(float fX, float fY, float fZ) :
	x(fX),
	y(fY),
	z(fZ)
{}

inline float& NiPoint3::operator[](int i)
{
	return (&x)[i];
}

inline const float& NiPoint3::operator[](int i) const
{
	return (&x)[i];
}

inline bool NiPoint3::operator==(const NiPoint3& pt) const
{
	return x == pt.x && y == pt.y && z == pt.z;
}

inline bool NiPoint3::operator!=(const NiPoint3& pt) const
{
	return !(*this == pt);
}

inline NiPoint3 NiPoint3::operator+(const NiPoint3& pt) const
{
	return NiPoint3(x + pt.x, y + pt.y, z + pt.z);
}

inline NiPoint3 NiPoint3::operator-(const NiPoint3& pt) const
{
	return NiPoint3(x - pt.x, y - pt.y, z - pt.z);
}

inline float NiPoint3::operator*(const NiPoint3& pt) const
{
	return x * pt.x + y * pt.y + z * pt.z;
}

inline NiPoint3 NiPoint3::operator*(float fScalar) const
{
	return NiPoint3(fScalar * x, fScalar * y, fScalar * z);
}

inline NiPoint3 NiPoint3::operator/(float fScalar) const
{
	float fInvScalar = 1.0f / fScalar;
	return NiPoint3(fInvScalar * x, fInvScalar * y, fInvScalar * z);
}

inline NiPoint3 NiPoint3::operator-() const
{
	return NiPoint3(-x, -y, -z);
}

inline NiPoint3 operator*(float fScalar, const NiPoint3& pt)
{
	return NiPoint3(fScalar * pt.x, fScalar * pt.y, fScalar * pt.z);
}

inline NiPoint3& NiPoint3::operator+=(const NiPoint3& pt)
{
	x += pt.x;
	y += pt.y;
	z += pt.z;
	return *this;
}

inline NiPoint3& NiPoint3::operator-=(const NiPoint3& pt)
{
	x -= pt.x;
	y -= pt.y;
	z -= pt.z;
	return *this;
}

inline NiPoint3& NiPoint3::operator*=(float fScalar)
{
	x *= fScalar;
	y *= fScalar;
	z *= fScalar;
	return *this;
}

inline NiPoint3& NiPoint3::operator/=(float fScalar)
{
	float fInvScalar = 1.0f / fScalar;
	x *= fInvScalar;
	y *= fInvScalar;
	z *= fInvScalar;
	return *this;
}

inline float NiPoint3::SqrLength() const
{
	return x * x + y * y + z * z;
}

inline float NiPoint3::Dot(const NiPoint3& pt) const
{
	return x * pt.x + y * pt.y + z * pt.z;
}

inline NiPoint3 NiPoint3::ComponentProduct(const NiPoint3& p0, const NiPoint3& p1)
{
	return NiPoint3(p0.x * p1.x, p0.y * p1.y, p0.z * p1.z);
}
