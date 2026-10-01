#include "BSCore/BSCoreUtils.h"

#include <cstdarg>
#include <cstdio>

DWORD BSCoreUtils::StackTrace::uiFramesS = 32;

void BSCoreUtils::StackTrace::SetCaptureDepth(DWORD aDepth)
{
	uiFramesS = aDepth < 64 ? aDepth : 64;
}

int BSprintf(const char* apFormat, ...)
{
	va_list args;
	va_start(args, apFormat);
	int iResult = std::vprintf(apFormat, args);
	va_end(args);
	return iResult;
}
