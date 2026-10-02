#include "BSResource/BSResourceBTree.h"
#include "BSCore/BSMemoryutility.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/MemoryContextTracker.h"

#include <cstring>

namespace BSResource::BTree
{
	Cursor::PageEntry::PageEntry() : pNode(nullptr), pPage(nullptr)
	{
	}

	Cursor::Cursor() : IndexA{}, uiDepth(0), pEntry(nullptr)
	{
	}

	bool Tree::CursorSearch(Cursor& arCursor, const ID& arKey) const
	{
		if (arCursor.PageA[0].pNode != pRoot)
		{
			arCursor.PageA[0].pNode = pRoot;
			arCursor.PageA[0].pPage = pRootPage;
		}
		unsigned int uiDepth = 0;
		bool bFound = false;
		do
		{
			Node* pNode = arCursor.PageA[uiDepth].pNode;
			const unsigned int uiFlags = pNode->uiFlags;
			unsigned int uiLow = 0;
			unsigned int uiHigh = uiFlags & 0x7fffffff;
			if (!(uiFlags & 0x80000000))
			{
				while (uiLow < uiHigh)
				{
					const unsigned int uiMid = (uiLow + uiHigh) >> 1;
					const ID& rMid = pNode->Interior.KeyA[uiMid];
					if (arKey == rMid)
					{
						uiHigh = uiMid;
						break;
					}
					if (arKey < rMid)
						uiHigh = uiMid;
					else
						uiLow = uiMid + 1;
				}
				arCursor.IndexA[uiDepth++] = uiHigh;
				Node* pChild = pNode->Interior.ChildA[uiHigh];
				if (arCursor.PageA[uiDepth].pPage != pChild)
				{
					arCursor.PageA[uiDepth].pNode = pChild;
					arCursor.PageA[uiDepth].pPage = pNode->Interior.ChildA[uiHigh];
				}
			}
			else
			{
				const unsigned int uiCount = uiHigh;
				while (uiLow < uiHigh)
				{
					const unsigned int uiMid = (uiLow + uiHigh) >> 1;
					if (arKey <= *pNode->EntryA[uiMid])
						uiHigh = uiMid;
					else
						uiLow = uiMid + 1;
				}
				arCursor.IndexA[uiDepth++] = uiHigh;
				bFound = uiHigh < uiCount && *pNode->EntryA[uiHigh] == arKey;
			}
		} while (uiDepth < uiHeight);
		arCursor.uiDepth = uiDepth;
		return bFound;
	}

	void Node::MoveToNode(Node* apDest, unsigned int auiDest, const Node* apSource, unsigned int auiSource, unsigned int auiCount)
	{
		if (!auiCount)
			return;
		const unsigned int uiKeys = 12 * auiCount;
		const unsigned int uiChildren = 8 * auiCount;
		BSmemcpy(apDest->Interior.KeyA + auiDest, uiKeys, apSource->Interior.KeyA + auiSource, uiKeys);
		BSmemcpy(apDest->Interior.ChildA + auiDest + 1, uiChildren, apSource->Interior.ChildA + auiSource + 1, uiChildren);
	}

	void Node::ShiftRight(Node* apNode, unsigned int auiStart, unsigned int auiEnd)
	{
		const unsigned int uiCount = auiEnd - auiStart;
		if (!uiCount)
			return;
		memmove(apNode->Interior.KeyA + auiStart + 1, apNode->Interior.KeyA + auiStart, static_cast<unsigned int>(12 * uiCount));
		memmove(apNode->Interior.ChildA + auiStart + 2, apNode->Interior.ChildA + auiStart + 1, static_cast<unsigned int>(8 * uiCount));
	}

