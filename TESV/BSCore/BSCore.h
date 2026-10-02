#pragma once

namespace BSCore
{
	extern thread_local constinit int iThreadInitEpochS;
	void InitThreadHeader(int* apGuard);
	void InitThreadFooter(int* apGuard);
	void InitThreadAbort(int* apGuard);
}
