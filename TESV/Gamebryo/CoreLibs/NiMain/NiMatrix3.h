#pragma once

class NiMatrix3
{
public:
	float m_pEntry[3][3];
};
static_assert(sizeof(NiMatrix3) == 0x24);
