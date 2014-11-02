#! /bin/sh

###############################################################################
# Generic build script.
# Author Ben Cardoen
###############################################################################

# Assume pwd = projectdir (in other words, right after a git clone)
# Source code and CMakeLists in ./main/
# Generate build folder in ./build/

BUILD_DIR="build"

if [ -d "$BUILD_DIR" ]
	then
	if [ -L "$BUILD_DIR" ]
		then
		echo "Build directory found, but it's a link. Computer says no."
	else
        echo "Found stale build directory : Removing"
		rm -r $BUILD_DIR
	fi
fi
mkdir $BUILD_DIR
cd $BUILD_DIR

## Generate Eclipse IDE project files
# ARG1 argument is not needed for compilation but ensures the indexer in eclipse actually works.

# Uncomment to use clang++ as compiler.
#cmake -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_CXX_COMPILER_ARG1=-std=c++1y -DCMAKE_CXX_COMPILER="clang++" -DCMAKE_BUILD_TYPE=Debug ../main

# Comment to disable g++
cmake -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_CXX_COMPILER_ARG1=-std=c++1y -DCMAKE_CXX_COMPILER="g++" -DCMAKE_BUILD_TYPE=Debug ../main

# Compile & link everything in build, assuming quad core
make all -j8
