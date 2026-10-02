add_library(bscore STATIC

  NewOverLoads.cpp
  Win32/BSSemaphore_Win32.cpp
  BSSpinLock.cpp
  BSCore.cpp
  BSCoreUtils.cpp
  BSCoreMessage.cpp
  BSCoreMessageHandler.cpp
  BSCoreMessageSource.cpp
  BSTEvent.cpp
  MemoryManagementEvents.cpp
  BSTimer.cpp
  Win32/BSTimer_Win32.cpp
  BSThread.cpp
  Win32/BSThread_Win32.cpp
  ../BSSystem/BSFixedString.cpp
  ../BSSystem/BSFixedStringPreserveCase.cpp
  ../BSSystem/BSStringPoolSingleton.cpp
  ../BSSystem/BSStringPoolDelimited.cpp
  ../BSSystem/BSStringPool.cpp
  ../BSSystem/BSStorage.cpp
  ../BSSystem/BSMemStorage.cpp
  ../BSSystem/BSSystemFile.cpp
  ../BSSystem/Win32/BSSystemFile_Win32.cpp
  ../BSSystem/BSStringT.cpp
  ../BSSystem/FilePathUtilities.cpp
  ../BSSystem/BSSystemDir.cpp
  ../BSSystem/Win32/BSSystemDir_Win32.cpp
  ../BSSystem/BSSearchPath.cpp
  ../Gamebryo/CoreLibs/NiSystem/NiSearchPath.cpp
  ../Gamebryo/CoreLibs/NiSystem/Win32/NiFilename.cpp
  BSTSmallArray.cpp
  BSTArray.cpp
  BSRandom.cpp
  BSStringUtilities.cpp
  BSMemoryutility.cpp
  BSPrecisionTimer.cpp
  ZeroOverheadHeap.cpp
  HeapBlocks.cpp
  BSTObjectArena.cpp
  BSSmallBlockAllocator.cpp
  CompactingStore.cpp
  CompactingStoreCallback.cpp
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
