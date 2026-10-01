#include "BSCore/HeapBlocks.h"
#include "BSCore/BSTIntrusiveRBTree.h"

void HeapBlock::Init(size_t aiSize)
{
	uiMemSize = aiSize;
	FreeOrUsed.pPrevFree = nullptr;
	pNextFree = nullptr;
	pPrevious = nullptr;
}

void HeapBlock::InitAsFree(size_t aiSize)
{
	Init(aiSize | FREE);
}

unsigned int HeapBlock::VerifyMemoryCommitted(const HeapBlock*, size_t, unsigned int)
{
	return 0;
}

HeapBlockFreeHead* HeapBlockFreeHead::TreeSearch(HeapBlockFreeHead* apTreeRoot, size_t auiMinimumSize)
{
	HeapBlockFreeHead* pBest = nullptr;
	size_t uiBestSize = SIZE_MAX;
	while (apTreeRoot && uiBestSize != auiMinimumSize)
	{
		size_t uiSize = apTreeRoot->uiMemSize & SIZE_MASK;
		if (auiMinimumSize > uiSize)
			apTreeRoot = apTreeRoot->pRightChild;
		else
		{
			if (uiSize < uiBestSize)
			{
				pBest = apTreeRoot;
				uiBestSize = uiSize;
			}
			if (auiMinimumSize < uiSize)
				apTreeRoot = apTreeRoot->pLeftChild;
		}
	}
	return pBest;
}

HeapBlockFreeHead* HeapBlockFreeHead::GetPredecessor(HeapBlockFreeHead* apBlock)
{
	if (!apBlock)
		return nullptr;
	if (apBlock->pLeftChild)
	{
		apBlock = apBlock->pLeftChild;
		while (apBlock->pRightChild)
			apBlock = apBlock->pRightChild;
		return apBlock;
	}
	auto* pParent = reinterpret_cast<HeapBlockFreeHead*>(apBlock->uiParentPtrAndBlackBit & ~uintptr_t(1));
	while (pParent && pParent->pRightChild != apBlock)
	{
		apBlock = pParent;
		pParent = reinterpret_cast<HeapBlockFreeHead*>(pParent->uiParentPtrAndBlackBit & ~uintptr_t(1));
	}
	return pParent;
}

HeapBlockFreeHead* HeapBlockFreeHead::GetSuccessor(HeapBlockFreeHead* apBlock)
{
	if (!apBlock)
		return nullptr;
	if (apBlock->pRightChild)
	{
		apBlock = apBlock->pRightChild;
		while (apBlock->pLeftChild)
			apBlock = apBlock->pLeftChild;
		return apBlock;
	}
	auto* pParent = reinterpret_cast<HeapBlockFreeHead*>(apBlock->uiParentPtrAndBlackBit & ~uintptr_t(1));
	while (pParent && pParent->pLeftChild != apBlock)
	{
		apBlock = pParent;
		pParent = reinterpret_cast<HeapBlockFreeHead*>(pParent->uiParentPtrAndBlackBit & ~uintptr_t(1));
	}
	return pParent;
}

namespace
{
	struct HeapBlockTreeAccess
	{
		static size_t GetKey(HeapBlockFreeHead* apNode) { return apNode->GetSize(); }
		static HeapBlockFreeHead*& Left(HeapBlockFreeHead* apNode) { return apNode->pLeftChild; }
		static HeapBlockFreeHead*& Right(HeapBlockFreeHead* apNode) { return apNode->pRightChild; }
		static HeapBlockFreeHead**& Root(HeapBlockFreeHead* apNode) { return apNode->ppRoot; }
		static HeapBlockFreeHead* Parent(HeapBlockFreeHead* apNode) { return apNode->QParent(); }
		static bool Black(HeapBlockFreeHead* apNode) { return apNode->IsBlack(); }
		static void SetBlack(HeapBlockFreeHead* apNode, bool abBlack) { apNode->SetBlack(abBlack); }
		static void SetParent(HeapBlockFreeHead* apNode, HeapBlockFreeHead* apParent) { apNode->SetParent(apParent); }
	};
	using HeapBlockTree = BSTIntrusiveRBTree<HeapBlockFreeHead, size_t, HeapBlockTreeAccess>;
}

void ReplaceTreeNode(HeapBlockFreeHead*& arpRoot, HeapBlockFreeHead* apOld, HeapBlock* apNew)
{
	auto* pReplacement = static_cast<HeapBlockFreeHead*>(apNew);
	HeapBlockFreeHead* pParent = HeapBlockTreeAccess::Parent(apOld);
	pReplacement->uiMemSize |= HeapBlock::FREE_HEAD;
	pReplacement->uiParentPtrAndBlackBit = apOld->uiParentPtrAndBlackBit;
	if (!pParent)
		arpRoot = pReplacement;
	else if (pParent->pLeftChild == apOld)
		pParent->pLeftChild = pReplacement;
	else
		pParent->pRightChild = pReplacement;
	pReplacement->pLeftChild = apOld->pLeftChild;
	pReplacement->pRightChild = apOld->pRightChild;
	pReplacement->ppRoot = &arpRoot;
	if (pReplacement->pLeftChild)
		HeapBlockTreeAccess::SetParent(pReplacement->pLeftChild, pReplacement);
	if (pReplacement->pRightChild)
		HeapBlockTreeAccess::SetParent(pReplacement->pRightChild, pReplacement);
	apOld->uiMemSize &= ~HeapBlock::FREE_HEAD;
}

