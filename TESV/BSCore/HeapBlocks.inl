#pragma once

inline size_t HeapBlock::GetSize() const { return uiMemSize & SIZE_MASK; }
inline size_t HeapBlock::TotalSize(size_t auiSize) { return ((auiSize + 15) & ~size_t(15)) + sizeof(HeapBlock); }
inline size_t HeapBlock::GetTotalSize() const { return TotalSize(GetSize()); }
inline bool HeapBlock::GuardInTact() const { return true; }
inline bool HeapBlock::IsMarkedFree() const { return (uiMemSize & FREE) != 0; }
inline bool HeapBlock::IsFreeHead() const { return (uiMemSize & (FREE | FREE_HEAD)) == (FREE | FREE_HEAD); }
inline bool HeapBlock::IsDecommitted() const { return (uiMemSize & DECOMMITTED) != 0; }
inline void HeapBlock::SetSize(size_t auiSize) { uiMemSize = (uiMemSize & ~SIZE_MASK) | auiSize; }
inline void HeapBlock::SetAsFree() { uiMemSize |= FREE; }
inline void HeapBlock::SetAsAllocated() { uiMemSize &= ~FREE; }
inline void HeapBlock::SetAsAllocatedWithSize(size_t auiSize) { uiMemSize = auiSize; }
inline void HeapBlock::SetFreeHead(bool abFreeHead) { if (abFreeHead) uiMemSize |= FREE_HEAD; else uiMemSize &= ~FREE_HEAD; }
inline void HeapBlock::SetAsDecommitted() { uiMemSize |= DECOMMITTED; }
inline void HeapBlock::SetAsNotDecommitted() { uiMemSize &= ~DECOMMITTED; }
inline void HeapBlock::MarkDecommitted(bool abDecommitted) { if (abDecommitted) SetAsDecommitted(); else SetAsNotDecommitted(); }
inline void HeapBlock::InheritDecommitted(const HeapBlock* apBlock) { uiMemSize |= apBlock->uiMemSize & DECOMMITTED; }
inline char* HeapBlock::GetMem(size_t aiOffset) { return reinterpret_cast<char*>(this + 1) + aiOffset; }
inline HeapBlock* HeapBlock::MemToBlock(const void* apMemory) { return reinterpret_cast<HeapBlock*>(const_cast<char*>(static_cast<const char*>(apMemory)) - sizeof(HeapBlock)); }
inline HeapBlock* HeapBlock::Next() { return reinterpret_cast<HeapBlock*>(reinterpret_cast<char*>(this + 1) + GetSize()); }
inline HeapBlock* HeapBlock::Prev() { return pPrevious; }
inline HeapBlock* HeapBlock::NextFree() { return pNextFree; }
inline HeapBlock* HeapBlock::PrevFree() { return FreeOrUsed.pPrevFree; }
inline void HeapBlock::SetPrev(HeapBlock* apBlock) { pPrevious = apBlock; }
inline void HeapBlock::SetNextFree(HeapBlock* apBlock) { pNextFree = apBlock; }
inline void HeapBlock::SetPrevFree(HeapBlock* apBlock) { FreeOrUsed.pPrevFree = apBlock; }
inline bool HeapBlock::ListInsertBefore(HeapBlock* apBefore)
{
	pNextFree = apBefore;
	FreeOrUsed.pPrevFree = apBefore->FreeOrUsed.pPrevFree;
	if (FreeOrUsed.pPrevFree)
		FreeOrUsed.pPrevFree->pNextFree = this;
	apBefore->FreeOrUsed.pPrevFree = this;
	return !FreeOrUsed.pPrevFree;
}
inline bool HeapBlock::ListInsertAfter(HeapBlock* apAfter)
{
	FreeOrUsed.pPrevFree = apAfter;
	pNextFree = apAfter->pNextFree;
	if (pNextFree)
		pNextFree->FreeOrUsed.pPrevFree = this;
	apAfter->pNextFree = this;
	return !pNextFree;
}
inline void HeapBlock::ListRemove()
{
	if (FreeOrUsed.pPrevFree)
		FreeOrUsed.pPrevFree->pNextFree = pNextFree;
	if (pNextFree)
		pNextFree->FreeOrUsed.pPrevFree = FreeOrUsed.pPrevFree;
	FreeOrUsed.pPrevFree = nullptr;
	pNextFree = nullptr;
}
inline HeapBlockFreeHead* HeapBlock::GetFreeHead() { return static_cast<HeapBlockFreeHead*>(this); }
inline HeapBlockFreeHead* HeapBlockFreeHead::QLeftChild() const { return pLeftChild; }
inline HeapBlockFreeHead* HeapBlockFreeHead::QRightChild() const { return pRightChild; }
inline HeapBlockFreeHead* HeapBlockFreeHead::QParent() const { return reinterpret_cast<HeapBlockFreeHead*>(uiParentPtrAndBlackBit & ~uintptr_t(1)); }
inline HeapBlockFreeHead** HeapBlockFreeHead::QRootPtr() const { return ppRoot; }
inline void HeapBlockFreeHead::SetLeftChild(HeapBlockFreeHead* apChild) { pLeftChild = apChild; }
inline void HeapBlockFreeHead::SetRightChild(HeapBlockFreeHead* apChild) { pRightChild = apChild; }
inline void HeapBlockFreeHead::SetParent(HeapBlockFreeHead* apParent) { uiParentPtrAndBlackBit = reinterpret_cast<uintptr_t>(apParent) | (uiParentPtrAndBlackBit & 1); }
inline bool HeapBlockFreeHead::IsBlack() const { return (uiParentPtrAndBlackBit & 1) != 0; }
inline bool HeapBlockFreeHead::IsRed() const { return !IsBlack(); }
inline void HeapBlockFreeHead::SetBlack(bool abBlack) { uiParentPtrAndBlackBit = (uiParentPtrAndBlackBit & ~uintptr_t(1)) | uintptr_t(abBlack); }
inline void HeapBlockFreeHead::SetRed(bool abRed) { SetBlack(!abRed); }
