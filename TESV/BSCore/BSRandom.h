#pragma once

#include <cstdint>

class BSRandom
{
public:
	BSRandom();
	~BSRandom();
	void Init(unsigned int auiSeed);
	void Seed(unsigned int auiSeed);
	unsigned int UnsignedInt(unsigned int auiMax);

private:
	void GenerateNumbers();
	uint32_t uiStateA[625];
	uint32_t uiIndex;
	bool bInitialized;
};
static_assert(sizeof(BSRandom) == 0x9CC);
