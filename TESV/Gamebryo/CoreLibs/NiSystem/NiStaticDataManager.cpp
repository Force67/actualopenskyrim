#include "Gamebryo/CoreLibs/NiSystem/NiStaticDataManager.h"
#include "Gamebryo/CoreLibs/NiSystem/NiInitOptions.h"
#include "Gamebryo/CoreLibs/NiSystem/NiSystemSDM.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/MemoryContextTracker.h"

#include <new>

void operator delete(void* apMemory, size_t stSize) noexcept;

const NiInitOptions* NiStaticDataManager::ms_pkInitOptions = nullptr;

NiStaticDataManager::LibraryFunction NiStaticDataManager::ms_pfnRootInitFunction;
NiStaticDataManager::LibraryFunction NiStaticDataManager::ms_pfnRootShutdownFunction;
NiStaticDataManager::LibraryFunction NiStaticDataManager::ms_apfnInitFunctions[16];
NiStaticDataManager::LibraryFunction NiStaticDataManager::ms_apfnShutdownFunctions[16];
unsigned int NiStaticDataManager::ms_uiNumLibraries;
bool NiStaticDataManager::ms_bInitialized;

void NiStaticDataManager::SetRootLibrary(LibraryFunction pfnInit, LibraryFunction pfnShutdown)
{
	ms_pfnRootInitFunction = pfnInit;
	ms_pfnRootShutdownFunction = pfnShutdown;
}

void NiStaticDataManager::AddLibrary(LibraryFunction pfnInit, LibraryFunction pfnShutdown)
{
	unsigned int uiCount = ms_uiNumLibraries;
	ms_apfnInitFunctions[uiCount] = pfnInit;
	ms_apfnShutdownFunctions[uiCount] = pfnShutdown;
	ms_uiNumLibraries = uiCount + 1;
	if (ms_bInitialized)
		pfnInit();
}

void NiStaticDataManager::RemoveLibrary(LibraryFunction pfnInit, LibraryFunction pfnShutdown)
{
	unsigned int uiCount = ms_uiNumLibraries;
	unsigned int uiIndex = 0;
	while (uiIndex < ms_uiNumLibraries)
	{
		if (ms_apfnInitFunctions[uiIndex] == pfnInit && ms_apfnShutdownFunctions[uiIndex] == pfnShutdown)
			break;
		++uiIndex;
	}
	if (ms_bInitialized)
	{
		pfnShutdown();
		uiCount = ms_uiNumLibraries;
	}
	--uiCount;
	for (; uiIndex < uiCount; ++uiIndex)
	{
		ms_apfnInitFunctions[uiIndex] = ms_apfnInitFunctions[uiIndex + 1];
		ms_apfnShutdownFunctions[uiIndex] = ms_apfnShutdownFunctions[uiIndex + 1];
	}
	ms_uiNumLibraries = uiCount;
}

bool NiStaticDataManager::ms_bAutoCreatedInitOptions;

void NiStaticDataManager::Init(const NiInitOptions* pkOptions)
{
	AutoMemContext kContext(MC_GB_SYSTEM);
	const NiInitOptions* pkExistingOptions = ms_pkInitOptions;
	if (!pkOptions && !pkExistingOptions)
	{
		NiInitOptions* pkNewOptions = static_cast<NiInitOptions*>(MemoryManager::Instance().Allocate(sizeof(NiInitOptions), 0, false));
		if (pkNewOptions)
			new (pkNewOptions) NiInitOptions();
		pkOptions = pkNewOptions;
		ms_bAutoCreatedInitOptions = true;
	}
	else
	{
		if (pkExistingOptions)
			pkOptions = pkExistingOptions;
		ms_bAutoCreatedInitOptions = false;
	}
	ms_pkInitOptions = pkOptions;
	NiSystemSDM::Init();
	if (ms_pfnRootInitFunction)
		ms_pfnRootInitFunction();
	for (unsigned int uiIndex = 0; uiIndex < ms_uiNumLibraries; ++uiIndex)
		ms_apfnInitFunctions[uiIndex]();
	ms_bInitialized = true;
}

void NiStaticDataManager::Shutdown()
{
	for (unsigned int uiIndex = 0; uiIndex < ms_uiNumLibraries; ++uiIndex)
		ms_apfnShutdownFunctions[uiIndex]();
	if (ms_pfnRootShutdownFunction)
		ms_pfnRootShutdownFunction();
	NiSystemSDM::Shutdown();
	if (ms_bAutoCreatedInitOptions)
	{
		const NiInitOptions* pkOptions = ms_pkInitOptions;
		if (pkOptions)
		{
			pkOptions->~NiInitOptions();
			operator delete(const_cast<NiInitOptions*>(pkOptions), sizeof(NiInitOptions));
		}
	}
	ms_bInitialized = false;
}
