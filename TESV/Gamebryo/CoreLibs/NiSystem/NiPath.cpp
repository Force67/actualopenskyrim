#include "Gamebryo/CoreLibs/NiSystem/NiPath.h"

#include <windows.h>
#include <direct.h>

#include <cctype>
#include <cstdint>
#include <cstring>

bool NiPath::IsRelative(char* pcPath)
{
	if (std::strlen(pcPath) < 2)
		return true;
	if (*pcPath == '\\' || *pcPath == '/')
		return false;
	unsigned char ucDrive = static_cast<unsigned char>(std::toupper(*pcPath) - 'A');
	return pcPath[1] != ':' || ucDrive > 25;
}

char* NiPath::StripAbsoluteBase(const char* pcAbsolutePath)
{
	if (!_strnicmp(pcAbsolutePath, "\\\\", 2))
		return reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(std::strchr(pcAbsolutePath + 2, '\\')) + 1);
	if (!_strnicmp(pcAbsolutePath, "\\", 1))
		return const_cast<char*>(pcAbsolutePath + 1);
	unsigned char ucDrive = static_cast<unsigned char>(std::toupper(*pcAbsolutePath) - 'A');
	if (pcAbsolutePath[1] == ':' && ucDrive <= 25)
		return const_cast<char*>(pcAbsolutePath + (pcAbsolutePath[2] == '\\' ? 3 : 2));
	return const_cast<char*>(pcAbsolutePath);
}

bool NiPath::Standardize(char*)
{
	return false;
}

void NiPath::ReplaceInvalidFilenameCharacters(char* pcFilename, char cReplacement)
{
	size_t uiLength = std::strlen(pcFilename);
	for (size_t i = 0; i < uiLength; ++i)
	{
		switch (pcFilename[i])
		{
		case '"':
		case '*':
		case '/':
		case ':':
		case '<':
		case '>':
		case '?':
		case '\\':
		case '|':
			pcFilename[i] = cReplacement;
			break;
		default:
			break;
		}
	}
}

void NiPath::RemoveSlashDotSlash(char* pcPath)
{
	while (char* pcMatch = std::strstr(pcPath, "\\.\\"))
		strcpy_s(pcMatch, std::strlen(pcMatch), pcMatch + 2);
}

bool NiPath::IsUniqueAbsolute(char* pcPath)
{
	if (IsRelative(pcPath))
		return false;
	const char* pcBody = StripAbsoluteBase(pcPath);
	if (std::strstr(pcBody, ".."))
		return false;
	char acPath[260];
	strcpy_s(acPath, sizeof(acPath), pcBody);
	return true;
}

void NiPath::RemoveDotDots(char* pcPath)
{
	RemoveSlashDotSlash(pcPath);
	if (IsRelative(pcPath))
	{
		while (*pcPath == '.' || *pcPath == '\\')
			++pcPath;
	}
	else
	{
		pcPath = StripAbsoluteBase(pcPath);
	}
	if (!pcPath)
		return;
	char* pcEnd = pcPath + std::strlen(pcPath) + 1;
	while (char* pcMatch = std::strstr(pcPath, "\\.."))
	{
		*pcMatch = 0;
		char* pcPrevious = std::strrchr(pcPath, '\\');
		char* pcSource = pcMatch + 3;
		if (!pcPrevious)
		{
			pcPrevious = pcPath;
			++pcSource;
		}
		for (int32_t i = 0; pcSource + i < pcEnd; ++i)
			pcPrevious[i] = pcSource[i];
	}
}

