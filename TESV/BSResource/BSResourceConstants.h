#pragma once

namespace BSResource
{
	enum ErrorCode : int
	{
		EC_NONE = 0,
		EC_NOT_EXIST = 1,
		EC_INVALID_PATH = 2,
		EC_FILE_ERROR = 3,
		EC_INVALID_TYPE = 4,
		EC_MEMORY_ERROR = 5,
		EC_BUSY = 6,
		EC_INVALID_PARAM = 7,
		EC_UNSUPPORTED = 8,
	};
}
