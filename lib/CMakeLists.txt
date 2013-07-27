cmake_minimum_required(VERSION 2.8)
project(libfr)

# C Flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -ggdb3 -msse4 -D DEBUG -D _OPENCV")

# CXX Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -ggdb3 -msse4 -D DEBUG -D _OPENCV")

# Project specific includes
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/include/abr")

# Project specific sources
add_library(libfr 
  src/classifier.cpp 
  src/const.cpp 
  src/core.cpp 
  src/core_simple.cpp 
  src/core_sse.cpp 
  src/lbp.cpp 
  src/preprocess.cpp
  src/simplexml.cpp
)

# Package finder
find_package(PkgConfig)

# OpenCV dependency
pkg_check_modules(OPENCV opencv2)
include_directories(${OPENCV_INCLUDE_DIRS})
link_directories(${OPENCV_LIBRARIES})
target_link_libraries(libfr ${OPENCV_LIBS})

# LibXml2 dependency
pkg_check_modules(LIBXML2 libxml-2.0)
include_directories(${LIBXML2_INCLUDE_DIRS})
link_directories(${LIBXML2_LIBRARIES})
target_link_libraries(libfr ${LIBXML2_LIBRARIES})
