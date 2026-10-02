#include "BSSystem/BSSystemFile.h"

namespace
{
	BSSystemFile::ErrorCode TranslateLastError()
	{
		return BSSystemFile::TranslateErrorCode(GetLastError());
	}
}

BSSystemFile::ErrorCode BSSystemFile::DoOpen(const char* pFileName, AccessMode eAccessMode, OpenMode eOpenMode)
{
	DWORD uAccess = 0;
	switch (eAccessMode)
	{
	case AM_RDONLY:
		uAccess = GENERIC_READ;
		break;
	case AM_RDWR:
		uAccess = GENERIC_READ | GENERIC_WRITE;
		break;
	case AM_WRONLY:
		uAccess = GENERIC_WRITE;
		break;
	default:
		break;
	}

	DWORD uDisposition = OPEN_EXISTING;
	switch (eOpenMode)
	{
	case OM_CREAT:
		uDisposition = CREATE_ALWAYS;
		break;
	case OM_TRUNC:
		uDisposition = TRUNCATE_EXISTING;
		break;
	case OM_EXCL:
		uDisposition = CREATE_NEW;
		break;
	default:
		break;
	}

	DWORD uAttributes = ((static_cast<int32_t>(uiFlags) >> 31) & 0x60000000) + FILE_FLAG_SEQUENTIAL_SCAN;
	hFile = CreateFileA(pFileName, uAccess, FILE_SHARE_READ, nullptr, uDisposition, uAttributes, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		if (eOpenMode == OM_APPEND)
		{
			hFile = CreateFileA(pFileName, uAccess, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, uAttributes, nullptr);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				ErrorCode eResult = TranslateLastError();
				if (eResult != EC_NONE)
					return eResult;
			}
		}
		else
		{
			ErrorCode eResult = TranslateLastError();
			if (eResult != EC_NONE)
				return eResult;
		}
	}

	if (eOpenMode == OM_APPEND && static_cast<int32_t>(uiFlags) >= 0)
	{
		LARGE_INTEGER liNewPosition;
		LARGE_INTEGER liZero;
		liZero.QuadPart = 0;
		if (SetFilePointerEx(hFile, liZero, &liNewPosition, FILE_END))
			return EC_NONE;
		return TranslateLastError();
	}
	return EC_NONE;
}

BSSystemFile::ErrorCode BSSystemFile::DoRead(void* apvBuffer, uint32_t auiBytes, uint64_t* apuiBytesRead)
{
	*apuiBytesRead = 0;
	if (static_cast<int32_t>(uiFlags) < 0)
		return EC_NOT_OPEN;
	DWORD uBytesRead = 0;
	if (ReadFile(hFile, apvBuffer, auiBytes, &uBytesRead, nullptr))
	{
		*apuiBytesRead = uBytesRead;
		return EC_NONE;
	}
	return TranslateLastError();
}

BSSystemFile::ErrorCode BSSystemFile::DoWrite(const void* apvBuffer, uint32_t auiBytes, uint64_t* apuiBytesWritten)
{
	*apuiBytesWritten = 0;
	if (static_cast<int32_t>(uiFlags) < 0)
		return EC_NOT_OPEN;
	DWORD uBytesWritten = 0;
	if (WriteFile(hFile, apvBuffer, auiBytes, &uBytesWritten, nullptr))
	{
		*apuiBytesWritten = uBytesWritten;
		return EC_NONE;
	}
	return TranslateLastError();
}

BSSystemFile::ErrorCode BSSystemFile::DoSeek(int64_t aiOffset, SeekMode eSeekMode, int64_t* apoiNewPosition)
{
	if (static_cast<int32_t>(uiFlags) < 0)
		return EC_NOT_OPEN;
	LARGE_INTEGER liDistance;
	liDistance.QuadPart = aiOffset;
	LARGE_INTEGER liNewPosition;
	if (SetFilePointerEx(hFile, liDistance, &liNewPosition, eSeekMode))
	{
		*apoiNewPosition = liNewPosition.QuadPart;
		return EC_NONE;
	}
	return TranslateLastError();
}

BSSystemFile::ErrorCode BSSystemFile::DoGetSize(uint64_t* apuiSize)
{
	LARGE_INTEGER liFileSize;
	if (GetFileSizeEx(hFile, &liFileSize))
	{
		*apuiSize = liFileSize.QuadPart;
		return EC_NONE;
	}
	*apuiSize = 0;
	return EC_NOT_OPEN;
}

BSSystemFile::ErrorCode BSSystemFile::DoSetEndOfFile()
{
	if (SetEndOfFile(hFile))
		return EC_NONE;
	return TranslateLastError();
}

