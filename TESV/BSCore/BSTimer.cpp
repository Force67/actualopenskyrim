#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"

#include <emmintrin.h>

float BSTimer::fGlobalTimeMultiplier = 1.0f;
float BSTimer::fGlobalTimeMultiplierTarget = 1.0f;
float BSTimer::fGlobalTimeMultiplierStep = 0.02f;
float BSTimer::fFrequencyMS;
float BSTimer::fFrequencyUS;

namespace
{
	unsigned int ElapsedMilliseconds(const BSTimer& arTimer)
	{
		const auto uiElapsed = static_cast<uint64_t>(arTimer.GetHighPrecisionTime()) -
			static_cast<uint64_t>(arTimer.iHighPrecisionInitTime);
		const double dElapsed = static_cast<double>(static_cast<int64_t>(uiElapsed)) / BSTimer::fFrequencyMS;
		return static_cast<unsigned int>(_mm_cvttsd_si64(_mm_set_sd(dElapsed)));
	}
}

void BSTimer::SetGlobalTimeMultiplier(float afMult, bool abNow)
{
	fGlobalTimeMultiplierTarget = afMult;
	if (bUseGlobalTimeMultiplierTarget)
	{
		if (abNow)
			fGlobalTimeMultiplier = afMult;
	}
	else if (afMult < fGlobalTimeMultiplier || afMult > fGlobalTimeMultiplier)
	{
		fDelta /= fGlobalTimeMultiplier;
		fGlobalTimeMultiplier = afMult;
		fDelta *= afMult;
	}
}

void BSTimer::UpdateGlobalTimeMultiplier()
{
	if (fGlobalTimeMultiplier >= fGlobalTimeMultiplierTarget)
	{
		if (fGlobalTimeMultiplier > fGlobalTimeMultiplierTarget)
		{
			fGlobalTimeMultiplier *= 1.0f - fGlobalTimeMultiplierStep;
			if (!(fGlobalTimeMultiplier >= fGlobalTimeMultiplierTarget))
				fGlobalTimeMultiplier = fGlobalTimeMultiplierTarget;
		}
	}
	else
	{
		fGlobalTimeMultiplier *= fGlobalTimeMultiplierStep + 1.0f;
		if (fGlobalTimeMultiplier > fGlobalTimeMultiplierTarget)
			fGlobalTimeMultiplier = fGlobalTimeMultiplierTarget;
	}
}

void BSTimer::Enable()
{
	if (uiDisableCounter && !--uiDisableCounter)
	{
		const unsigned int uiTime = ElapsedMilliseconds(*this);
		uiFirstTime = uiTime - uiDisabledFirstTime;
		uiLastTime = uiTime - uiDisabledLastTime;
		uiPreviousTime = uiLastTime;
	}
}

void BSTimer::Disable()
{
	if (!uiDisableCounter++)
	{
		const unsigned int uiTime = ElapsedMilliseconds(*this);
		uiDisabledFirstTime = uiTime - uiFirstTime;
		uiDisabledLastTime = uiTime - uiLastTime;
	}
}

void BSTimer::SetSampleCount(unsigned int auiCount)
{
	if (cSampleCount == auiCount)
		return;
	if (auiCount - 3 > 251)
		ClearSamples();
	else
	{
		cSampleCount = static_cast<unsigned char>(auiCount);
		CreateSamples();
	}
}

void BSTimer::CreateSamples()
{
	if (pSamples)
		return;
	const unsigned int uiCount = cSampleCount;
	pSamples = static_cast<float*>(MemoryManager::Instance().Allocate(uiCount * sizeof(float), 0, false));
	for (unsigned int i = 0; i < uiCount; ++i)
		pSamples[i] = 0.016f;
	cSampleIndex = 0;
}

void BSTimer::ClearSamples()
{
	cSampleCount = 0;
	float* const pBuffer = pSamples;
	MemoryManager::Instance().Deallocate(pBuffer, false);
	pSamples = nullptr;
}