	void Node::InsertSplit(Node* apRight, Node* apLeft, const ID* const& arEntry, unsigned int auiPos, ID& arKey)
	{
		const ID* pBoundary;
		if (auiPos >= 63)
		{
			const unsigned int uiPos = auiPos - 63;
			if (uiPos)
				BSmemcpy(apRight->EntryA, 8 * uiPos, apLeft->EntryA + 63, 8 * uiPos);
			apRight->EntryA[uiPos] = arEntry;
			if (uiPos != 63)
				BSmemcpy(apRight->EntryA + uiPos + 1, 8 * (63 - uiPos), apLeft->EntryA + auiPos, 8 * (63 - uiPos));
			apLeft->uiFlags = 0x8000003f;
			apRight->uiFlags = 0x80000040;
			pBoundary = apLeft->EntryA[62];
		}
		else
		{
			BSmemcpy(apRight->EntryA, 504, apLeft->EntryA + 63, 504);
			memmove(apLeft->EntryA + auiPos + 1, apLeft->EntryA + auiPos, 8 * (63 - auiPos));
			apLeft->EntryA[auiPos] = arEntry;
			apLeft->uiFlags = 0x80000040;
			apRight->uiFlags = 0x8000003f;
			pBoundary = apLeft->EntryA[63];
		}
		arKey.uiDir = pBoundary->uiDir;
		arKey.uiFile = pBoundary->uiFile;
		arKey.uiExt = pBoundary->uiExt;
	}

	void Node::InsertSplit(Node* apRight, Node* apLeft, ID& arKey, Node* apChild, unsigned int auiPos)
	{
		Node* pFirst = apChild;
		if (auiPos >= 25)
		{
			unsigned int uiDest = 0;
			const unsigned int uiPos = auiPos - 25;
			if (uiPos)
			{
				ID OldKey = arKey;
				arKey = apLeft->Interior.KeyA[25];
				pFirst = apLeft->Interior.ChildA[26];
				MoveToNode(apRight, 0, apLeft, 26, auiPos - 26);
				uiDest = uiPos;
				apRight->Interior.KeyA[uiPos - 1] = OldKey;
				apRight->Interior.ChildA[uiPos] = apChild;
			}
			if (uiPos < 25)
				MoveToNode(apRight, uiDest, apLeft, uiDest + 25, 25 - uiDest);
		}
		else
		{
			ID OldKey = arKey;
			arKey = apLeft->Interior.KeyA[24];
			pFirst = apLeft->Interior.ChildA[25];
			BSmemcpy(apRight->Interior.KeyA, 300, apLeft->Interior.KeyA + 25, 300);
			BSmemcpy(apRight->Interior.ChildA + 1, 200, apLeft->Interior.ChildA + 26, 200);
			ShiftRight(apLeft, auiPos, 24);
			apLeft->Interior.KeyA[auiPos] = OldKey;
			apLeft->Interior.ChildA[auiPos + 1] = apChild;
		}
		apRight->uiFlags = 25;
		apLeft->uiFlags = 25;
		apRight->Interior.ChildA[0] = pFirst;
	}

	Pager::Pager(unsigned int)
	{
	}

	bool Pager::GetPage(Node*& arpNode, Node*& arpPage, unsigned int& aruiLeaf, std::int64_t)
	{
		aruiLeaf = 1;
		AutoMemContext Context(static_cast<MEM_CONTEXT>(8));
		Node* pNode = static_cast<Node*>(MemoryManager::Instance().Allocate(1024, 128, true));
		arpNode = pNode;
		arpPage = pNode;
		return pNode != nullptr;
	}

	bool Pager::NewPage(Node*& arpNode, Node*& arpPage)
	{
		AutoMemContext Context(static_cast<MEM_CONTEXT>(8));
		Node* pNode = static_cast<Node*>(MemoryManager::Instance().Allocate(1024, 128, true));
		arpNode = pNode;
		arpPage = pNode;
		return pNode != nullptr;
	}

	void Pager::ReleasePage(Node* apPage)
	{
		AutoMemContext Context(static_cast<MEM_CONTEXT>(8));
		MemoryManager::Instance().Deallocate(apPage, false);
	}

