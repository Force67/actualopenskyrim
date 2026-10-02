#include "BSCore/CompactingStoreCommon.h"

#include <cstdlib>

using namespace CompactingStore;

int NoopMoveCallback::InitializeInstance()
{
	return std::atexit(DestroyInstance);
}

void NoopMoveCallback::DestroyInstance()
{
	instance.NoopMoveCallback::~NoopMoveCallback();
}
