#pragma once

#include <cstdint>

// Memory contexts select the heap for an allocation. Only the ones the memory
// system itself names are listed.
enum MEM_CONTEXT : int32_t
{
	MC_CORE_SYSTEM = 0,
	MC_CORE_STATICVARS = 1,
	MC_CORE_UNKNOWN = 2,

	MC_GB_SYSTEM = 61,

	MEM_CONTEXT_COUNT = 127,
};
