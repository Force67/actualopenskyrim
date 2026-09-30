add_library(nimain STATIC
  NiRefObject.cpp
)
target_include_directories(nimain PUBLIC ${TESV_ROOT})
target_link_libraries(nimain PUBLIC bscore)
