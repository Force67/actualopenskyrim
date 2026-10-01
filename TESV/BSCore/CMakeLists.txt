add_library(bscore STATIC
  BSTArray.cpp
  BSRandom.cpp
  BSStringUtilities.cpp
  BSPrecisionTimer.cpp
  ZeroOverheadHeap.cpp
  HeapBlocks.cpp
  BSTObjectArena.cpp
  BSSmallBlockAllocator.cpp
  CompactingStore.cpp
  AbstractHeap.cpp
  MemoryHeap.cpp
  BSCRC32.cpp
  BSReadWriteLock.cpp
  IMemoryHeap.cpp
  IMemoryStore.cpp
  MemoryManager.cpp
  ScrapHeap.cpp
)
target_include_directories(bscore PUBLIC ${TESV_ROOT})
if(NOT WIN32)
  target_include_directories(bscore PUBLIC ${OPENSKYRIM_PLATFORM_INCLUDE})
  target_sources(bscore PRIVATE ${OPENSKYRIM_PLATFORM_SOURCES})
endif()
