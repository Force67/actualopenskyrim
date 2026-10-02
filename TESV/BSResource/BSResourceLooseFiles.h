#pragma once

#include "BSResource/BSResourceLocation.h"
#include "BSSystem/BSFixedString.h"

namespace BSResource
{
	class LooseFileLocation : public Location
	{
	public:
		~LooseFileLocation() override;
		ErrorCode DoCreateStream(const char*, BSTSmartPointer<Stream>&, Location*&, bool) override;
		ErrorCode DoCreateAsyncStream(const char*, BSTSmartPointer<AsyncStream>&, Location*&, bool) override;
		ErrorCode DoTraversePrefix(const char*, LocationTraverser&) override;
		ErrorCode DoGetInfo(const char*, Info&) override;
		ErrorCode DoGetInfo(const char*, Info&, Location*&) override;
		ErrorCode DoDelete(const char*) override;
		const char* DoGetName() const override;
		unsigned int DoGetMinimumAsyncPacketSize() const override;
		ErrorCode CreateStreamFromNames(const char*, const char*, BSTSmartPointer<Stream>&, Location*&, bool);
		ErrorCode CreateAsyncStreamFromNames(const char*, const char*, BSTSmartPointer<AsyncStream>&, Location*&, bool);
		static void BuildCannonicalNames(const char* apPath, char* apDirectory, char* apFile);
		static bool FileExists(const char* apPath, uint64_t& arSize);
		static bool DirectoryExists(const char* apPath);
		bool GetLocationFreeSpace(uint64_t& arFreeSpace) const;

		BSFixedString Prefix;
		unsigned int uiMinimumAsyncPacketSize;
		bool bAsyncSupported;
	};

	struct TranslatedPath
	{
		static void TranslatePath(const char* apPath, char* apBuffer, const char*& arFile, const char*& arEnd);
	};

	void TraversePath(const char* apPath, const char* apRelativePath, char* apEnd, LocationTraverser& arTraverser, LooseFileLocation& arLocation);
	static_assert(sizeof(LooseFileLocation) == 32);
	static_assert(offsetof(LooseFileLocation, Prefix) == 16);
	static_assert(offsetof(LooseFileLocation, uiMinimumAsyncPacketSize) == 24);
	static_assert(offsetof(LooseFileLocation, bAsyncSupported) == 28);
}
