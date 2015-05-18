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

echo "$SCRIPT Detecting nr of CPU's to use for make ....".

# Nr of thread for make
NRCPU=4

if hash nproc 2>/dev/null;
then 
	NRCPU=`nproc`
	echo "$SCRIPT Found $NRCPU Cores."
	NRCPU=$((2*$NRCPU))
	echo "$SCRIPT Using $NRCPU threads for make."
fi

## Set default values of build variables.
BUILD_TYPE="Debug"
COMPILER="g++"
ECLIPSE_INDEXER_ARGS="-std=c++11"
BUILD_DIR="build"

# Detect if we are on cygwin, if we are Eclipse's cdt needs gnu++11, if not we need c++11 (or face horrors in the ide)
OSNAME=$(uname -o)
echo "$SCRIPT Running script on OS::  $OSNAME"
if [ "$OSNAME" == "Cygwin" ]
then
    ECLIPSE_INDEXER_ARGS="-std=gnu++11"
    echo "$SCRIPT detected Cygwin,  changing eclipse indexer to $ECLIPSE_INDEXER_ARGS"
fi

# Override build type with first argument
if [ "$#" -ge 1 ]
then
    BUILD_TYPE="$1"
    echo "$SCRIPT  Overriding BUILD_TYPE with value $1 ."
else
    echo "$SCRIPT  Using Default BUILD_TYPE :: $BUILD_TYPE"
fi

# Override compiler invocation with second argument
if [ "$#" -eq 2 ]
then
    COMPILER="$2"
    echo "$SCRIPT  Overriding Compiler choice with value :: $2"
fi

# Fix root path testfiles. (filecompare trips over win<>*nix)
find testfiles -type f -execdir dos2unix -q {} \;

# If stale build is found, try to remove it.
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

# Apply dos2unix to avoid errors in filecmp across systems. (in theory line 46 covers this, but it has failed before)
find testfiles -type f -execdir dos2unix -q {} \;

echo "$SCRIPT Generating CMake Build."
## Generate Eclipse IDE project files
# ARG1 argument is not needed for compilation but ensures the indexer in eclipse actually works.
cmake -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_CXX_COMPILER_ARG1="$ECLIPSE_INDEXER_ARGS" -DCMAKE_CXX_COMPILER="$COMPILER" -DCMAKE_BUILD_TYPE=$BUILD_TYPE ../main

echo "$SCRIPT Building project .... "
# Compile & link everything in build, assuming quad core
make all -j$NRCPU
