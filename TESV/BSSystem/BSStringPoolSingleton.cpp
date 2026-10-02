#include "BSSystem/BSStringPool.h"
#include "BSCore/BSCore.h"

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <new>

alignas(BSStringPool::BucketTable) unsigned char BSStringPool::cBucketTableS[sizeof(BucketTable)];
int BSStringPool::iBucketTableInitS = 0;

void BSStringPool::DestroyBucketTable()
{
	auto* pTable = reinterpret_cast<BucketTable*>(cBucketTableS);
	for (auto& lock : pTable->kLocks)
		lock.~BSSpinLock();
}

BSStringPool::BucketTable& BSStringPool::BucketTable::GetSingleton()
{
	if (std::atomic_ref<int>(iBucketTableInitS).load(std::memory_order_acquire) > BSCore::iThreadInitEpochS)
	{
		BSCore::InitThreadHeader(&iBucketTableInitS);
		if (iBucketTableInitS == -1)
		{
			try
			{
				auto* pTable = new (cBucketTableS) BucketTable;
				pTable->bInitialized = true;
				std::memset(pTable->pBuckets, 0, sizeof(pTable->pBuckets));
				std::atexit(DestroyBucketTable);
				BSCore::InitThreadFooter(&iBucketTableInitS);
			}
			catch (...)
			{
				BSCore::InitThreadAbort(&iBucketTableInitS);
				throw;
			}
		}
	}
	return *reinterpret_cast<BucketTable*>(cBucketTableS);
}
