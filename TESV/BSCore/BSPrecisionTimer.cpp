#include "BSCore/BSPrecisionTimer.h"

#include <windows.h>

float BSPrecisionTimer::fFrequencyMS = 1.0f;
float BSPrecisionTimer::fFrequencyMSRecip = 1.0f;

void BSPrecisionTimer::Initialize()
{
	LARGE_INTEGER iFrequency;
	QueryPerformanceFrequency(&iFrequency);
	fFrequencyMS = static_cast<float>(iFrequency.QuadPart) * 0.001f;
	fFrequencyMSRecip = 1.0f / fFrequencyMS;
}

int64_t BSPrecisionTimer::GetTimer()
{
	LARGE_INTEGER iTimer{};
	QueryPerformanceCounter(&iTimer);
	return iTimer.QuadPart;
}

float BSPrecisionTimer::ElapsedTime() const
{
	const float fRecip = fFrequencyMSRecip;
	const int64_t iElapsed = int64_t(uint64_t(GetTimer()) - uint64_t(iStartTime));
	return static_cast<float>(iElapsed) * fRecip;
}

struct BSPrecisionTimerInitializer
{
	BSPrecisionTimerInitializer() { BSPrecisionTimer::Initialize(); }
};

BSPrecisionTimerInitializer PrecisionTimerInitializer;
