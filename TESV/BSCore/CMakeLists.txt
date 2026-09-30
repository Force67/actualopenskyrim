add_library(bscore STATIC
  BSCRC32.cpp
)
target_include_directories(bscore PUBLIC ${TESV_ROOT})
