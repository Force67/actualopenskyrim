#pragma once

#include <windows.h>
#include <cstddef>
#include <cstdint>

namespace BSCoreUtils
{
	struct StackTrace
	{
		static void SetCaptureDepth(DWORD aDepth);
		static DWORD uiFramesS;

		unsigned int uiNumFrames;
		uint64_t FramesA[64];
	};
	static_assert(sizeof(StackTrace) == 0x208);
	static_assert(offsetof(StackTrace, FramesA) == 8);
}

int BSprintf(const char* apFormat, ...);
