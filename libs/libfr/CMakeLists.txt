cmake_minimum_required(VERSION 2.8)
project(libfr)

# C Flags
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -ggdb3 -msse4 -mfpmath=both -ffast-math -fomit-frame-pointer -finline-functions -D DEBUG -D _OPENCV")

# CXX Flags
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -ggdb3 -msse4 -mfpmath=both -ffast-math -fomit-frame-pointer -finline-functions -D DEBUG -D _OPENCV -Wno-narrowing")

# Project specific sources
set(LIB_SOURCES 
    src/libfr.cpp

    src/base_app.cpp
)

# Package finder
find_package(PkgConfig)

# Find boost package
find_package(Boost COMPONENTS program_options REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include ${Boost_INCLUDE_DIR})
add_library(libfr SHARED ${LIB_SOURCES})
target_link_libraries(libfr ${Boost_LIBRARIES} libar)
