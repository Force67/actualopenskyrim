#pragma once

#include "BSResource/BSResourceLocation.h"

namespace BSResource
{
	void RegisterLocation(Location* apLocation, unsigned int auiPriority);
	void UnregisterLocation(Location* apLocation);
	void RegisterGlobalPath(const char* apPath);
	void UnregisterGlobalPath(const char* apPath);
	bool RegisterStream(const char* apName, const BSTSmartPointer<Stream>& arStream);
	void Update();
	unsigned int QPathSeparator();
}
