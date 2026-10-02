#include "BSResource/BSResourceLocations.h"
#include "BSResource/BSResource.h"
#include "BSCore/MemoryContextTracker.h"
#include "BSCore/BSAutoLock.h"
#include "BSCore/MemoryManager.h"

#include <cstring>
#include <new>

namespace BSResource
{
	GlobalLocations* pGlobalLocations;
	GlobalPaths* pGlobalPaths;

	LocationTree::LocationTree(Location* apLhs, Location* apRhs) : pLhs(apLhs), pRhs(apRhs) {}

	LocationTree::~LocationTree() = default;

	static inline void ClearList(GlobalLocations::Entry* apList)
	{
		while (apList)
		{
			GlobalLocations::Entry* pEntry = apList;
			apList = pEntry->pNext;
			delete pEntry;
		}
	}

	GlobalLocations::~GlobalLocations()
	{
		ClearList(pHead);
		ClearList(pPendingMount);
		ClearList(pFree);
		pGlobalLocations = nullptr;
	}

	GlobalPaths::~GlobalPaths()
	{
		NamesA.~BSTArray();
		pGlobalPaths = nullptr;
	}

	template <class Functor>
	static inline ErrorCode TraverseTree(LocationTree& arTree, const char* apPath, bool abCheckPath, Functor aFunctor)
	{
		ErrorCode eResult = EC_NONE;
		if (arTree.pLhs)
		{
			eResult = abCheckPath && !apPath ? EC_INVALID_PATH : aFunctor(arTree.pLhs, apPath);
			if (eResult == EC_NONE)
				return eResult;
		}
		if (arTree.pRhs)
			return abCheckPath && !apPath ? EC_INVALID_PATH : aFunctor(arTree.pRhs, apPath);
		return eResult;
	}

	template <class Functor>
	static inline ErrorCode TraverseLocations(GlobalLocations& arLocations, Functor aFunctor)
	{
		BSAutoLock<BSSpinLock> kLock(arLocations.Lock);
		ErrorCode eResult = EC_NOT_EXIST;
		for (GlobalLocations::Entry* pEntry = arLocations.pHead; pEntry && eResult != EC_NONE; pEntry = pEntry->pNext)
			eResult = aFunctor(pEntry->pLocation);
		return eResult;
	}

	template <class Functor>
	static inline ErrorCode TraversePaths(GlobalPaths& arPaths, const char* apPath, bool abCheckPath, Functor aFunctor)
	{
		char cName[260];
		ErrorCode eResult = EC_NOT_EXIST;
		for (unsigned int uiIndex = 0; uiIndex < arPaths.NamesA.QSize() && eResult != EC_NONE; ++uiIndex)
		{
			strcpy_s(cName, sizeof(cName), arPaths.NamesA[uiIndex].pString);
			strcat_s(cName, sizeof(cName), apPath);
			eResult = aFunctor(arPaths.pRootLocation, cName);
		}
		if (eResult == EC_NONE)
			return eResult;
		return abCheckPath && !apPath ? EC_INVALID_PATH : aFunctor(arPaths.pRootLocation, apPath);
	}

	ErrorCode LocationTree::DoCreateStream(const char* apPath, BSTSmartPointer<Stream>& arStream, Location*& arLocation, bool abReadOnly)
	{
		return TraverseTree(*this, apPath, true, [&](Location* pLocation, const char* pName) { return pLocation->DoCreateStream(pName, arStream, arLocation, abReadOnly); });
	}

	ErrorCode GlobalLocations::DoCreateStream(const char* apPath, BSTSmartPointer<Stream>& arStream, Location*& arLocation, bool abReadOnly)
	{
		return TraverseLocations(*this, [&](Location* pLocation) { return apPath ? pLocation->DoCreateStream(apPath, arStream, arLocation, abReadOnly) : EC_INVALID_PATH; });
	}

