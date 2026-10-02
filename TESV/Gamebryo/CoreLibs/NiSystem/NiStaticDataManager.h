#pragma once

class NiInitOptions;

class NiStaticDataManager
{
public:
	using LibraryFunction = void (*)();
	static void Init(const NiInitOptions* pkOptions);
	static void Shutdown();
	static void SetRootLibrary(LibraryFunction pfnInit, LibraryFunction pfnShutdown);
	static void AddLibrary(LibraryFunction pfnInit, LibraryFunction pfnShutdown);
	static void RemoveLibrary(LibraryFunction pfnInit, LibraryFunction pfnShutdown);

	static const NiInitOptions* GetInitOptions() { return ms_pkInitOptions; }
	static const NiInitOptions* ms_pkInitOptions;

private:
	static LibraryFunction ms_pfnRootInitFunction;
	static LibraryFunction ms_pfnRootShutdownFunction;
	static LibraryFunction ms_apfnInitFunctions[16];
	static LibraryFunction ms_apfnShutdownFunctions[16];
	static unsigned int ms_uiNumLibraries;
	static bool ms_bInitialized;
	static bool ms_bAutoCreatedInitOptions;
};