	void Node::MoveWithinNode(Node* apNode, unsigned int auiDest, unsigned int auiSource, unsigned int auiCount)
	{
		if (auiDest == auiSource || !auiCount)
			return;
		const unsigned int uiKeys = 12 * auiCount;
		const unsigned int uiChildren = 8 * auiCount;
		BSmemmove(apNode->Interior.KeyA + auiDest, uiKeys, apNode->Interior.KeyA + auiSource, uiKeys);
		BSmemmove(apNode->Interior.ChildA + auiDest + 1, uiChildren, apNode->Interior.ChildA + auiSource + 1, uiChildren);
	}

	bool Node::TrySiblingOverflow(Node* apLeft, Node* apRight, Node* apNode, const ID* const& arEntry, unsigned int auiPos, Node* apParent, unsigned int auiParentPos, unsigned int& aruiSide)
	{
		const unsigned int uiLeft = apLeft ? apLeft->uiFlags & 0x7fffffff : 126;
		const unsigned int uiRight = apRight ? apRight->uiFlags & 0x7fffffff : 126;
		if (uiRight < 126)
		{
			const unsigned int uiKeep = (uiRight + 126) >> 1;
			const unsigned int uiMove = 126 - uiKeep;
			if (uiMove && uiRight)
				BSmemmove(apRight->EntryA + uiMove, 8 * uiRight, apRight->EntryA, 8 * uiRight);
			if (auiPos > uiKeep)
			{
				const unsigned int uiPos = auiPos - uiKeep;
				if (uiPos != 1)
					BSmemcpy(apRight->EntryA, 8 * (uiPos - 1), apNode->EntryA + uiKeep + 1, 8 * (uiPos - 1));
				apRight->EntryA[uiPos - 1] = arEntry;
				if (auiPos != 126)
					BSmemcpy(apRight->EntryA + uiPos, 8 * (126 - auiPos), apNode->EntryA + auiPos, 8 * (126 - auiPos));
			}
			else
			{
				if (uiMove)
					BSmemcpy(apRight->EntryA, 8 * uiMove, apNode->EntryA + uiKeep, 8 * uiMove);
				if (uiKeep != auiPos)
					memmove(apNode->EntryA + auiPos + 1, apNode->EntryA + auiPos, 8 * (uiKeep - auiPos));
				apNode->EntryA[auiPos] = arEntry;
			}
			apNode->uiFlags = (uiKeep + 1) | 0x80000000;
			apRight->uiFlags = (uiMove + uiRight) | 0x80000000;
			apParent->Interior.KeyA[auiParentPos] = *apNode->EntryA[uiKeep];
			aruiSide = 2;
			return true;
		}
		if (uiLeft >= uiRight)
		{
			aruiSide = 0;
			return false;
		}
		const unsigned int uiKeep = (uiLeft + 126) >> 1;
		const unsigned int uiMove = 126 - uiKeep;
		if (uiMove > auiPos)
		{
			if (auiPos)
				BSmemcpy(apLeft->EntryA + uiLeft, 8 * auiPos, apNode->EntryA, 8 * auiPos);
			apLeft->EntryA[uiLeft + auiPos] = arEntry;
			if (uiMove - auiPos != 1)
				BSmemcpy(apLeft->EntryA + uiLeft + auiPos + 1, 8 * (uiMove - auiPos - 1), apNode->EntryA + auiPos, 8 * (uiMove - auiPos - 1));
			if (uiMove != 1)
				BSmemmove(apNode->EntryA, 8 * uiKeep + 8, apNode->EntryA + uiMove - 1, 8 * uiKeep + 8);
		}
		else
		{
			if (uiMove)
				BSmemcpy(apLeft->EntryA + uiLeft, 8 * uiMove, apNode->EntryA, 8 * uiMove);
			const unsigned int uiPos = auiPos - uiMove;
			if (uiMove && uiPos)
				BSmemmove(apNode->EntryA, 8 * uiPos, apNode->EntryA + uiMove, 8 * uiPos);
			apNode->EntryA[uiPos] = arEntry;
			if (auiPos != uiPos + 1 && 126 != auiPos)
				BSmemmove(apNode->EntryA + uiPos + 1, 8 * (126 - auiPos), apNode->EntryA + auiPos, 8 * (126 - auiPos));
		}
		apNode->uiFlags = (uiKeep + 1) | 0x80000000;
		apLeft->uiFlags = (uiMove + uiLeft) | 0x80000000;
		apParent->Interior.KeyA[auiParentPos - 1] = *apLeft->EntryA[uiMove + uiLeft - 1];
		aruiSide = 1;
		return true;
	}

