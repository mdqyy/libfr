cmake_minimum_required(VERSION 2.8)
project(detector)

# C Flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -msse3 -mfpmath=both -O3 -fopenmp -ffast-math -fomit-frame-pointer -finline-functions -D NDEBUG")

# CXX Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -msse3 -mfpmath=both -O3 -fopenmp -ffast-math -fomit-frame-pointer -finline-functions -D NDEBUG")

find_package(PkgConfig)

# Add library includes
include_directories("include")

set(APP_SOURCES
  src/classifier.cpp
  src/demo.cpp
  src/image.cpp
  src/lrd_engine.cpp
  src/simplexml.cpp
#  src/wbdetect.cpp
)

add_executable(detector ${APP_SOURCES})

# Package finder
find_package(PkgConfig)

# OpenCV dependency
# See http://opencv.willowgarage.com/wiki/CompileOpenCVUsingLinux
# See http://stackoverflow.com/questions/7417242/linking-problem-with-opencv-and-cmake
pkg_check_modules(OPENCV opencv)
include_directories(${OPENCV_INCLUDE_DIRS})
set(OPENCV_LIBS opencv_core opencv_imgproc opencv_calib3d opencv_video opencv_features2d opencv_ml opencv_highgui opencv_objdetect opencv_contrib opencv_legacy opencv_gpu)
target_link_libraries(detector ${OPENCV_LIBS})

# argtables2 dependency
pkg_check_modules(ARGTABLE argtable2)
set(ARGTABLE_LIBS argtable2)
include_directories(${ARGTABLE_INCLUDE_DIRS})
target_link_libraries(detector ${ARGTABLE_LIBRARIES})

# LibXml2 dependency
pkg_check_modules(LIBXML2 libxml-2.0)
include_directories(${LIBXML2_INCLUDE_DIRS})
target_link_libraries(detector ${LIBXML2_LIBRARIES})