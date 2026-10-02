#include "BSResource/BSResourceCacheDrive.h"

namespace BSResource
{
	bool CacheDrive::SetupDrive(bool) { return false; }
	bool CacheDrive::GetShouldValidate(uint64_t) { return false; }
	void CacheDrive::PostValidate(uint64_t) {}
	void CacheDrive::FlushCache() {}


	bool CacheDrive::Task::InfoEqual(const Info& arFirst, const Info& arSecond)
	{
		FILETIME LocalTime;
		SYSTEMTIME First;
		SYSTEMTIME Second;
		FileTimeToLocalFileTime(&arFirst.ModifyTime, &LocalTime);
		FileTimeToSystemTime(&LocalTime, &First);
		FileTimeToLocalFileTime(&arSecond.ModifyTime, &LocalTime);
		FileTimeToSystemTime(&LocalTime, &Second);
		return First.wYear == Second.wYear && First.wMonth == Second.wMonth &&
			First.wDay == Second.wDay && First.wHour == Second.wHour && First.wMinute == Second.wMinute;
	}
}