	bool Node::TryOverflowRight(Node* apRight, Node* apNode, const ID& arKey, Node* apChild, unsigned int auiPos, Node* apParent, unsigned int auiParentPos)
	{
		const unsigned int uiCount = apRight->uiFlags;
		if (uiCount >= 50)
			return false;
		const unsigned int uiKeep = (uiCount + 50) >> 1;
		const unsigned int uiMove = 50 - uiKeep;
		const unsigned int uiFirst = uiKeep + 1;
		if (uiMove && uiCount)
		{
			BSmemmove(apRight->Interior.KeyA + uiMove, 12 * uiCount, apRight->Interior.KeyA, 12 * uiCount);
			BSmemmove(apRight->Interior.ChildA + uiMove + 1, 8 * uiCount, apRight->Interior.ChildA + 1, 8 * uiCount);
		}
		apRight->Interior.ChildA[uiMove] = apRight->Interior.ChildA[0];
		ID& rSeparator = apParent->Interior.KeyA[auiParentPos];
		apRight->Interior.KeyA[uiMove - 1] = rSeparator;
		if (auiPos >= uiFirst)
		{
			if (auiPos == uiFirst)
			{
				rSeparator = arKey;
				MoveToNode(apRight, 0, apNode, uiFirst, uiMove - 1);
				apRight->Interior.ChildA[0] = apChild;
			}
			else
			{
				const unsigned int uiPos = auiPos - uiFirst;
				BSmemcpy(&rSeparator, 12, apNode->Interior.KeyA + uiFirst, 12);
				apRight->Interior.ChildA[0] = apNode->Interior.ChildA[uiFirst + 1];
				MoveToNode(apRight, 0, apNode, uiFirst + 1, uiPos - 1);
				apRight->Interior.KeyA[uiPos - 1] = arKey;
				apRight->Interior.ChildA[uiPos] = apChild;
				MoveToNode(apRight, uiPos, apNode, auiPos, 50 - auiPos);
			}
		}
		else
		{
			MoveToNode(apRight, 0, apNode, uiFirst, uiMove - 1);
			apRight->Interior.ChildA[0] = apNode->Interior.ChildA[uiFirst];
			rSeparator = apNode->Interior.KeyA[uiKeep];
			ShiftRight(apNode, auiPos, uiKeep);
			apNode->Interior.KeyA[auiPos] = arKey;
			apNode->Interior.ChildA[auiPos + 1] = apChild;
		}
		apNode->uiFlags = uiFirst;
		apRight->uiFlags = uiCount + uiMove;
		return true;
	}

