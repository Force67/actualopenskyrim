cmake_minimum_required(VERSION 3.24)
project(openskyrim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# TESV/ mirrors the original code root, so includes read like the original
# ones: #include "BSCore/BSCRC32.h".
set(TESV_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/TESV)

add_subdirectory(TESV/BSCore)

set(OPENSKYRIM_LIBS bscore CACHE INTERNAL "Engine libraries")
