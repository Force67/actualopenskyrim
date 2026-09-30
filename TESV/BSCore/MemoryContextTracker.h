#pragma once

#include "BSCore/MemoryDefs.h"

// The memory context of the current thread, which picks the heap allocations
// come from.
class MemoryContextTracker
{
public:
	static MEM_CONTEXT GetMemContext() { return eThreadContext; }
	static void SetMemContext(MEM_CONTEXT aeContext) { eThreadContext = aeContext; }

private:
	static thread_local MEM_CONTEXT eThreadContext;
};

// Switches the thread's memory context for a scope.
class AutoMemContext
{
public:
	explicit AutoMemContext(MEM_CONTEXT aeContext) :
		eSaved(MemoryContextTracker::GetMemContext())
	{
		MemoryContextTracker::SetMemContext(aeContext);
	}

	~AutoMemContext() { MemoryContextTracker::SetMemContext(eSaved); }

private:
	MEM_CONTEXT eSaved;
};
