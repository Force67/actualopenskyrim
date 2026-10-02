#pragma once

#include <windows.h>

class NiSemaphore
{
public:
	NiSemaphore() : m_iCount(0), m_iMaxCount(1), m_hSemaphore(CreateSemaphoreA(nullptr, 0, 1, nullptr)) {}
	~NiSemaphore() { CloseHandle(m_hSemaphore); }

	volatile LONG m_iCount;
	int m_iMaxCount;
	HANDLE m_hSemaphore;
};
static_assert(sizeof(NiSemaphore) == 16);
