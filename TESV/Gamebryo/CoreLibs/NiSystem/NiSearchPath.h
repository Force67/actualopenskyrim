#pragma once

class NiSearchPath
{
public:
	using CallbackPtr = NiSearchPath* (*)();

	NiSearchPath();
	virtual ~NiSearchPath();
	void SetFilePath(const char* pcFilePath);
	void SetReferencePath(const char* pcReferencePath);
	virtual void Reset();
	virtual bool GetNextSearchPath(char* pcPath, unsigned int uiStringLen);

	// Lets engine code create the BSSystem search path without linking BSSystem.
	static CallbackPtr spSearchPathCreateCallback;
	static char ms_acDefPath[260];

protected:
	unsigned int m_uiNextPath;
	char m_acFilePath[260];
	char m_acReferencePath[260];
};
static_assert(sizeof(NiSearchPath) == 536);