void HeapBlockFreeHead::TreeInsert(HeapBlockFreeHead*& arpTreeRoot, HeapBlockFreeHead* apInsertBlock)
{
	HeapBlockFreeHead* pCurrent;
	HeapBlockTree::Insert(arpTreeRoot, apInsertBlock, pCurrent);
	if (pCurrent == apInsertBlock)
		apInsertBlock->uiMemSize |= FREE_HEAD;
	else
	{
		apInsertBlock->ListInsertAfter(pCurrent);
	}
}

void HeapBlockFreeHead::TreeRemove(HeapBlockFreeHead* apRemoveBlock)
{
	if ((apRemoveBlock->uiMemSize & (FREE | FREE_HEAD)) == (FREE | FREE_HEAD))
	{
		if (apRemoveBlock->pNextFree || apRemoveBlock->FreeOrUsed.pPrevFree)
		{
			ReplaceTreeNode(*apRemoveBlock->ppRoot, apRemoveBlock, apRemoveBlock->pNextFree);
		}
		else
			HeapBlockTree::Remove(apRemoveBlock);
		apRemoveBlock->uiMemSize &= ~FREE_HEAD;
	}
	apRemoveBlock->ListRemove();
}

struct RightLeft;
struct LeftRight
{
	using Other = RightLeft;
	static HeapBlockFreeHead*& ThisSide(HeapBlockFreeHead* apNode) { return apNode->pLeftChild; }
	static HeapBlockFreeHead*& OtherSide(HeapBlockFreeHead* apNode) { return apNode->pRightChild; }
};
struct RightLeft
{
	using Other = LeftRight;
	static HeapBlockFreeHead*& ThisSide(HeapBlockFreeHead* apNode) { return apNode->pRightChild; }
	static HeapBlockFreeHead*& OtherSide(HeapBlockFreeHead* apNode) { return apNode->pLeftChild; }
};

template <class Side>
void Rotate(HeapBlockFreeHead*& arpRoot, HeapBlockFreeHead* apNode)
{
	HeapBlockFreeHead* pParent = HeapBlockTreeAccess::Parent(apNode);
	HeapBlockFreeHead* pPivot = Side::OtherSide(apNode);
	HeapBlockFreeHead* pMiddle = Side::ThisSide(pPivot);
	Side::ThisSide(pPivot) = apNode;
	HeapBlockTreeAccess::SetParent(apNode, pPivot);
	Side::OtherSide(apNode) = pMiddle;
	if (pMiddle)
		HeapBlockTreeAccess::SetParent(pMiddle, apNode);
	if (!pParent)
		arpRoot = pPivot;
	else if (Side::ThisSide(pParent) == apNode)
		Side::ThisSide(pParent) = pPivot;
	else
		Side::OtherSide(pParent) = pPivot;
	HeapBlockTreeAccess::SetParent(pPivot, pParent);
}

template <class Side>
bool RebalanceAfterRemove(HeapBlockFreeHead*& arpRoot, HeapBlockFreeHead*& arpNode, HeapBlockFreeHead*& arpParent)
{
	HeapBlockFreeHead* pSibling = Side::OtherSide(arpParent);
	if (!HeapBlockTreeAccess::Black(pSibling))
	{
		HeapBlockTreeAccess::SetBlack(pSibling, true);
		HeapBlockTreeAccess::SetBlack(arpParent, false);
		Rotate<Side>(arpRoot, arpParent);
		pSibling = Side::OtherSide(arpParent);
	}
	HeapBlockFreeHead* pNear = Side::ThisSide(pSibling);
	HeapBlockFreeHead* pFar = Side::OtherSide(pSibling);
	if ((!pNear || HeapBlockTreeAccess::Black(pNear)) && (!pFar || HeapBlockTreeAccess::Black(pFar)))
	{
		HeapBlockTreeAccess::SetBlack(pSibling, false);
		arpNode = arpParent;
		arpParent = HeapBlockTreeAccess::Parent(arpParent);
		return false;
	}
	if (!pFar || HeapBlockTreeAccess::Black(pFar))
	{
		HeapBlockTreeAccess::SetBlack(pNear, true);
		HeapBlockTreeAccess::SetBlack(pSibling, false);
		Rotate<typename Side::Other>(arpRoot, pSibling);
		pSibling = Side::OtherSide(arpParent);
		pFar = Side::OtherSide(pSibling);
	}
	HeapBlockTreeAccess::SetBlack(pSibling, HeapBlockTreeAccess::Black(arpParent));
	HeapBlockTreeAccess::SetBlack(arpParent, true);
	if (pFar)
		HeapBlockTreeAccess::SetBlack(pFar, true);
	Rotate<Side>(arpRoot, arpParent);
	return true;
}

template bool RebalanceAfterRemove<LeftRight>(HeapBlockFreeHead*&, HeapBlockFreeHead*&, HeapBlockFreeHead*&);
template bool RebalanceAfterRemove<RightLeft>(HeapBlockFreeHead*&, HeapBlockFreeHead*&, HeapBlockFreeHead*&);