size_t NiPath::ConvertToRelative(char* pcRelativePath, size_t stRelBytes, const char* pcAbsolutePath, const char* pcRelativeToHere)
{
	size_t stLength = std::strlen(pcAbsolutePath);
	if (stRelBytes <= stLength || stLength >= 260)
		return 0;
	char acAbsolute[260];
	char acBase[260];
	char acCommon[260];
	strcpy_s(acAbsolute, sizeof(acAbsolute), pcAbsolutePath);
	strcpy_s(acBase, sizeof(acBase), pcRelativeToHere);
	if (acBase[std::strlen(acBase) - 1] != '\\')
		strcat_s(acBase, sizeof(acBase), "\\");
	RemoveDotDots(acAbsolute);
	RemoveDotDots(acBase);
	const char* pcAbsolute = StripAbsoluteBase(acAbsolute);
	const char* pcBase = StripAbsoluteBase(acBase);
	if (pcAbsolute - acAbsolute != pcBase - acBase || _strnicmp(acAbsolute, acBase, pcAbsolute - acAbsolute))
		return 0;
	strcpy_s(pcRelativePath, stRelBytes, ".\\");
	strcpy_s(acCommon, sizeof(acCommon), pcBase);
	size_t stCommonLength = std::strlen(acCommon);
	if (stCommonLength && acCommon[stCommonLength - 1] == '\\')
		acCommon[stCommonLength - 1] = 0;
	while (_strnicmp(pcAbsolute, acCommon, std::strlen(acCommon)))
	{
		strcat_s(pcRelativePath, stRelBytes, "..\\");
		char* pcSlash = std::strrchr(acCommon, '\\');
		if (!pcSlash)
		{
			acCommon[0] = 0;
			break;
		}
		*pcSlash = 0;
	}
	stCommonLength = std::strlen(acCommon);
	if (pcAbsolute[stCommonLength] == '\\')
		++stCommonLength;
	strcat_s(pcRelativePath, stRelBytes, pcAbsolute + stCommonLength);
	return std::strlen(pcRelativePath);
}

size_t NiPath::ConvertToAbsolute(char* pcPath, size_t stPathBytes, const char* pcRelativeToHere)
{
	char acAbsolute[522];
	size_t stBaseLength = std::strlen(pcRelativeToHere);
	bool bAddSlash = pcRelativeToHere[stBaseLength - 1] != '\\';
	size_t stLength = 0;
	if (std::strlen(pcPath) + stBaseLength + bAddSlash + 1 <= sizeof(acAbsolute))
	{
		strcpy_s(acAbsolute, sizeof(acAbsolute), pcRelativeToHere);
		if (bAddSlash)
			strcat_s(acAbsolute, sizeof(acAbsolute), "\\");
		strcat_s(acAbsolute, sizeof(acAbsolute), pcPath);
		RemoveDotDots(acAbsolute);
		stLength = std::strlen(acAbsolute);
	}
	else
	{
		acAbsolute[0] = 0;
	}
	if (stLength >= stPathBytes)
	{
		*pcPath = 0;
		return 0;
	}
	strcpy_s(pcPath, stPathBytes, acAbsolute);
	return stLength;
}

size_t NiPath::ConvertToAbsolute(char* pcPath, size_t stPathBytes)
{
	char acBase[260];
	if (!GetCurrentWorkingDirectory(acBase, sizeof(acBase)))
		return 0;
	return ConvertToAbsolute(pcPath, stPathBytes, acBase);
}

size_t NiPath::ConvertToAbsolute(char* pcAbsolutePath, size_t stAbsBytes, const char* pcRelativePath, const char* pcRelativeToHere)
{
	size_t stBaseLength = std::strlen(pcRelativeToHere);
	bool bAddSlash = pcRelativeToHere[stBaseLength - 1] != '\\';
	if (stAbsBytes < std::strlen(pcRelativePath) + stBaseLength + bAddSlash + 1)
	{
		if (stAbsBytes)
			*pcAbsolutePath = 0;
		return 0;
	}
	strcpy_s(pcAbsolutePath, stAbsBytes, pcRelativeToHere);
	if (bAddSlash)
		strcat_s(pcAbsolutePath, stAbsBytes, "\\");
	strcat_s(pcAbsolutePath, stAbsBytes, pcRelativePath);
	RemoveDotDots(pcAbsolutePath);
	return std::strlen(pcAbsolutePath);
}

bool NiPath::GetCurrentWorkingDirectory(char* pcPath, size_t stDestSize)
{
	return _getcwd(pcPath, static_cast<int>(stDestSize)) != nullptr;
}

bool NiPath::GetExecutableDirectory(char* pcPath, size_t stDestSize)
{
	HMODULE hModule = GetModuleHandleA(nullptr);
	DWORD uiLength = GetModuleFileNameA(hModule, pcPath, static_cast<DWORD>(stDestSize));
	if (!uiLength || uiLength == stDestSize)
		return false;
	char* pcSlash = std::strrchr(pcPath, '\\');
	if (!pcSlash)
		pcSlash = std::strrchr(pcPath, '/');
	if (!pcSlash)
		return false;
	pcSlash[1] = 0;
	Standardize(pcPath);
	return true;
}
