cmake_minimum_required(VERSION 2.8)
project(camera)

# Add library includes
include_directories(${CMAKE_SOURCE_DIR}/libs/libar/include)

set(APP_SOURCES
  src/main.cpp
)

add_executable(camera ${APP_SOURCES})

# Package finder
find_package(PkgConfig)

# Find boost package
find_package(Boost COMPONENTS program_options REQUIRED)

# OpenCV dependency
# See http://opencv.willowgarage.com/wiki/CompileOpenCVUsingLinux
# See http://stackoverflow.com/questions/7417242/linking-problem-with-opencv-and-cmake
pkg_check_modules(OPENCV opencv)
include_directories(${OPENCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIR})
set(OPENCV_LIBS opencv_core opencv_imgproc opencv_calib3d opencv_video opencv_features2d opencv_ml opencv_highgui opencv_objdetect opencv_contrib opencv_legacy) # opencv_gpu
target_link_libraries(camera ${OPENCV_LIBS} ${Boost_LIBRARIES} libfr)
