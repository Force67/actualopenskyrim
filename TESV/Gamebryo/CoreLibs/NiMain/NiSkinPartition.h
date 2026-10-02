#pragma once

#include "NiObject.h"
#include <cstddef>

class NiSkinPartition : public NiObject
{
public:
	struct Partition
	{
		uint32_t GetStripLengthSum() const;
		std::byte m_acData[80];
	};
	static const NiRTTI ms_RTTI;
	alignas(8) uint32_t m_uiPartitions;
	Partition* m_pkPartitions;
};
static_assert(sizeof(NiSkinPartition::Partition) == 80);
static_assert(offsetof(NiSkinPartition, m_uiPartitions) == 16);
static_assert(offsetof(NiSkinPartition, m_pkPartitions) == 24);

inline const NiRTTI NiSkinPartition::ms_RTTI("NiSkinPartition", &NiObject::ms_RTTI);
