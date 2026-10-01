#pragma once

#include <cstdint>
#include <emmintrin.h>

class BSPrecisionTimer
{
public:
	BSPrecisionTimer() : iStartTime(0), iTargetTime(0) {}
	explicit BSPrecisionTimer(float afTarget) { Reset(afTarget); }
	static void Initialize();
	static int64_t GetTimer();
	static float GetFrequencyRecip() { return fFrequencyMSRecip; }
	float ElapsedTime() const;
	void Reset(float afTargetTime)
	{
		iStartTime = GetTimer();
		iTargetTime = _mm_cvttss_si64(_mm_set_ss(fFrequencyMS * afTargetTime));
	}
	bool Check() const
	{
		return int64_t(uint64_t(GetTimer()) - uint64_t(iStartTime)) > iTargetTime;
	}
	float RemainingTime() const
	{
		const int64_t iTimer = GetTimer();
		const int64_t iRemaining = int64_t(uint64_t(iStartTime) + uint64_t(iTargetTime) - uint64_t(iTimer));
		return static_cast<float>(iRemaining) * fFrequencyMSRecip;
	}
	void Clear() { iTargetTime = 0; }
	bool IsSet() const { return iTargetTime != 0; }

	int64_t iStartTime;
	int64_t iTargetTime;

private:
	static float fFrequencyMS;
	static float fFrequencyMSRecip;
};
static_assert(sizeof(BSPrecisionTimer) == 16);