BSSystemFile::ErrorCode BSSystemFile::DoFlush()
{
	if (FlushFileBuffers(hFile))
		return EC_NONE;
	return EC_NOT_OPEN;
}

BSSystemFile::ErrorCode BSSystemFile::DoGetInfo(Info* apInfo)
{
	if (!GetFileTime(hFile, &apInfo->CreateTime, &apInfo->AccessTime, &apInfo->ModifyTime))
		return EC_NOT_OPEN;
	LARGE_INTEGER liFileSize;
	if (!GetFileSizeEx(hFile, &liFileSize))
	{
		apInfo->uiFileSize = 0;
		return EC_NOT_OPEN;
	}
	apInfo->uiFileSize = liFileSize.QuadPart;
	return EC_NONE;
}

bool BSSystemFile::DoClose()
{
	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
		return true;
	}
	return false;
}

namespace
{
	void CompletionEntryPoint(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		if (auto* pFunctor = static_cast<BSSystemFileAsyncFunctor*>(lpOverlapped->hEvent))
		{
			pFunctor->Process(BSSystemFile::TranslateErrorCode(dwErrorCode), dwNumberOfBytesTransfered);
			pFunctor->uiStatus = 1;
		}
	}
}

BSSystemFile::ErrorCode BSSystemFile::DoReadAsync(void* apvBuffer, uint32_t auiBytes, int64_t aiOffset,
	BSSystemFileAsyncFunctor* apFunctor)
{
	if (static_cast<int32_t>(uiFlags) >= 0)
		return EC_NOT_OPEN;
	apFunctor->Overlapped.Pointer = reinterpret_cast<void*>(aiOffset);
	apFunctor->Overlapped.hEvent = apFunctor;
	apFunctor->uiThreadId = GetCurrentThreadId();
	if (ReadFileEx(hFile, apvBuffer, auiBytes, &apFunctor->Overlapped, CompletionEntryPoint))
	{
		apFunctor->uiStatus = -1;
		return EC_NONE;
	}
	return TranslateLastError();
}

BSSystemFile::ErrorCode BSSystemFile::DoWriteAsync(const void* apvBuffer, uint32_t auiBytes, int64_t aiOffset,
	BSSystemFileAsyncFunctor* apFunctor)
{
	if (static_cast<int32_t>(uiFlags) >= 0)
		return EC_NOT_OPEN;
	apFunctor->Overlapped.Pointer = reinterpret_cast<void*>(aiOffset);
	apFunctor->Overlapped.hEvent = apFunctor;
	apFunctor->uiThreadId = GetCurrentThreadId();
	if (WriteFileEx(hFile, apvBuffer, auiBytes, &apFunctor->Overlapped, CompletionEntryPoint))
	{
		apFunctor->uiStatus = -1;
		return EC_NONE;
	}
	return TranslateLastError();
}

BSSystemFile::ErrorCode BSSystemFile::Delete(const char* pFileName)
{
	if (DeleteFileA(pFileName))
		return EC_NONE;
	return EC_NOT_OPEN;
}

BSSystemFile::ErrorCode BSSystemFile::Copy(const char* pFrom, const char* pTo, bool abFailIfExists)
{
	if (CopyFileA(pFrom, pTo, abFailIfExists))
		return EC_NONE;
	return EC_NOT_OPEN;
}

BSSystemFile::ErrorCode BSSystemFile::Move(const char* pFrom, const char* pTo, bool abNoOverwrite)
{
	if (MoveFileA(pFrom, pTo))
		return EC_NONE;
	if (abNoOverwrite)
		return EC_NOT_OPEN;
	if (!DeleteFileA(pTo))
		return EC_NOT_OPEN;
	if (MoveFileA(pFrom, pTo))
		return EC_NONE;
	return TranslateLastError();
}

BSSystemFile::ErrorCode BSSystemFile::SetTime(const char* pFileName, const FILETIME* apCreationTime,
	const FILETIME* apAccessTime, const FILETIME* apModifyTime)
{
	BSSystemFile file(pFileName, AM_RDWR, OM_NONE, false);
	ErrorCode eResult = file.GetErrorCode();
	if (eResult == EC_NONE && !SetFileTime(file.hFile, apCreationTime, apAccessTime, apModifyTime))
		eResult = TranslateLastError();
	return eResult;
}


BSSystemFile::ErrorCode BSSystemFile::TranslateErrorCode(DWORD uError)
{
	switch (uError)
	{
	case 0:
	case 109:
		return EC_NONE;
	case 2:
		return EC_NOT_EXIST;
	case 3:
		return EC_INVALID_PATH;
	case 19:
		return EC_NO_PERMISSION;
	case 183:
		return EC_ALREADY_EXISTS;
	default:
		return EC_NOT_OPEN;
	}
}