	ErrorCode GlobalPaths::DoCreateStream(const char* apPath, BSTSmartPointer<Stream>& arStream, Location*& arLocation, bool abReadOnly)
	{
		return TraversePaths(*this, apPath, true, [&](Location* pLocation, const char* pName) { return pLocation->DoCreateStream(pName, arStream, arLocation, abReadOnly); });
	}

	ErrorCode LocationTree::DoCreateAsyncStream(const char* apPath, BSTSmartPointer<AsyncStream>& arStream, Location*& arLocation, bool abReadOnly)
	{
		return TraverseTree(*this, apPath, true, [&](Location* pLocation, const char* pName) { return pLocation->DoCreateAsyncStream(pName, arStream, arLocation, abReadOnly); });
	}

	ErrorCode GlobalLocations::DoCreateAsyncStream(const char* apPath, BSTSmartPointer<AsyncStream>& arStream, Location*& arLocation, bool abReadOnly)
	{
		return TraverseLocations(*this, [&](Location* pLocation) { return apPath ? pLocation->DoCreateAsyncStream(apPath, arStream, arLocation, abReadOnly) : EC_INVALID_PATH; });
	}

	ErrorCode GlobalPaths::DoCreateAsyncStream(const char* apPath, BSTSmartPointer<AsyncStream>& arStream, Location*& arLocation, bool abReadOnly)
	{
		return TraversePaths(*this, apPath, true, [&](Location* pLocation, const char* pName) { return pLocation->DoCreateAsyncStream(pName, arStream, arLocation, abReadOnly); });
	}

	ErrorCode LocationTree::DoTraversePrefix(const char* apPath, LocationTraverser& arTraverser)
	{
		return TraverseTree(*this, apPath, false, [&](Location* pLocation, const char* pName) { return pLocation->DoTraversePrefix(pName, arTraverser); });
	}

	ErrorCode GlobalLocations::DoTraversePrefix(const char* apPath, LocationTraverser& arTraverser)
	{
		return TraverseLocations(*this, [&](Location* pLocation) { return pLocation->DoTraversePrefix(apPath, arTraverser); });
	}

	ErrorCode GlobalPaths::DoTraversePrefix(const char* apPath, LocationTraverser& arTraverser)
	{
		return TraversePaths(*this, apPath, false, [&](Location* pLocation, const char* pName) { return pLocation->DoTraversePrefix(pName, arTraverser); });
	}

	ErrorCode LocationTree::DoGetInfo(const char* apPath, Info& arInfo, Location*& arLocation)
	{
		return TraverseTree(*this, apPath, false, [&](Location* pLocation, const char* pName) { return pLocation->DoGetInfo(pName, arInfo, arLocation); });
	}

	ErrorCode GlobalLocations::DoGetInfo(const char* apPath, Info& arInfo, Location*& arLocation)
	{
		return TraverseLocations(*this, [&](Location* pLocation) { return pLocation->DoGetInfo(apPath, arInfo, arLocation); });
	}

	ErrorCode GlobalPaths::DoGetInfo(const char* apPath, Info& arInfo, Location*& arLocation)
	{
		return TraversePaths(*this, apPath, false, [&](Location* pLocation, const char* pName) { return pLocation->DoGetInfo(pName, arInfo, arLocation); });
	}

	ErrorCode LocationTree::DoGetInfo(const char* apPath, Info& arInfo)
	{
		return TraverseTree(*this, apPath, false, [&](Location* pLocation, const char* pName) { return pLocation->DoGetInfo(pName, arInfo); });
	}

	ErrorCode GlobalLocations::DoGetInfo(const char* apPath, Info& arInfo)
	{
		return TraverseLocations(*this, [&](Location* pLocation) { return pLocation->DoGetInfo(apPath, arInfo); });
	}

	ErrorCode GlobalPaths::DoGetInfo(const char* apPath, Info& arInfo)
	{
		return TraversePaths(*this, apPath, false, [&](Location* pLocation, const char* pName) { return pLocation->DoGetInfo(pName, arInfo); });
	}

	ErrorCode LocationTree::DoDelete(const char* apPath)
	{
		return TraverseTree(*this, apPath, false, [&](Location* pLocation, const char* pName) { return pLocation->DoDelete(pName); });
	}

