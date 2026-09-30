add_library(bscore STATIC
  BSCRC32.cpp
  BSReadWriteLock.cpp
)
target_include_directories(bscore PUBLIC ${TESV_ROOT})
if(NOT WIN32)
  target_include_directories(bscore PUBLIC ${OPENSKYRIM_PLATFORM_INCLUDE})
endif()
