#include "BSCore/BSTimer.h"

#include <windows.h>
#include <emmintrin.h>

long long BSTimer::GetHighPrecisionTime() const
{
	LARGE_INTEGER count{};
	QueryPerformanceCounter(&count);
	return count.QuadPart;
}

void BSTimer::Init(unsigned int auiTime)
{
	if (!(fFrequencyMS < 0.0f || fFrequencyMS > 0.0f))
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		fFrequencyMS = static_cast<float>(frequency.QuadPart) * 0.001f;
		fFrequencyUS = fFrequencyMS * 0.001f;
	}
	iHighPrecisionInitTime = GetHighPrecisionTime();
	uiDisableCounter = 0;
	fClamp = fClampRemainder = fDelta = fRealTimeDelta = 0.0f;
	uiLastTime = 0;
	if (auiTime)
		uiFirstTime = auiTime;
	else
	{
		const auto uiElapsed = static_cast<uint64_t>(GetHighPrecisionTime()) - static_cast<uint64_t>(iHighPrecisionInitTime);
		const double dElapsed = static_cast<double>(static_cast<int64_t>(uiElapsed)) / fFrequencyMS;
		uiFirstTime = static_cast<unsigned int>(_mm_cvttsd_si64(_mm_set_sd(dElapsed)));
	}
	bUseGlobalTimeMultiplierTarget = false;
}
