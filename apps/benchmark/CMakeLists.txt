cmake_minimum_required(VERSION 2.8)
project(benchmark)

# C Flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -msse3 -mfpmath=both -O3 -fopenmp -ffast-math -fomit-frame-pointer -finline-functions -D NDEBUG")

# CXX Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -msse3 -mfpmath=both -O3 -fopenmp -ffast-math -fomit-frame-pointer -finline-functions -D NDEBUG")

find_package(PkgConfig)

include_directories(${CMAKE_SOURCE_DIR}/lib/include)
add_executable(benchmark benchmark.cpp)


# Package finder
find_package(PkgConfig)

# OpenCV dependency
pkg_check_modules(OPENCV opencv)
include_directories(${OPENCV_INCLUDE_DIRS})
# link_directories(${OPENCV_LIBRARIES})

target_link_libraries(benchmark ${OPENCV_LIBS} argtable2)

# libfr itself dependency
target_link_libraries(benchmark libfr)