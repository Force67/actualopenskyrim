#pragma once

class NiThreadProcedure
{
public:
	NiThreadProcedure();
	virtual ~NiThreadProcedure();
	virtual unsigned int ThreadProcedure(void* pvArg);
};
static_assert(sizeof(NiThreadProcedure) == 8);