	ErrorCode GlobalLocations::DoDelete(const char* apPath)
	{
		return TraverseLocations(*this, [&](Location* pLocation) { return pLocation->DoDelete(apPath); });
	}

	ErrorCode GlobalPaths::DoDelete(const char* apPath)
	{
		return TraversePaths(*this, apPath, false, [&](Location* pLocation, const char* pName) { return pLocation->DoDelete(pName); });
	}

	ErrorCode GlobalLocations::DoMount()
	{
		BSAutoLock<BSSpinLock> kLock(Lock);
		Entry** ppEntry = &pHead;
		while (*ppEntry)
		{
			Entry* pEntry = *ppEntry;
			if (pEntry->pLocation->Mount() != EC_NONE)
			{
				*ppEntry = pEntry->pNext;
				pEntry->pNext = pFree;
				pFree = pEntry;
			}
			else
			{
				ppEntry = &pEntry->pNext;
			}
		}
		return pHead ? EC_NONE : EC_FILE_ERROR;
	}

	void GlobalLocations::DoUnmount()
	{
		BSAutoLock<BSSpinLock> kLock(Lock);
		for (Entry* pEntry = pHead; pEntry; pEntry = pEntry->pNext)
			if (pEntry->pLocation->bMounted)
				pEntry->pLocation->DoUnmount();
	}

	void GlobalLocations::Register(Location* apLocation, unsigned int auiPriority)
	{
		FlushPendingMounts();
		Insert(apLocation, auiPriority);
	}

	void GlobalLocations::Insert(Location* apLocation, unsigned int auiPriority)
	{
		BSAutoLock<BSSpinLock> kLock(Lock);
		Entry* pEntry = nullptr;
		Entry** ppEntry = &pHead;
		while (*ppEntry && (*ppEntry)->uiPriority < auiPriority)
		{
			if ((*ppEntry)->pLocation == apLocation)
			{
				pEntry = *ppEntry;
				*ppEntry = pEntry->pNext;
			}
			else
			{
				ppEntry = &(*ppEntry)->pNext;
			}
		}
		Entry** ppInsert = ppEntry;
		if (!pEntry)
		{
			while (*ppEntry && !pEntry)
			{
				if ((*ppEntry)->pLocation == apLocation)
				{
					pEntry = *ppEntry;
					*ppEntry = pEntry->pNext;
				}
				else
				{
					ppEntry = &(*ppEntry)->pNext;
				}
			}
		}
		if (!pEntry)
		{
			pEntry = pFree;
			if (pEntry)
				pFree = pEntry->pNext;
			else
			{
				pEntry = static_cast<Entry*>(MemoryManager::Instance().Allocate(sizeof(Entry), 0, false));
				if (pEntry)
				{
					pEntry->pNext = nullptr;
					pEntry->pLocation = nullptr;
					pEntry->uiPriority = 0xffffffff;
				}
			}
		}
		if (pEntry)
		{
			if (apLocation->Mount() != EC_NONE)
				ppInsert = &pPendingMount;
			pEntry->pLocation = apLocation;
			pEntry->uiPriority = auiPriority;
			pEntry->pNext = *ppInsert;
			*ppInsert = pEntry;
		}
	}

	void GlobalLocations::FlushPendingMounts()
	{
		if (!pPendingMount)
			return;
		BSAutoLock<BSSpinLock> kLock(Lock);
		Entry* pEntry = pPendingMount;
		pPendingMount = nullptr;
		while (pEntry)
		{
			Entry* pNext = pEntry->pNext;
			pEntry->pNext = pFree;
			pFree = pEntry;
			unsigned int uiPriority = pEntry->uiPriority;
			Location* pLocation = pEntry->pLocation;
			Insert(pLocation, uiPriority);
			pEntry = pNext;
		}
	}
	void GlobalLocations::Unregister(Location* apLocation)
	{
		BSAutoLock<BSSpinLock> kLock(Lock);
		Entry** ppEntry = &pHead;
		while (*ppEntry)
		{
			Entry* pEntry = *ppEntry;
			if (pEntry->pLocation == apLocation)
			{
				*ppEntry = pEntry->pNext;
				pEntry->pNext = pFree;
				pFree = pEntry;
				if (pEntry->pLocation->bMounted)
					pEntry->pLocation->DoUnmount();
				break;
			}
			ppEntry = &pEntry->pNext;
		}
	}

