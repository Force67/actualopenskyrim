#pragma once

inline NiColor::NiColor(float fR, float fG, float fB) :
	r(fR),
	g(fG),
	b(fB)
{}

inline NiColor& NiColor::operator=(float fScalar)
{
	r = fScalar;
	g = fScalar;
	b = fScalar;
	return *this;
}

inline bool NiColor::operator==(const NiColor& c) const
{
	return r == c.r && g == c.g && b == c.b;
}

inline bool NiColor::operator!=(const NiColor& c) const
{
	return !(*this == c);
}

inline NiColor NiColor::operator+(const NiColor& c) const
{
	return NiColor(r + c.r, g + c.g, b + c.b);
}

inline NiColor NiColor::operator-(const NiColor& c) const
{
	return NiColor(r - c.r, g - c.g, b - c.b);
}

inline NiColor NiColor::operator*(float fScalar) const
{
	return NiColor(r * fScalar, g * fScalar, b * fScalar);
}

inline NiColor NiColor::operator*(const NiColor& c) const
{
	return NiColor(r * c.r, g * c.g, b * c.b);
}

inline NiColor NiColor::operator/(float fScalar) const
{
	return NiColor(r / fScalar, g / fScalar, b / fScalar);
}

inline NiColor NiColor::operator/(const NiColor& c) const
{
	return NiColor(r / c.r, g / c.g, b / c.b);
}

inline NiColor NiColor::operator-() const
{
	return NiColor(-r, -g, -b);
}

inline NiColor& NiColor::operator+=(const NiColor& c)
{
	r += c.r;
	g += c.g;
	b += c.b;
	return *this;
}

inline NiColor& NiColor::operator-=(const NiColor& c)
{
	r -= c.r;
	g -= c.g;
	b -= c.b;
	return *this;
}

inline NiColor& NiColor::operator*=(float fScalar)
{
	r *= fScalar;
	g *= fScalar;
	b *= fScalar;
	return *this;
}

inline NiColor& NiColor::operator*=(const NiColor& c)
{
	r *= c.r;
	g *= c.g;
	b *= c.b;
	return *this;
}

inline NiColor& NiColor::operator/=(float fScalar)
{
	r /= fScalar;
	g /= fScalar;
	b /= fScalar;
	return *this;
}

inline NiColor& NiColor::operator/=(const NiColor& c)
{
	r /= c.r;
	g /= c.g;
	b /= c.b;
	return *this;
}

inline NiColorA::NiColorA(float fR, float fG, float fB, float fA) :
	r(fR),
	g(fG),
	b(fB),
	a(fA)
{}

inline NiColorA& NiColorA::operator=(float fScalar)
{
	r = fScalar;
	g = fScalar;
	b = fScalar;
	a = fScalar;
	return *this;
}

inline bool NiColorA::operator==(const NiColorA& c) const
{
	return r == c.r && g == c.g && b == c.b && a == c.a;
}

inline bool NiColorA::operator!=(const NiColorA& c) const
{
	return !(*this == c);
}

inline NiColorA NiColorA::operator+(const NiColorA& c) const
{
	return NiColorA(r + c.r, g + c.g, b + c.b, a + c.a);
}

inline NiColorA NiColorA::operator-(const NiColorA& c) const
{
	return NiColorA(r - c.r, g - c.g, b - c.b, a - c.a);
}

inline NiColorA NiColorA::operator*(float fScalar) const
{
	return NiColorA(r * fScalar, g * fScalar, b * fScalar, a * fScalar);
}

inline NiColorA NiColorA::operator*(const NiColorA& c) const
{
	return NiColorA(r * c.r, g * c.g, b * c.b, a * c.a);
}

inline NiColorA NiColorA::operator/(float fScalar) const
{
	return NiColorA(r / fScalar, g / fScalar, b / fScalar, a / fScalar);
}

inline NiColorA NiColorA::operator/(const NiColorA& c) const
{
	return NiColorA(r / c.r, g / c.g, b / c.b, a / c.a);
}

inline NiColorA NiColorA::operator-() const
{
	return NiColorA(-r, -g, -b, -a);
}

inline NiColorA& NiColorA::operator+=(const NiColorA& c)
{
	r += c.r;
	g += c.g;
	b += c.b;
	a += c.a;
	return *this;
}

inline NiColorA& NiColorA::operator-=(const NiColorA& c)
{
	r -= c.r;
	g -= c.g;
	b -= c.b;
	a -= c.a;
	return *this;
}

inline NiColorA& NiColorA::operator*=(float fScalar)
{
	r *= fScalar;
	g *= fScalar;
	b *= fScalar;
	a *= fScalar;
	return *this;
}

inline NiColorA& NiColorA::operator*=(const NiColorA& c)
{
	r *= c.r;
	g *= c.g;
	b *= c.b;
	a *= c.a;
	return *this;
}

inline NiColorA& NiColorA::operator/=(float fScalar)
{
	r /= fScalar;
	g /= fScalar;
	b /= fScalar;
	a /= fScalar;
	return *this;
}

inline NiColorA& NiColorA::operator/=(const NiColorA& c)
{
	r /= c.r;
	g /= c.g;
	b /= c.b;
	a /= c.a;
	return *this;
}