	bool Node::TryOverflowLeft(Node* apLeft, Node* apNode, const ID& arKey, Node* apChild, unsigned int auiPos, Node* apParent, unsigned int auiParentPos)
	{
		const unsigned int uiCount = apLeft->uiFlags;
		if (uiCount >= 50)
			return false;
		const unsigned int uiKeep = (uiCount + 50) >> 1;
		const unsigned int uiMove = 50 - uiKeep;
		const unsigned int uiLast = uiMove - 1;
		ID& rSeparator = apParent->Interior.KeyA[auiParentPos - 1];
		apLeft->Interior.KeyA[uiCount] = rSeparator;
		apLeft->Interior.ChildA[uiCount + 1] = apNode->Interior.ChildA[0];
		unsigned int uiNodeCount = uiKeep + 1;
		unsigned int uiLeftCount = uiMove + uiCount;
		if (auiPos < uiLast)
		{
			BSmemcpy(&rSeparator, 12, apNode->Interior.KeyA + uiLast, 12);
			apNode->Interior.ChildA[0] = apNode->Interior.ChildA[uiMove];
			MoveToNode(apLeft, uiCount + 1, apNode, 0, auiPos);
			const unsigned int uiDest = uiCount + 1 + auiPos;
			apLeft->Interior.KeyA[uiDest] = arKey;
			apLeft->Interior.ChildA[uiDest + 1] = apChild;
			MoveToNode(apLeft, uiDest + 1, apNode, auiPos, uiLast - auiPos);
			++uiLeftCount;
			MoveWithinNode(apNode, 0, uiMove, uiKeep);
			uiNodeCount = uiKeep;
		}
		else if (auiPos == uiLast)
		{
			rSeparator = arKey;
			apNode->Interior.ChildA[0] = apChild;
			MoveToNode(apLeft, uiCount + 1, apNode, 0, uiLast);
			MoveWithinNode(apNode, 0, uiLast, 50 - uiLast);
		}
		else
		{
			MoveToNode(apLeft, uiCount + 1, apNode, 0, uiLast);
			rSeparator = apNode->Interior.KeyA[uiLast];
			apNode->Interior.ChildA[0] = apNode->Interior.ChildA[uiMove];
			MoveWithinNode(apNode, 0, uiMove, auiPos - uiMove);
			const unsigned int uiPos = auiPos - uiMove;
			apNode->Interior.KeyA[uiPos] = arKey;
			apNode->Interior.ChildA[uiPos + 1] = apChild;
			MoveWithinNode(apNode, uiPos + 1, auiPos, 50 - auiPos);
		}
		apNode->uiFlags = uiNodeCount;
		apLeft->uiFlags = uiLeftCount;
		return true;
	}

	void Tree::NewRoot(const ID& arKey, Node* apChild)
	{
		Node* pNode = nullptr;
		Node* pPage = nullptr;
		if (!pPager->NewPage(pNode, pPage))
			return;
		memcpy(pNode, pRoot, sizeof(Node));
		Node* pRootNode = pRoot;
		pRootNode->Interior.KeyA[0] = arKey;
		pRootNode->Interior.ChildA[0] = pPage;
		pRootNode->Interior.ChildA[1] = apChild;
		pRootNode->uiFlags = 1;
		++uiHeight;
	}

	bool Tree::CursorInsertAt(Cursor& arCursor, const ID* const& arEntry, const ID*& arpPrevious, bool abReplace)
	{
		if (arCursor.uiDepth != uiHeight)
			return false;
		const unsigned int uiDepth = arCursor.uiDepth - 1;
		const unsigned int uiPos = arCursor.IndexA[uiDepth];
		Node* pNode = arCursor.PageA[uiDepth].pNode;
		if (abReplace)
		{
			arpPrevious = pNode->EntryA[uiPos];
			pNode->EntryA[uiPos] = arEntry;
			return true;
		}
		const unsigned int uiActive = pNode->uiFlags & 0x7fffffff;
		bool bInserted = false;
		if (uiActive < 126)
		{
			if (uiActive != uiPos)
				memmove(pNode->EntryA + uiPos + 1, pNode->EntryA + uiPos, static_cast<unsigned int>(8 * (uiActive - uiPos)));
			pNode->EntryA[uiPos] = arEntry;
			pNode->uiFlags = (uiActive + 1) | 0x80000000;
			bInserted = true;
		}
		if (!bInserted && uiDepth)
		{
			const unsigned int uiParentDepth = uiDepth - 1;
			const unsigned int uiParentPos = arCursor.IndexA[uiParentDepth];
			Node* pParent = arCursor.PageA[uiParentDepth].pNode;
			Node* pLeft = uiParentPos ? pParent->Interior.ChildA[uiParentPos - 1] : nullptr;
			Node* pRight = uiParentPos + 1 <= pParent->uiFlags ? pParent->Interior.ChildA[uiParentPos + 1] : nullptr;
			unsigned int uiSide;
			if (pLeft || pRight)
				bInserted = Node::TrySiblingOverflow(pLeft, pRight, pNode, arEntry, uiPos, pParent, uiParentPos, uiSide);
			if (!bInserted)
				bInserted = CursorInsertSplitNonRoot(arCursor, pNode, arEntry, uiPos, uiParentDepth);
		}
		else if (!bInserted)
		{
			Node* pRight = nullptr;
			Node* pPage = nullptr;
			ID Key;
			if (pPager->NewPage(pRight, pPage))
			{
				Node::InsertSplit(pRight, pNode, arEntry, uiPos, Key);
				NewRoot(Key, pPage);
				bInserted = true;
			}
		}
		if (bInserted)
			++uiCount;
		return bInserted;
	}

