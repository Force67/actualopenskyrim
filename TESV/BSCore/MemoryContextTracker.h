#pragma once

#include "BSCore/MemoryDefs.h"

extern thread_local constinit MEM_CONTEXT etMemContextS;

inline MEM_CONTEXT QMemContext() { return etMemContextS; }
inline void SetMemContext(MEM_CONTEXT aeContext) { etMemContextS = aeContext; }

class AutoMemContext
{
public:
	explicit AutoMemContext(MEM_CONTEXT aeContext, bool = true, const char* = nullptr)
	{
		Enter(aeContext);
	}
	~AutoMemContext() { Leave(); }

	void Enter(MEM_CONTEXT aeContext, bool = true, const char* = nullptr)
	{
		iOldMemContext = QMemContext();
		SetMemContext(aeContext);
	}
	void Leave() { SetMemContext(iOldMemContext); }

private:
	MEM_CONTEXT iOldMemContext;
};
static_assert(sizeof(AutoMemContext) == 4);
