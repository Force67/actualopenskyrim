#include "Gamebryo/CoreLibs/NiSystem/NiSystemSDM.h"
#include "Gamebryo/CoreLibs/NiSystem/NiMemManager.h"
#include "Gamebryo/CoreLibs/NiSystem/NiSystemDesc.h"

bool NiSystemSDM::ms_bInitialized;

void NiSystemSDM::Init()
{
	if (!ms_bInitialized)
	{
		ms_bInitialized = true;
		NiMemManager::_SDMInit();
		NiSystemDesc::InitSystemDesc();
	}
}

void NiSystemSDM::Shutdown()
{
	if (ms_bInitialized)
	{
		ms_bInitialized = false;
		NiSystemDesc::ShutdownSystemDesc();
		NiMemManager::_SDMShutdown();
	}
}
