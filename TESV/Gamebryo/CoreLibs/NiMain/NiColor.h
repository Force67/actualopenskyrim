#pragma once

class NiColor
{
public:
	float r, g, b;

	NiColor(float fR = 0.0f, float fG = 0.0f, float fB = 0.0f);

	NiColor& operator=(float fScalar);
	bool operator==(const NiColor& c) const;
	bool operator!=(const NiColor& c) const;

	NiColor operator+(const NiColor& c) const;
	NiColor operator-(const NiColor& c) const;
	NiColor operator*(float fScalar) const;
	NiColor operator*(const NiColor& c) const;
	NiColor operator/(float fScalar) const;
	NiColor operator/(const NiColor& c) const;
	NiColor operator-() const;

	NiColor& operator+=(const NiColor& c);
	NiColor& operator-=(const NiColor& c);
	NiColor& operator*=(float fScalar);
	NiColor& operator*=(const NiColor& c);
	NiColor& operator/=(float fScalar);
	NiColor& operator/=(const NiColor& c);

	// Map to the unit cube. Colors are only added or multiplied in the
	// lighting system, so the components are assumed non-negative.
	void Clamp();

	// Scale down by the maximum component, preserving the color.
	void Scale();

	static const NiColor WHITE;
	static const NiColor BLACK;

	void LoadBinary(class NiStream& stream);
	void SaveBinary(class NiStream& stream);
	static void LoadBinary(class NiStream& stream, NiColor* pkValues, unsigned int uiNumValues);
	static void SaveBinary(class NiStream& stream, NiColor* pkValues, unsigned int uiNumValues);
};
static_assert(sizeof(NiColor) == 0xC);

class alignas(16) NiColorA
{
public:
	float r, g, b, a;

	NiColorA(float fR = 0.0f, float fG = 0.0f, float fB = 0.0f, float fA = 0.0f);

	NiColorA& operator=(float fScalar);
	bool operator==(const NiColorA& c) const;
	bool operator!=(const NiColorA& c) const;

	NiColorA operator+(const NiColorA& c) const;
	NiColorA operator-(const NiColorA& c) const;
	NiColorA operator*(float fScalar) const;
	NiColorA operator*(const NiColorA& c) const;
	NiColorA operator/(float fScalar) const;
	NiColorA operator/(const NiColorA& c) const;
	NiColorA operator-() const;

	NiColorA& operator+=(const NiColorA& c);
	NiColorA& operator-=(const NiColorA& c);
	NiColorA& operator*=(float fScalar);
	NiColorA& operator*=(const NiColorA& c);
	NiColorA& operator/=(float fScalar);
	NiColorA& operator/=(const NiColorA& c);

	void Clamp();
	void Scale();

	static const NiColorA WHITE;
	static const NiColorA BLACK;

	void LoadBinary(class NiStream& stream);
	void SaveBinary(class NiStream& stream);
	static void LoadBinary(class NiStream& stream, NiColorA* pkValues, unsigned int uiNumValues);
	static void SaveBinary(class NiStream& stream, NiColorA* pkValues, unsigned int uiNumValues);
};
static_assert(sizeof(NiColorA) == 0x10);

#include "Gamebryo/CoreLibs/NiMain/NiColor.inl"
