cmake_minimum_required(VERSION 2.8)
project(demo)

# Add library includes
include_directories(${CMAKE_SOURCE_DIR}/libs/libar/include)

set(APP_SOURCES
  src/main.cpp
)

add_executable(demo ${APP_SOURCES})

# Package finder
find_package(PkgConfig)

# Find boost package
find_package(Boost COMPONENTS program_options REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
target_link_libraries(demo ${Boost_LIBRARIES} libfr)
