#pragma once

// The warp parameter shared by Slerp and Squad: a cubic hermite that eases
// the linear blend so it stays close to the true slerp for small angles.
inline float NiQuaternion_CounterWarp(float fT, float fCos)
{
	float fWarp = 1.0f - (fCos * 0.82279688f);
	fWarp = fWarp * fWarp * 0.58549219f;
	return (((fT + fT) - 3.0f) * (fWarp * fT) + 1.0f + fWarp) * fT;
}
