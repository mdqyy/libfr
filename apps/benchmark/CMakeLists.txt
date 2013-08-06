cmake_minimum_required(VERSION 2.8)
project(benchmark)

# C Flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -msse3 -mfpmath=both -O3 -fopenmp -ffast-math -fomit-frame-pointer -finline-functions -D NDEBUG")

# CXX Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -msse3 -mfpmath=both -O3 -fopenmp -ffast-math -fomit-frame-pointer -finline-functions -D NDEBUG")

find_package(PkgConfig)

# Add library includes
include_directories(${CMAKE_SOURCE_DIR}/libs/libar/include)

set(APP_SOURCES
  benchmark.cpp
)

add_executable(benchmark ${APP_SOURCES})

# Package finder
find_package(PkgConfig)

# OpenCV dependency
pkg_check_modules(OPENCV opencv)
include_directories(${OPENCV_INCLUDE_DIRS})

# argtables2 dependency
pkg_check_modules(ARGTABLE argtable2)
set(ARGTABLE_LIBS argtable2)
include_directories(${ARGTABLE_INCLUDE_DIRS})
target_link_libraries(benchmark ${ARGTABLE_LIBRARIES})

# libar itself dependency
target_link_libraries(benchmark libfr)
