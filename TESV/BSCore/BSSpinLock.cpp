#include "BSCore/BSSpinLock.h"

BSSpinLock::BSSpinLock() : OwningThread(0), uiLockCount(0)
{
}

BSSpinLock::~BSSpinLock()
{
}
