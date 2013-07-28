cmake_minimum_required(VERSION 2.8)
project(libar)

# C Flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -ggdb3 -msse4 -mfpmath=both -ffast-math -fomit-frame-pointer -finline-functions -D DEBUG -D _OPENCV")

# CXX Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -ggdb3 -msse4 -mfpmath=both -ffast-math -fomit-frame-pointer -finline-functions -D DEBUG -D _OPENCV -Wno-narrowing")

# Project specific includes
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/include/abr")

# Project specific sources
set(LIB_SOURCES 
  src/classifier.cpp 
  src/const.cpp 
  src/core.cpp 
  src/core_simple.cpp 
  src/core_sse.cpp 
  src/lbp.cpp 
  src/preprocess.cpp
  src/simplexml.cpp
)

add_library(libar ${LIB_SOURCES})

# Package finder
find_package(PkgConfig)

# OpenCV dependency
# See http://opencv.willowgarage.com/wiki/CompileOpenCVUsingLinux
# See http://stackoverflow.com/questions/7417242/linking-problem-with-opencv-and-cmake
pkg_check_modules(OPENCV opencv)
include_directories(${OPENCV_INCLUDE_DIRS})
set(OPENCV_LIBS opencv_core opencv_imgproc opencv_calib3d opencv_video opencv_features2d opencv_ml opencv_highgui opencv_objdetect opencv_contrib opencv_legacy opencv_gpu)
target_link_libraries(libar ${OPENCV_LIBS})

# LibXml2 dependency
pkg_check_modules(LIBXML2 libxml-2.0)
include_directories(${LIBXML2_INCLUDE_DIRS})
target_link_libraries(libar ${LIBXML2_LIBRARIES})

# FFmpeg dependency
# pkg_check_modules(FFMPEG ffmpeg)
# include_directories(${FFMPEG_INCLUDE_DIRS})
# target_link_libraries(libar ${FFMPEG_LIBRARIES})
