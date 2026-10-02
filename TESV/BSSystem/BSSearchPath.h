#pragma once

#include <cstddef>
#include <cstdint>

#include "Gamebryo/CoreLibs/NiSystem/NiSearchPath.h"

class BSSearchPath : public NiSearchPath
{
public:
	BSSearchPath();

	void Reset() override;
	bool GetNextSearchPath(char* apPath, uint32_t auiStringLen) override;
	bool Access(const char* apPath);

	// Installed as NiSearchPath::spSearchPathCreateCallback by InitCallback.
	static NiSearchPath* GetBSSearchPath();
	static void InitCallback();
};

static_assert(sizeof(BSSearchPath) == 0x218);