	bool Tree::CursorInsertSplitNonRoot(Cursor& arCursor, Node* apNode, const ID* const& arEntry, unsigned int auiPos, unsigned int auiDepth)
	{
		Node* pRight = nullptr;
		Node* pPage = nullptr;
		if (!pPager->NewPage(pRight, pPage))
			return false;
		Node* pChild = pPage;
		ID Key;
		Node::InsertSplit(pRight, apNode, arEntry, auiPos, Key);
		unsigned int uiDepth = auiDepth;
		unsigned int uiNextDepth = auiDepth + 1;
		bool bPropagate = true;
		for (;;)
		{
			Node* pNode = arCursor.PageA[uiDepth].pNode;
			const unsigned int uiPos = arCursor.IndexA[uiDepth];
			const unsigned int uiActive = pNode->uiFlags;
			if (uiActive < 50)
			{
				Node::ShiftRight(pNode, uiPos, uiActive);
				pNode->Interior.KeyA[uiPos] = Key;
				pNode->Interior.ChildA[uiPos + 1] = pChild;
				pNode->uiFlags = uiActive + 1;
				bPropagate = false;
			}
			else
			{
				if (uiDepth)
				{
					Node* pParent = arCursor.PageA[uiDepth - 1].pNode;
					const unsigned int uiParentPos = arCursor.IndexA[uiDepth - 1];
					if ((uiParentPos + 1 <= pParent->uiFlags && Node::TryOverflowRight(pParent->Interior.ChildA[uiParentPos + 1], pNode, Key, pChild, uiPos, pParent, uiParentPos)) ||
						(uiParentPos && Node::TryOverflowLeft(pParent->Interior.ChildA[uiParentPos - 1], pNode, Key, pChild, uiPos, pParent, uiParentPos)))
						bPropagate = false;
				}
				if (bPropagate)
				{
					Node* pNew = nullptr;
					if (pPager->NewPage(pNew, pPage))
					{
						Node::InsertSplit(pNew, pNode, Key, pChild, uiPos);
						pChild = pPage;
						uiNextDepth = uiDepth--;
					}
				}
			}
			if (!uiNextDepth)
				break;
			if (!bPropagate)
				return true;
		}
		if (bPropagate)
			NewRoot(Key, pChild);
		return true;
	}

