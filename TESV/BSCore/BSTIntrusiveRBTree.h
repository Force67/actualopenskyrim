#pragma once

template <class Node, class Key, class Access>
class BSTIntrusiveRBTree : public Access
{
public:
	static void Insert(Node*& arpTreeRoot, Node* apInsertNode, Node*& arpCurrent);
	static void Remove(Node* apNode);

private:
	static void Rotate(Node*& arpTreeRoot, Node* apNode, bool abLeft);
};

template <class Node, class Key, class Access>
void BSTIntrusiveRBTree<Node, Key, Access>::Rotate(Node*& arpTreeRoot, Node* apNode, bool abLeft)
{
	Node* pParent = Access::Parent(apNode);
	Node* pPivot = abLeft ? Access::Right(apNode) : Access::Left(apNode);
	Node* pMiddle = abLeft ? Access::Left(pPivot) : Access::Right(pPivot);
	if (abLeft)
	{
		Access::Right(apNode) = pMiddle;
		Access::Left(pPivot) = apNode;
	}
	else
	{
		Access::Left(apNode) = pMiddle;
		Access::Right(pPivot) = apNode;
	}
	if (pMiddle)
		Access::SetParent(pMiddle, apNode);
	Access::SetParent(apNode, pPivot);
	Access::SetParent(pPivot, pParent);
	if (!pParent)
		arpTreeRoot = pPivot;
	else if (Access::Left(pParent) == apNode)
		Access::Left(pParent) = pPivot;
	else
		Access::Right(pParent) = pPivot;
}

template <class Node, class Key, class Access>
void BSTIntrusiveRBTree<Node, Key, Access>::Insert(Node*& arpTreeRoot, Node* apInsertNode, Node*& arpCurrent)
{
	Node* pParent = nullptr;
	Node* pCurrent = arpTreeRoot;
	const Key key = Access::GetKey(apInsertNode);
	while (pCurrent)
	{
		pParent = pCurrent;
		const Key currentKey = Access::GetKey(pCurrent);
		if (key == currentKey)
		{
			arpCurrent = pCurrent;
			return;
		}
		pCurrent = key < currentKey ? Access::Left(pCurrent) : Access::Right(pCurrent);
	}
	Access::Root(apInsertNode) = &arpTreeRoot;
	Access::Left(apInsertNode) = nullptr;
	Access::Right(apInsertNode) = nullptr;
	Access::SetBlack(apInsertNode, !pParent);
	Access::SetParent(apInsertNode, pParent);
	if (!pParent)
		arpTreeRoot = apInsertNode;
	else if (key < Access::GetKey(pParent))
		Access::Left(pParent) = apInsertNode;
	else
		Access::Right(pParent) = apInsertNode;
	Node* pNode = apInsertNode;
	while (pParent && !Access::Black(pParent))
	{
		Node* pGrandparent = Access::Parent(pParent);
		const bool bLeft = Access::Left(pGrandparent) == pParent;
		Node* pUncle = bLeft ? Access::Right(pGrandparent) : Access::Left(pGrandparent);
		if (pUncle && !Access::Black(pUncle))
		{
			Access::SetBlack(pParent, true);
			Access::SetBlack(pUncle, true);
			pNode = pGrandparent;
			pParent = Access::Parent(pNode);
			if (pParent)
				Access::SetBlack(pNode, false);
		}
		else
		{
			if (pNode == (bLeft ? Access::Right(pParent) : Access::Left(pParent)))
			{
				Rotate(arpTreeRoot, pParent, bLeft);
				pParent = pNode;
			}
			Access::SetBlack(pParent, true);
			Access::SetBlack(pGrandparent, false);
			Rotate(arpTreeRoot, pGrandparent, !bLeft);
			break;
		}
	}
	arpCurrent = apInsertNode;
}

template <class Node, class Key, class Access>
void BSTIntrusiveRBTree<Node, Key, Access>::Remove(Node* apNode)
{
	Node*& rpRoot = *Access::Root(apNode);
	Node* pRemoved = apNode;
	Node* pChild;
	Node* pParent;
	bool bBlack = Access::Black(apNode);
	if (!Access::Left(apNode) || !Access::Right(apNode))
	{
		pChild = Access::Left(apNode) ? Access::Left(apNode) : Access::Right(apNode);
		pParent = Access::Parent(apNode);
		if (pChild)
			Access::SetParent(pChild, pParent);
		if (!pParent)
			rpRoot = pChild;
		else if (Access::Left(pParent) == apNode)
			Access::Left(pParent) = pChild;
		else
			Access::Right(pParent) = pChild;
	}
	else
	{
		pRemoved = Access::Right(apNode);
		while (Access::Left(pRemoved))
			pRemoved = Access::Left(pRemoved);
		bBlack = Access::Black(pRemoved);
		pChild = Access::Right(pRemoved);
		pParent = Access::Parent(pRemoved);
		if (pParent == apNode)
			pParent = pRemoved;
		else
		{
			Access::Left(pParent) = pChild;
			if (pChild)
				Access::SetParent(pChild, pParent);
			Access::Right(pRemoved) = Access::Right(apNode);
			Access::SetParent(Access::Right(pRemoved), pRemoved);
		}
		Node* pOldParent = Access::Parent(apNode);
		if (!pOldParent)
			rpRoot = pRemoved;
		else if (Access::Left(pOldParent) == apNode)
			Access::Left(pOldParent) = pRemoved;
		else
			Access::Right(pOldParent) = pRemoved;
		Access::SetParent(pRemoved, pOldParent);
		Access::SetBlack(pRemoved, Access::Black(apNode));
		Access::Left(pRemoved) = Access::Left(apNode);
		Access::SetParent(Access::Left(pRemoved), pRemoved);
	}
	if (!bBlack)
		return;
	while (pChild != rpRoot && (!pChild || Access::Black(pChild)))
	{
		const bool bLeft = pChild == Access::Left(pParent);
		Node* pSibling = bLeft ? Access::Right(pParent) : Access::Left(pParent);
		if (!Access::Black(pSibling))
		{
			Access::SetBlack(pSibling, true);
			Access::SetBlack(pParent, false);
			Rotate(rpRoot, pParent, bLeft);
			pSibling = bLeft ? Access::Right(pParent) : Access::Left(pParent);
		}
		Node* pNear = bLeft ? Access::Left(pSibling) : Access::Right(pSibling);
		Node* pFar = bLeft ? Access::Right(pSibling) : Access::Left(pSibling);
		if ((!pNear || Access::Black(pNear)) && (!pFar || Access::Black(pFar)))
		{
			Access::SetBlack(pSibling, false);
			pChild = pParent;
			pParent = Access::Parent(pParent);
		}
		else
		{
			if (!pFar || Access::Black(pFar))
			{
				Access::SetBlack(pNear, true);
				Access::SetBlack(pSibling, false);
				Rotate(rpRoot, pSibling, !bLeft);
				pSibling = bLeft ? Access::Right(pParent) : Access::Left(pParent);
				pFar = bLeft ? Access::Right(pSibling) : Access::Left(pSibling);
			}
			Access::SetBlack(pSibling, Access::Black(pParent));
			Access::SetBlack(pParent, true);
			if (pFar)
				Access::SetBlack(pFar, true);
			Rotate(rpRoot, pParent, bLeft);
			break;
		}
	}
	if (pChild)
		Access::SetBlack(pChild, true);
}
