cmake_minimum_required(VERSION 2.8)
project(benchmark)

find_package(PkgConfig)

include_directories(${CMAKE_SOURCE_DIR}/lib/include)
add_executable(benchmark benchmark.cpp)


# Package finder
find_package(PkgConfig)

# OpenCV dependency
pkg_check_modules(OPENCV opencv2)
include_directories(${OPENCV_INCLUDE_DIRS})
link_directories(${OPENCV_LIBRARIES})


target_link_libraries(benchmark ${OPENCV_LIBS})

# libfr itself dependency
target_link_libraries(benchmark libfr)