void BSTimer::Update(unsigned int auiTime)
{
	if (uiDisableCounter)
	{
		fDelta = fRealTimeDelta = 0.0f;
		return;
	}
	unsigned int uiTime = auiTime - uiFirstTime;
	if (!(fClamp < 0.0f || fClamp > 0.0f))
	{
		const float fElapsed = static_cast<float>(uiTime - uiPreviousTime);
		fDelta = fElapsed > 166.66667f ? 0.16666667f : fElapsed * 0.001f;
	}
	else
	{
		const float fTotal = fClamp + fClampRemainder;
		const auto uiWhole = static_cast<unsigned int>(_mm_cvttss_si64(_mm_set_ss(fTotal)));
		uiTime = uiPreviousTime + uiWhole;
		fClampRemainder = fTotal - static_cast<float>(uiWhole);
		fDelta = fClamp * 0.001f;
	}
	if (cSampleCount < 3)
		uiLastTime = uiTime;
	else
	{
		pSamples[cSampleIndex] = fDelta;
		cSampleIndex = (static_cast<unsigned int>(cSampleIndex) + 1) % cSampleCount;
		__m128 sum = _mm_setzero_ps();
		__m128 maximum = _mm_setzero_ps();
		__m128 minimum = _mm_set_ss(0.16666667f);
		unsigned int i = 0;
		if (cSampleCount >= 8)
		{
			__m128 sums[2] = {_mm_setzero_ps(), _mm_setzero_ps()};
			__m128 maxima[2] = {_mm_setzero_ps(), _mm_setzero_ps()};
			__m128 minima[2] = {_mm_set1_ps(0.16666667f), _mm_set1_ps(0.16666667f)};
			for (; i < (cSampleCount & ~7u); i += 8)
			{
				for (unsigned int j = 0; j < 2; ++j)
				{
					const __m128 values = _mm_loadu_ps(pSamples + i + j * 4);
					sums[j] = _mm_add_ps(sums[j], values);
					maxima[j] = _mm_max_ps(maxima[j], values);
					minima[j] = _mm_min_ps(minima[j], values);
				}
			}
			const __m128 total = _mm_add_ps(sums[0], sums[1]);
			const __m128 high = _mm_max_ps(maxima[0], maxima[1]);
			const __m128 low = _mm_min_ps(minima[0], minima[1]);
			sum = _mm_add_ps(_mm_movehl_ps(total, total), total);
			maximum = _mm_max_ps(_mm_movehl_ps(high, high), high);
			minimum = _mm_min_ps(_mm_movehl_ps(low, low), low);
			sum = _mm_add_ss(sum, _mm_shuffle_ps(sum, sum, 0xF5));
			maximum = _mm_max_ss(maximum, _mm_shuffle_ps(maximum, maximum, 0xF5));
			minimum = _mm_min_ss(minimum, _mm_shuffle_ps(minimum, minimum, 0xF5));
		}
		for (; i < cSampleCount; ++i)
		{
			const __m128 value = _mm_set_ss(pSamples[i]);
			sum = _mm_add_ss(sum, value);
			maximum = _mm_max_ss(value, maximum);
			minimum = _mm_min_ss(value, minimum);
		}
		maximum = _mm_min_ss(_mm_max_ss(maximum, _mm_setzero_ps()), _mm_set_ss(0.05f));
		sum = _mm_div_ss(_mm_sub_ss(_mm_sub_ss(sum, maximum), minimum), _mm_set_ss(static_cast<float>(cSampleCount - 2)));
		fDelta = _mm_cvtss_f32(sum);
		uiLastTime += static_cast<unsigned int>(_mm_cvttss_si64(_mm_set_ss(fDelta * 1000.0f)));
	}
	uiPreviousTime = uiTime;
	fRealTimeDelta = fDelta;
	fDelta *= fGlobalTimeMultiplier;
}
