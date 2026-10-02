#pragma once

#include "BSResource/BSResourceID.h"

namespace BSResource::BTree
{
	struct Node
	{
		union
		{
			const ID* EntryA[127];
			struct
			{
				ID KeyA[50];
				Node* ChildA[51];
			} Interior;
		};
		unsigned int uiPadding;
		unsigned int uiFlags;

		static void InsertSplit(Node* apRight, Node* apLeft, const ID* const& arEntry, unsigned int auiPos, ID& arKey);
		static void InsertSplit(Node* apRight, Node* apLeft, ID& arKey, Node* apChild, unsigned int auiPos);
		static void MoveToNode(Node* apDest, unsigned int auiDest, const Node* apSource, unsigned int auiSource, unsigned int auiCount);
		static void MoveWithinNode(Node* apNode, unsigned int auiDest, unsigned int auiSource, unsigned int auiCount);
		static void ShiftRight(Node* apNode, unsigned int auiStart, unsigned int auiEnd);
		static bool TrySiblingOverflow(Node* apLeft, Node* apRight, Node* apNode, const ID* const& arEntry, unsigned int auiPos, Node* apParent, unsigned int auiParentPos, unsigned int& aruiSide);
		static bool TryOverflowLeft(Node* apLeft, Node* apNode, const ID& arKey, Node* apChild, unsigned int auiPos, Node* apParent, unsigned int auiParentPos);
		static bool TryOverflowRight(Node* apRight, Node* apNode, const ID& arKey, Node* apChild, unsigned int auiPos, Node* apParent, unsigned int auiParentPos);
	};

	struct Cursor
	{
		struct PageEntry
		{
			PageEntry();
			Node* pNode;
			Node* pPage;
		};
		Cursor();
		PageEntry PageA[8];
		unsigned int IndexA[8];
		unsigned int uiDepth;
		const ID* pEntry;
	};

	class Pager
	{
	public:
		explicit Pager(unsigned int auiSize);
		bool GetPage(Node*& arpNode, Node*& arpPage, unsigned int& aruiLeaf, std::int64_t aiPage);
		bool NewPage(Node*& arpNode, Node*& arpPage);
		void ReleasePage(Node* apPage);
		std::uint64_t uiStorage;
	};
	struct Tree
	{
		bool CursorSearch(Cursor& arCursor, const ID& arKey) const;
		bool CursorInsertAt(Cursor& arCursor, const ID* const& arEntry, const ID*& arpPrevious, bool abReplace);
		bool CursorInsertSplitNonRoot(Cursor& arCursor, Node* apNode, const ID* const& arEntry, unsigned int auiPos, unsigned int auiDepth);
		bool CursorNext(Cursor& arCursor, const ID*& arpEntry) const;
		void CursorClear(Cursor& arCursor);
		void NewRoot(const ID& arKey, Node* apChild);

		Pager* pPager;
		Node* pRoot;
		Node* pRootPage;
		unsigned int uiCount;
		unsigned int uiHeight;
	};

	static_assert(sizeof(Node) == 1024);
	static_assert(offsetof(Node, Interior.ChildA) == 600);
	static_assert(offsetof(Node, uiFlags) == 1020);
	static_assert(sizeof(Cursor) == 176);
	static_assert(offsetof(Cursor, IndexA) == 128);
	static_assert(offsetof(Cursor, uiDepth) == 160);
	static_assert(offsetof(Cursor, pEntry) == 168);
	static_assert(sizeof(Tree) == 32);
}
