#pragma once

#include <cstddef>
#include <cstdint>

enum NiMemEventType : int32_t
{
	NI_UNKNOWN,
	NI_OPER_NEW,
	NI_OPER_NEW_ARRAY,
	NI_OPER_DELETE,
	NI_OPER_DELETE_ARRAY,
	NI_MALLOC,
	NI_REALLOC,
	NI_ALIGNEDMALLOC,
	NI_ALIGNEDREALLOC,
	NI_FREE,
	NI_ALIGNEDFREE,
	NI_EXTERNAL_MALLOC,
	NI_EXTERNAL_REALLOC,
	NI_EXTERNAL_ALIGNEDMALLOC,
	NI_EXTERNAL_ALGINEDREALLOC,
	NI_EXTERNAL_FREE,
	NI_EXTERNAL_ALIGNEDFREE,
};

void* _NiMalloc(size_t stSizeInBytes);
void* _NiAlignedMalloc(size_t stSizeInBytes, size_t stAlignment);
void* _NiRealloc(void* pvMemory, size_t stSizeInBytes);
void* _NiAlignedRealloc(void* pvMemory, size_t stSizeInBytes, size_t stAlignment);
void* _NiExternalMalloc(size_t stSizeInBytes);
void* _NiExternalAlignedMalloc(size_t stSizeInBytes, size_t stAlignment);
void* _NiExternalRealloc(void* pvMemory, size_t stSizeInBytes);
void* _NiExternalAlignedRealloc(void* pvMemory, size_t stSizeInBytes, size_t stAlignment);
void _NiFree(void* pvMemory);
void _NiAlignedFree(void* pvMemory);
void _NiExternalFree(void* pvMemory);
void _NiExternalAlignedFree(void* pvMemory);
