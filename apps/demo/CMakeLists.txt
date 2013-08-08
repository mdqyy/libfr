cmake_minimum_required(VERSION 2.8)
project(demo)

# Add library includes
include_directories(${CMAKE_SOURCE_DIR}/libs/libar/include)
include_directories(${CMAKE_SOURCE_DIR}/libs/libfr/include)

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

# See http://qt-project.org/quarterly/view/using_cmake_to_build_qt_projects
# find_package(Qt4 REQUIRED)

# include(${QT_USE_FILE})
# add_definitions(${QT_DEFINITIONS})

# See http://qt.developpez.com/doc/5.0-snapshot/cmake-manual/
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Core)

# Find the QtWidgets library
find_package(Qt5Widgets)

set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_RELEASE} -fprofile-arcs -ftest-coverage")

# set up a mapping so that the Release configuration for the Qt imported target is
# used in the COVERAGE CMake configuration.
set_target_properties(Qt5::Core PROPERTIES MAP_IMPORTED_CONFIG_COVERAGE "RELEASE")

# Use the Widgets module from Qt 5.
qt5_use_modules(demo Widgets)

# Google test package
# See http://www.rad.upenn.edu/sbia/software/basis/apidoc/latest/group__CMakeFindModules.html
# See http://www.rad.upenn.edu/sbia/software/basis/apidoc/latest/FindGTest_8cmake.html
find_package(GTest)
