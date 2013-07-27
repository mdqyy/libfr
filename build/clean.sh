#! /bin/bash

if [ -f cmake_install.cmake ]; then
	rm cmake_install.cmake
fi

if [ -f CMakeCache.txt ]; then
	rm CMakeCache.txt
fi

if [ -f Makefile ]; then
	rm Makefile
fi

if [ -d CMakeFiles ]; then
	rm -rf CMakeFiles
fi

if [ -d apps ]; then
	rm -rf apps
fi

if [ -d lib ]; then
	rm -rf lib
fi

