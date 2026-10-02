#include "BSSystem/BSStorage.h"

#include <cstdlib>

BSStorage::~BSStorage()
{
	if (upStreamBuffer)
	{
		upStreamBuffer->Buffer.~AutoScrapBuffer();
		operator delete(upStreamBuffer, sizeof(BSStorageDefs::StreamBuffer));
	}
}
