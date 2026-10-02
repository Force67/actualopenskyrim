#include "NiStream.h"

bool NiStream::bUseDefaultPath = false;
const uint32_t NiStream::ms_uiNifMinVersion = 0x04020100;
const uint32_t NiStream::ms_uiNifMaxVersion = 0x14020007;
const uint32_t NiStream::ms_uiNifMinUserDefinedVersion = 0;
const uint32_t NiStream::ms_uiNifMaxUserDefinedVersion = 12;

CRITICAL_SECTION NiStream::ms_kCleanupCriticalSection;

NiTStringPointerMap<NiStream::LoadFunction>* NiStream::ms_pkLoaders = nullptr;
NiTPrimitiveArray<NiStream::PostProcessFunction>* NiStream::ms_pkPostProcessFunctions = nullptr;
