#pragma once

class NiSystemSDM
{
public:
	static void Init();
	static void Shutdown();

protected:
	static bool ms_bInitialized;
};
