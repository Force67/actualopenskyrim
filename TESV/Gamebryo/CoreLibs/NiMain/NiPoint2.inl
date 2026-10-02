#pragma once

inline NiPoint2::NiPoint2(float fX, float fY) :
	x(fX),
	y(fY)
{}

inline float& NiPoint2::operator[](int i)
{
	return (&x)[i];
}

inline const float& NiPoint2::operator[](int i) const
{
	return (&x)[i];
}

inline bool NiPoint2::operator==(const NiPoint2& pt) const
{
	return x == pt.x && y == pt.y;
}

inline bool NiPoint2::operator!=(const NiPoint2& pt) const
{
	return !(*this == pt);
}

inline NiPoint2 NiPoint2::operator+(const NiPoint2& pt) const
{
	return NiPoint2(x + pt.x, y + pt.y);
}

inline NiPoint2 NiPoint2::operator-(const NiPoint2& pt) const
{
	return NiPoint2(x - pt.x, y - pt.y);
}

inline float NiPoint2::operator*(const NiPoint2& pt) const
{
	return x * pt.x + y * pt.y;
}

inline NiPoint2 NiPoint2::operator*(float fScalar) const
{
	return NiPoint2(fScalar * x, fScalar * y);
}

inline NiPoint2 NiPoint2::operator/(float fScalar) const
{
	float fInvScalar = 1.0f / fScalar;
	return NiPoint2(fInvScalar * x, fInvScalar * y);
}

inline NiPoint2 NiPoint2::operator-() const
{
	return NiPoint2(-x, -y);
}

inline NiPoint2 operator*(float fScalar, const NiPoint2& pt)
{
	return NiPoint2(fScalar * pt.x, fScalar * pt.y);
}

inline NiPoint2& NiPoint2::operator+=(const NiPoint2& pt)
{
	x += pt.x;
	y += pt.y;
	return *this;
}

inline NiPoint2& NiPoint2::operator-=(const NiPoint2& pt)
{
	x -= pt.x;
	y -= pt.y;
	return *this;
}

inline NiPoint2& NiPoint2::operator*=(float fScalar)
{
	x *= fScalar;
	y *= fScalar;
	return *this;
}

inline NiPoint2& NiPoint2::operator/=(float fScalar)
{
	float fInvScalar = 1.0f / fScalar;
	x *= fInvScalar;
	y *= fInvScalar;
	return *this;
}

inline float NiPoint2::SqrLength() const
{
	return x * x + y * y;
}

inline float NiPoint2::Dot(const NiPoint2& pt) const
{
	return x * pt.x + y * pt.y;
}

inline NiPoint2 NiPoint2::ComponentProduct(const NiPoint2& p0, const NiPoint2& p1)
{
	return NiPoint2(p0.x * p1.x, p0.y * p1.y);
}
