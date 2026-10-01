#pragma once

#include <cstddef>
#include <cstdint>

namespace CompactingStore
{
	struct BlockHeader
	{
		size_t uiSize;
	};
	struct FreeBlock : BlockHeader
	{
		FreeBlock* pLeft;
		FreeBlock* pRight;
	};
	struct StoreBlock
	{
		union
		{
			void* pAddress;
			StoreBlock* pNext;
		};
		volatile uint32_t uiAccessFlags;
	};
	static_assert(sizeof(StoreBlock) == 0x10);
	static_assert(offsetof(StoreBlock, uiAccessFlags) == 0x8);

	class MoveCallback
	{
	public:
		virtual void operator()(void* apDest, const void* apSrc, size_t auiSize) = 0;
		virtual ~MoveCallback();
	};

	class NoopMoveCallback : public MoveCallback
	{
	public:
		void operator()(void* apDest, const void* apSrc, size_t auiSize) override;
		~NoopMoveCallback() override = default;
		static NoopMoveCallback instance;
	};

	struct AllocatedBlock : BlockHeader
	{
		StoreBlock* pOwner;
	};
	static_assert(sizeof(AllocatedBlock) == 0x10);
	void ExecuteMove(void* apDest, void* apSrc, size_t auiSizeFlags);
	class Accessor;
	struct HandleType
	{
		HandleType() : pStoreBlock(nullptr) {}
		~HandleType();
		void Access(Accessor& arAccessor) const;
		StoreBlock* pStoreBlock;
	};

	class Accessor
	{
	public:
		Accessor();
		explicit Accessor(StoreBlock* apStoreBlock);
		Accessor(Accessor& arRhs);
		explicit Accessor(const HandleType& arRhs);
		~Accessor();
		Accessor& operator=(Accessor& arRhs);
		Accessor& operator=(const HandleType& arRhs);
		void Replicate(Accessor& arDest) const;
		static void* BeginStoreBlockAccess(volatile StoreBlock* apStoreBlock);
		static void EndStoreBlockAccess(volatile StoreBlock* apStoreBlock);
		operator void*() const { return pAddress; }
		bool operator==(const Accessor& arRhs) const { return pStoreBlock == arRhs.pStoreBlock; }
		bool operator!=(const Accessor& arRhs) const { return pStoreBlock != arRhs.pStoreBlock; }

		StoreBlock* pStoreBlock;
		void* pAddress;
	};
	static_assert(sizeof(Accessor) == 0x10);
}
