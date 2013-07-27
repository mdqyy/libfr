#! /bin/bash

# Generate CMake project
cmake ../

# Clean old files
make clean

# Make new files
make -f Makefile

