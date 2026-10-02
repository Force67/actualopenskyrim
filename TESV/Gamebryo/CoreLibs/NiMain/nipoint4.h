#pragma once

// A homogeneous row of the engine matrix type in some builds; kept for
// layouts that embed it.
class NiPoint4
{
public:
	union
	{
		struct
		{
			float x;
			float y;
			float z;
			float w;
		} v;
		float m_pt[4];
	};
};
static_assert(sizeof(NiPoint4) == 0x10);
