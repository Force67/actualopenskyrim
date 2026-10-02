#pragma once

#include <cstddef>
#include <cstdint>

class BSTimer
{
public:
	void SetGlobalTimeMultiplier(float afMult, bool abNow);
	static void UpdateGlobalTimeMultiplier();
	void Update(unsigned int auiTime);
	void SetSampleCount(unsigned int auiCount);
	void CreateSamples();
	void ClearSamples();
	void Enable();
	void Disable();
	void Init(unsigned int auiTime);
	long long GetHighPrecisionTime() const;
	static int InitializeAppTimer();
	static void DestroyAppTimer();

	static float fGlobalTimeMultiplier;
	static float fGlobalTimeMultiplierTarget;
	static float fGlobalTimeMultiplierStep;
	static float fFrequencyMS;
	static float fFrequencyUS;

	float* pSamples;
	long long iHighPrecisionInitTime;
	float fClamp;
	float fClampRemainder;
	float fDelta;
	float fRealTimeDelta;
	unsigned int uiLastTime;
	unsigned int uiPreviousTime;
	unsigned int uiFirstTime;
	unsigned int uiDisabledLastTime;
	unsigned int uiDisabledFirstTime;
	unsigned int uiDisableCounter;
	unsigned char cSampleCount;
	unsigned char cSampleIndex;
	bool bUseGlobalTimeMultiplierTarget;
};
static_assert(sizeof(BSTimer) == 0x40);
static_assert(offsetof(BSTimer, iHighPrecisionInitTime) == 8);
static_assert(offsetof(BSTimer, fDelta) == 0x18);
static_assert(offsetof(BSTimer, uiFirstTime) == 0x28);
static_assert(offsetof(BSTimer, uiDisableCounter) == 0x34);
static_assert(offsetof(BSTimer, bUseGlobalTimeMultiplierTarget) == 0x3A);

extern BSTimer appTimer;
