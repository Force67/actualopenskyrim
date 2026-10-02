add_library(nisystem STATIC
  BSNiAllocator.cpp
  NiBinaryStream.cpp
  NiFile.cpp
  NiInitOptions.cpp
  NiStaticDataManager.cpp
  NiMemManager.cpp
  NiMemoryDefines.cpp
  NiMemStream.cpp
  NiPath.cpp
  NiSearchPath.cpp
  NiSmallObjectAllocator.cpp
  NiStandardAllocator.cpp
  NiSystemSDM.cpp
  NiThreadProcedure.cpp
  NiThread.cpp
  NiThread_Win32.cpp
  Win32/NiSystemDesc.cpp
  Win32/NiFilename.cpp
  Win32/NiFile_Win32.cpp
  Win32/NiMessageBox.cpp
  Win32/NiSystem.cpp
)
target_include_directories(nisystem PUBLIC ${TESV_ROOT})
target_link_libraries(nisystem PUBLIC bscore)
