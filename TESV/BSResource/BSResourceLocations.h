#pragma once

#include "BSResource/BSResourceLocation.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSTArray.h"
#include "BSSystem/BSFixedString.h"

namespace BSResource
{
	struct LocationTree : Location
	{
		LocationTree(Location* apLhs, Location* apRhs);
		~LocationTree() override;
		ErrorCode DoCreateStream(const char*, BSTSmartPointer<Stream>&, Location*&, bool) override;
		ErrorCode DoCreateAsyncStream(const char*, BSTSmartPointer<AsyncStream>&, Location*&, bool) override;
		ErrorCode DoTraversePrefix(const char*, LocationTraverser&) override;
		ErrorCode DoGetInfo(const char*, Info&) override;
		ErrorCode DoGetInfo(const char*, Info&, Location*&) override;
		ErrorCode DoDelete(const char*) override;

		Location* pLhs;
		Location* pRhs;
	};

	class GlobalLocations : public Location
	{
	public:
		struct Entry
		{
			Entry* pNext;
			Location* pLocation;
			unsigned int uiPriority;
		};

		GlobalLocations() : pHead(nullptr), pPendingMount(nullptr), pFree(nullptr) {}
		~GlobalLocations() override;
		void Register(Location* apLocation, unsigned int auiPriority);
		void Insert(Location* apLocation, unsigned int auiPriority);
		void FlushPendingMounts();
		void Unregister(Location* apLocation);
		ErrorCode DoMount() override;
		void DoUnmount() override;
		ErrorCode DoCreateStream(const char*, BSTSmartPointer<Stream>&, Location*&, bool) override;
		ErrorCode DoCreateAsyncStream(const char*, BSTSmartPointer<AsyncStream>&, Location*&, bool) override;
		ErrorCode DoTraversePrefix(const char*, LocationTraverser&) override;
		ErrorCode DoGetInfo(const char*, Info&) override;
		ErrorCode DoGetInfo(const char*, Info&, Location*&) override;
		ErrorCode DoDelete(const char*) override;

		char cSingleton[4];
		BSSpinLock Lock;
		Entry* pHead;
		Entry* pPendingMount;
		Entry* pFree;
	};

	class GlobalPaths : public Location
	{
	public:
		GlobalPaths() : NamesA(), pRootLocation(nullptr) {}
		~GlobalPaths() override;
		void AddPath(const char* apPath);
		void RemovePath(const char* apPath);
		void MakeName(BSFixedString& arName, const char* apPath);
		void SetRootLocation(Location* apLocation) { pRootLocation = apLocation; }
		ErrorCode DoCreateStream(const char*, BSTSmartPointer<Stream>&, Location*&, bool) override;
		ErrorCode DoCreateAsyncStream(const char*, BSTSmartPointer<AsyncStream>&, Location*&, bool) override;
		ErrorCode DoTraversePrefix(const char*, LocationTraverser&) override;
		ErrorCode DoGetInfo(const char*, Info&) override;
		ErrorCode DoGetInfo(const char*, Info&, Location*&) override;
		ErrorCode DoDelete(const char*) override;

		char cSingleton[8];
		union { BSTArray<BSFixedString> NamesA; };
		Location* pRootLocation;
	};

	extern GlobalLocations* pGlobalLocations;
	extern GlobalPaths* pGlobalPaths;

	static_assert(sizeof(LocationTree) == 32);
	static_assert(offsetof(LocationTree, pLhs) == 16);
	static_assert(offsetof(LocationTree, pRhs) == 24);
	static_assert(sizeof(GlobalLocations::Entry) == 24);
	static_assert(offsetof(GlobalLocations::Entry, pNext) == 0);
	static_assert(offsetof(GlobalLocations::Entry, pLocation) == 8);
	static_assert(offsetof(GlobalLocations::Entry, uiPriority) == 16);
	static_assert(sizeof(GlobalLocations) == 56);
	static_assert(offsetof(GlobalLocations, Lock) == 20);
	static_assert(offsetof(GlobalLocations, pHead) == 32);
	static_assert(offsetof(GlobalLocations, pPendingMount) == 40);
	static_assert(offsetof(GlobalLocations, pFree) == 48);
	static_assert(sizeof(GlobalPaths) == 56);
	static_assert(offsetof(GlobalPaths, NamesA) == 24);
	static_assert(offsetof(GlobalPaths, pRootLocation) == 48);
}
