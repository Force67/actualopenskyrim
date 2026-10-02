#pragma once

#include <cstdint>

#include "BSSystem/BSStringT.h"

// Pure path helpers shared by the file layer. NormPath only treats '\\' as a
// separator in its dot handling, not '/'.
namespace FilePathUtilities
{
	bool Join(const char* apDirPathStart, const char* apDirPathEnd, BSStaticStringT<260>& arsReturnPath);
	bool SplitPath(const char* apFilePath, BSStaticStringT<260>& arsDirName, BSStaticStringT<260>& arsFileName);
	bool SplitExt(const char* apFilePath, BSStaticStringT<260>& arsRootName, BSStaticStringT<260>& arsExtension);
	bool NormPath(const char* apFilePath, BSStaticStringT<260>& arsNormPath, bool abUseBackslashes);
	uint32_t FindLastChar(const BSStaticStringT<260>& arsPath, char cChar);
	uint32_t FindLastSlash(const char* apDirPath);
}