	static inline unsigned int FindPath(BSTArray<BSFixedString>& arNames, const BSFixedString& arName, bool& arFound)
	{
		arFound = false;
		int iFirst = 0;
		int iLast = static_cast<int>(arNames.QSize() - 1);
		while (iFirst <= iLast)
		{
			unsigned int uiMiddle = static_cast<unsigned int>(iFirst + ((iLast - iFirst) >> 1));
			uintptr_t uiName = reinterpret_cast<uintptr_t>(arName.pString);
			uintptr_t uiCurrent = reinterpret_cast<uintptr_t>(arNames[uiMiddle].pString);
			if (uiName > uiCurrent)
				iFirst = static_cast<int>(uiMiddle + 1);
			else if (uiName < uiCurrent)
				iLast = static_cast<int>(uiMiddle - 1);
			else
			{
				arFound = true;
				return uiMiddle;
			}
		}
		return static_cast<unsigned int>(iFirst);
	}

	void GlobalPaths::AddPath(const char* apPath)
	{
		BSFixedString kName;
		MakeName(kName, apPath);
		bool bFound;
		unsigned int uiIndex = FindPath(NamesA, kName, bFound);
		if (!bFound)
			NamesA.Insert(uiIndex, kName);
	}

	void GlobalPaths::MakeName(BSFixedString& arName, const char* apPath)
	{
		char cName[260];
		char* pOutput = cName;
		const char* pInput = apPath;
		unsigned int uiRemaining = 258;
		while (*pInput && uiRemaining--)
		{
			char cChar = *pInput++;
			*pOutput++ = cChar == '/' || cChar == '\\' ? '\\' : cChar;
		}
		if (pInput > apPath)
		{
			if (pOutput[-1] == '/' || pOutput[-1] == '\\')
				pOutput[-1] = '\\';
			else
			{
				*pOutput = '\\';
				pOutput[1] = 0;
			}
		}
		// Only a newly appended separator receives an explicit terminator.
		arName << cName;
	}

	void GlobalPaths::RemovePath(const char* apPath)
	{
		BSFixedString kName;
		MakeName(kName, apPath);
		bool bFound;
		unsigned int uiIndex = FindPath(NamesA, kName, bFound);
		if (bFound)
		{
			if (NamesA.QSize() == 1)
			{
				BSFixedString* pName = NamesA.QBuffer();
				if (pName)
				{
					pName->~BSFixedString();
					NamesA.iSize = 0;
				}
			}
			else
			{
				NamesA[uiIndex].~BSFixedString();
				NamesA.BSTArrayBase::MoveItems(NamesA.QBuffer(), uiIndex, uiIndex + 1, NamesA.QSize() - uiIndex - 1, sizeof(BSFixedString));
				--NamesA.iSize;
			}
		}
	}
	void RegisterLocation(Location* apLocation, unsigned int auiPriority)
	{
		GlobalLocations* pLocations = pGlobalLocations;
		pLocations->Register(apLocation, auiPriority);
	}

	void UnregisterLocation(Location* apLocation)
	{
		pGlobalLocations->Unregister(apLocation);
	}

	void RegisterGlobalPath(const char* apPath)
	{
		AutoMemContext kContext(static_cast<MEM_CONTEXT>(8));
		pGlobalPaths->AddPath(apPath);
	}

	void UnregisterGlobalPath(const char* apPath)
	{
		pGlobalPaths->RemovePath(apPath);
	}

	void Update()
	{
		pGlobalLocations->FlushPendingMounts();
	}
}
