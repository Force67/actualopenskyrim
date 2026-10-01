#include "BSCore/BSMemoryutility.h"

#include <cerrno>
#include <stdlib.h>
#include <cstring>

void BSmemcpy(void* apDestination, size_t auiDestinationSize, const void* apSource, size_t auiCount)
{
	if (!auiCount)
		return;
	if (apDestination && apSource && auiDestinationSize >= auiCount)
	{
		std::memcpy(apDestination, apSource, auiCount);
		return;
	}
	if (apDestination)
		std::memset(apDestination, 0, auiDestinationSize);
	*_errno() = apDestination && apSource ? ERANGE : EINVAL;
	_invalid_parameter_noinfo();
}