	bool Tree::CursorNext(Cursor& arCursor, const ID*& arpEntry) const
	{
		Node* pNode = arCursor.PageA[uiHeight - 1].pNode;
		unsigned int uiPos = arCursor.IndexA[uiHeight - 1];
		if (uiPos < (pNode->uiFlags & 0x7fffffff))
		{
			arpEntry = pNode->EntryA[uiPos];
			++arCursor.IndexA[arCursor.uiDepth - 1];
			return true;
		}
		if (uiHeight <= 1)
			return false;
		unsigned int uiDepth = arCursor.uiDepth - 2;
		for (;;)
		{
			for (;;)
			{
				pNode = arCursor.PageA[uiDepth].pNode;
				uiPos = arCursor.IndexA[uiDepth] + 1;
				if (uiPos <= pNode->uiFlags)
					break;
				if (!uiDepth)
					return false;
				--uiDepth;
			}
			do
			{
				arCursor.IndexA[uiDepth] = uiPos;
				Node* pChild = pNode->Interior.ChildA[uiPos];
				arCursor.PageA[++uiDepth].pNode = pChild;
				arCursor.PageA[uiDepth].pPage = pNode->Interior.ChildA[uiPos];
				pNode = pChild;
				uiPos = 0;
			} while (!(pNode->uiFlags & 0x80000000));
			arCursor.IndexA[uiDepth] = 0;
			bool bFound = false;
			if (arCursor.uiDepth == uiHeight)
			{
				pNode = arCursor.PageA[uiHeight - 1].pNode;
				uiPos = arCursor.IndexA[uiHeight - 1];
				if (uiPos < (pNode->uiFlags & 0x7fffffff))
				{
					arpEntry = pNode->EntryA[uiPos];
					bFound = true;
				}
			}
			arCursor.IndexA[uiDepth] = (bFound ? arCursor.IndexA[uiDepth] : 0) + 1;
			if (bFound)
				return true;
		}
	}

	void Tree::CursorClear(Cursor& arCursor)
	{
		if (uiHeight <= 1)
		{
			const unsigned int uiActive = pRoot->uiFlags & 0x7fffffff;
			if (uiActive)
				memset(pRoot->EntryA, 0, 8ULL * uiActive);
			pRoot->uiFlags = 0x80000000;
		}
		else
		{
			if (arCursor.PageA[0].pNode != pRoot)
			{
				arCursor.PageA[0].pNode = pRoot;
				arCursor.PageA[0].pPage = pRootPage;
			}
			unsigned int uiDepth = 0;
			do
			{
				Node* pNode = arCursor.PageA[uiDepth].pNode;
				arCursor.IndexA[uiDepth++] = 0;
				if (!(pNode->uiFlags & 0x80000000))
				{
					Node* pChild = pNode->Interior.ChildA[0];
					if (arCursor.PageA[uiDepth].pPage != pChild)
					{
						arCursor.PageA[uiDepth].pNode = pChild;
						arCursor.PageA[uiDepth].pPage = pNode->Interior.ChildA[0];
					}
				}
			} while (uiDepth < uiHeight);
			--uiDepth;
			for (;;)
			{
				Node* pLeaf = arCursor.PageA[uiDepth].pNode;
				const unsigned int uiActive = pLeaf->uiFlags & 0x7fffffff;
				if (uiActive)
					memset(pLeaf->EntryA, 0, 8ULL * uiActive);
				pLeaf->uiFlags = 0x80000000;
				bool bNext = false;
				for (;;)
				{
					pPager->ReleasePage(arCursor.PageA[uiDepth].pNode);
					arCursor.PageA[uiDepth].pPage = nullptr;
					arCursor.PageA[uiDepth--].pNode = nullptr;
					Node* pParent = arCursor.PageA[uiDepth].pNode;
					unsigned int uiPos = arCursor.IndexA[uiDepth] + 1;
					if (uiPos <= pParent->uiFlags)
					{
						do
						{
							arCursor.IndexA[uiDepth] = uiPos;
							Node* pChild = pParent->Interior.ChildA[uiPos];
							arCursor.PageA[++uiDepth].pNode = pChild;
							arCursor.PageA[uiDepth].pPage = pParent->Interior.ChildA[uiPos];
							pParent = pChild;
							uiPos = 0;
						} while (!(pParent->uiFlags & 0x80000000));
						bNext = true;
						break;
					}
					if (!uiDepth)
						break;
					pParent->uiFlags = 0;
				}
				if (!bNext)
					break;
			}
			for (unsigned int ui = 1; ui < uiHeight; ++ui)
			{
				arCursor.PageA[ui].pPage = nullptr;
				arCursor.IndexA[ui] = 0;
			}
		}
		pRoot->uiFlags = 0x80000000;
		arCursor.uiDepth = 1;
		uiCount = 0;
		uiHeight = 1;
	}
}
