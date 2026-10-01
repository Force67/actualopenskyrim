#pragma once

class BSFixedString
{
public:
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

	const wchar_t* pString;
};
static_assert(sizeof(BSFixedStringW) == 8);

bool operator==(const BSFixedStringW& arFirst, const wchar_t* apSecond);
bool operator!=(const BSFixedStringW& arFirst, const wchar_t* apSecond);
bool operator==(const wchar_t* apFirst, const BSFixedStringW& arSecond);
bool operator!=(const wchar_t* apFirst, const BSFixedStringW& arSecond);
