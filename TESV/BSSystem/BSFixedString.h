#pragma once

class BSFixedString
{
public:
	BSFixedString() : pString(nullptr) {}
	explicit BSFixedString(const char* apString);
	BSFixedString(const BSFixedString& arOther);
	BSFixedString(BSFixedString&& arOther);
	~BSFixedString();
	const BSFixedString& operator<<(const char* apString);
	const BSFixedString& operator=(const BSFixedString& arOther);
	const BSFixedString& operator=(BSFixedString&& arOther);
	bool operator!() const;
	unsigned int QLength() const;
	static char* CreateDelimited(BSFixedString& arString, char* apSource, const char* apDelimiters);
	static void CreatePreserveCase(BSFixedString& arString, const char* apSource);
	static void DestroyEmptyString();
	static const char* pEmptyStringS;
	static int iEmptyStringInitS;

	const char* pString;
};
static_assert(sizeof(BSFixedString) == 8);

bool operator==(const BSFixedString& arFirst, const char* apSecond);
bool operator!=(const BSFixedString& arFirst, const char* apSecond);
bool operator==(const char* apFirst, const BSFixedString& arSecond);
bool operator!=(const char* apFirst, const BSFixedString& arSecond);

class BSFixedStringW
{
public:
	explicit BSFixedStringW(const wchar_t* apString);
	BSFixedStringW(const BSFixedStringW& arOther);
	~BSFixedStringW();
	const BSFixedStringW& operator<<(const wchar_t* apString);
	const BSFixedStringW& operator=(const BSFixedStringW& arOther);
	operator const wchar_t*() const;
	static void CreatePreserveCase(BSFixedStringW& arString, const wchar_t* apSource);
	static void DestroyEmptyString();
	static const wchar_t* pEmptyStringS;
	static int iEmptyStringInitS;

	const wchar_t* pString;
};
static_assert(sizeof(BSFixedStringW) == 8);

bool operator==(const BSFixedStringW& arFirst, const wchar_t* apSecond);
bool operator!=(const BSFixedStringW& arFirst, const wchar_t* apSecond);
bool operator==(const wchar_t* apFirst, const BSFixedStringW& arSecond);
bool operator!=(const wchar_t* apFirst, const BSFixedStringW& arSecond);
