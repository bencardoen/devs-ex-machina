#! /bin/sh

###############################################################################
# Generic build script.
# Author Ben Cardoen
# Given sourcecode & CMake , runs CMake with preconfigured options.
###############################################################################

# Assume pwd = projectdir (in other words, right after a git clone)
# Source code and CMakeLists in ./main/
# Generate build folder in ./build/
SCRIPT="__SCRIPT__:"
echo "$SCRIPT Received "$#" arguments".
BUILD_TYPE="Debug"
COMPILER="g++"
if [ "$#" -ge 1 ]
then
    BUILD_TYPE="$1"
    echo "$SCRIPT  Overriding BUILD_TYPE with value $1 ."
else
    echo "$SCRIPT  Using Default BUILD_TYPE :: $BUILD_TYPE"
fi

if [ "$#" -eq 2 ]
then
    COMPILER="$2"
    echo "$SCRIPT  Overriding Compiler choice with value :: $2"
fi

# Fix root path testfiles.
find testfiles -type f -execdir dos2unix {} \;

BUILD_DIR="build"

if [ -d "$BUILD_DIR" ]
	then
    if [ -k "$BUILD_DIR" ]
        then
        echo "$SCRIPT Build directory exists but can't be removed, quitting."
        exit -1
    fi
	if [ -L "$BUILD_DIR" ]
		then
		echo "$SCRIPT Build directory found, but it's a link. Computer says no."
        exit -1
	else
        echo "$SCRIPT Found stale build directory : Removing"
		rm -r $BUILD_DIR
	fi
fi
mkdir $BUILD_DIR
cd $BUILD_DIR

# Copy testfiles to generated directory so that eclipse finds them (eclipse cwd is BUILD_DIR)
mkdir "testfiles"
cp -r ../testfiles/* testfiles/

# Apply dos2unix to avoid errors in filecmp across systems.
find testfiles -type f -execdir dos2unix {} \;

echo "$SCRIPT Generating CMake Build."
## Generate Eclipse IDE project files
# ARG1 argument is not needed for compilation but ensures the indexer in eclipse actually works.

# Uncomment to use clang++ as compiler.
cmake -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_CXX_COMPILER_ARG1=-std=gnu++11 -DCMAKE_CXX_COMPILER="$COMPILER" -DCMAKE_BUILD_TYPE=$BUILD_TYPE ../main


echo "$SCRIPT Building project .... "
# Compile & link everything in build, assuming quad core
make all -j8
