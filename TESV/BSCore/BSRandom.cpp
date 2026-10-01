#include "BSCore/BSRandom.h"

#include <windows.h>
#include <cstdlib>
#include <ctime>

BSRandom::BSRandom()
{
	uiStateA[0] = 0;
	uiIndex = 625;
	bInitialized = false;
}

BSRandom::~BSRandom() = default;

void BSRandom::Init(unsigned int auiSeed)
{
	uiStateA[0] = 0;
	uiIndex = 625;
	Seed(auiSeed);
}

void BSRandom::Seed(unsigned int auiSeed)
{
	uiStateA[1] = auiSeed;
	for (unsigned int i = 1; i < 624; ++i)
	{
		uint32_t uiPrevious = uiStateA[i];
		uiStateA[i + 1] = 1812433253u * (uiPrevious ^ ((uiPrevious >> 30) + i));
	}
	uiIndex = 623;
	srand(auiSeed);
	bInitialized = true;
}

void BSRandom::GenerateNumbers()
{
	if (uiIndex == 625)
		Seed(GetTickCount());
	for (unsigned int i = 0; i < 624; ++i)
	{
		uint32_t uiNext = i == 623 ? uiIndex : uiStateA[i + 2];
		uint32_t uiMix = (uiStateA[i + 1] & 1) + uiNext % 624;
		uint32_t uiValue = uiStateA[(i + 397) % 624 + 1] ^ (uiMix * 2);
		if (uiMix & 1)
			uiValue ^= 0x9908B0DFu;
		uiStateA[i + 1] = uiValue;
	}
	uiIndex = 0;
}

unsigned int BSRandom::UnsignedInt(unsigned int auiMax)
{
	if (!bInitialized)
	{
		int64_t iTime;
		Seed(static_cast<unsigned int>(_time64(&iTime)));
	}
	if (!auiMax)
		return 0;
	uint32_t uiCurrent = uiIndex;
	if (!uiCurrent)
		GenerateNumbers();
	uint32_t uiValue = uiStateA[uiCurrent + 1];
	uiIndex = (uiCurrent + 1) % 624;
	uiValue ^= uiValue >> 11;
	uiValue ^= (uiValue & 0xFF3A58ADu) << 7;
	uiValue ^= (uiValue & 0xFFFFDF8Cu) << 15;
	uiValue ^= uiValue >> 18;
	return ((auiMax + 1) & 0xFFFF7FFFu) ? uiValue % auiMax : uiValue & auiMax;
}
