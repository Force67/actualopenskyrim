cmake_minimum_required(VERSION 3.24)
project(openskyrim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# TESV/ mirrors the original code root, so includes read like the original
# ones: #include "BSCore/BSCRC32.h".
set(TESV_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/TESV)
# Win32 API subset for non-Windows builds; engine code calls Win32 as the
# original does.
set(OPENSKYRIM_PLATFORM_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/include)
set(OPENSKYRIM_PLATFORM_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Linux/src/Win32.cpp)

add_subdirectory(TESV/BSCore)
add_subdirectory(TESV/Gamebryo/CoreLibs/NiMain)
add_subdirectory(TESV/Gamebryo/CoreLibs/NiSystem)

set(OPENSKYRIM_LIBS bscore nimain nisystem CACHE INTERNAL "Engine libraries")
