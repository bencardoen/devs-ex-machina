#!/bin/bash

###############################################################################
# Generic build script.
#
# This file is part of the DEVS Ex Machina project.
# Copyright 2014 - 2015 University of Antwerp
# https://www.uantwerpen.be/en/
# Licensed under the EUPL V.1.1
# A full copy of the license is in COPYING.txt, or can be found at
# https://joinup.ec.europa.eu/community/eupl/og_page/eupl
#      Author: Stijn Manhaeve
#
# Given sourcecode & CMake , runs CMake with preconfigured options.
###############################################################################

#some enum values
e_DEBUG=0
e_RELEASE=1
e_BENCHMARK=2
# This v type is used for any permutation of different compile time parameters, (FRNG, pools, ..)
e_BENCHMARKFRNG=3

#some other values
SCRIPT="__SCRIPT__:"
SCRIPTNAME="${0##*/}"
BUILD_DIR="build"
DEBUG_DIR="Debug"
RELEASE_DIR="Release"
BMARK_DIR="Benchmark"
BMARKFRNG_DIR="BenchmarkFrng"
COMPILER="g++"
DOECLIPSE=false
FORCE_BUILD=""
FORCE_DELETE=false
EXTRA_ARGS=""

BUILDCHOICE=e_DEBUG
BUILD_DEBUG=()
BUILD_RELEASE=()
BUILD_BENCHMARK=()
BUILD_BENCHMARKFRNG=()

NRCPU=4
if hash nproc 2>/dev/null;
then 
  NRCPU=`nproc`
  # NRCPU=$((2*$NRCPU))
fi

# argument parsing
while [[ $# > 0 ]]
do
key="$1"
# echo "$SCRIPT key: $key"

case $key in
    -c|--compiler)
    COMPILER="$2"
    shift # past argument
    ;;
    -e|--eclipse)
    DOECLIPSE=true
    ;;
    -f|--force-build)
    FORCE_BUILD="--always-make"
    ;;
    -F|--force-delete)
    FORCE_DELETE=true
    ;;
    -d|--debug)
    BUILDCHOICE=$e_DEBUG
    ;;
    -r|--release)
    BUILDCHOICE=$e_RELEASE
    ;;
    -b|--benchmark)
    BUILDCHOICE=$e_BENCHMARK
    ;;
    -q|--benchmarkfrng)
    BUILDCHOICE=$e_BENCHMARKFRNG
    ;;
    -x|--extra)
    EXTRA_ARGS="$EXTRA_ARGS $2"
    shift
    echo "Setting '$EXTRA_ARGS' as optional argument string for cmake."
    ;;
    -h|--help)
    bold=$(tput bold)
    normal=$(tput sgr0)
    echo "${bold}usage:${normal}"
    echo "  $SCRIPTNAME [-c COMPILER] [-e] [-f] [-F] [-h] [-j NRCPU]"
    echo "              [-d TARGET [TARGET [...]]]"
    echo "              [-r TARGET [TARGET [...]]]"
    echo "              [-b TARGET [TARGET [...]]]"
    echo "              [-q TARGET [TARGET [...]]]"
    echo ""
    echo "${bold}parameters:${normal}"
    echo "  -c, --compiler COMPILER"
    echo "           Set the compiler. Default value is g++"
    echo "  -e, --eclipse"
    echo "           Generate Eclipse project files."
    echo "  -f, --force-build"
    echo "           Force make to build the target."
    echo "  -F, --force-delete"
    echo "           Delete the current build folder, if it exists."
    echo "  -h, --help"
    echo "           Show this help message and exit."
    echo "  -j NRCPU"
    echo "           Maximum amount of simultaneous jobs when building."
    echo "  -d, --debug"
    echo "           A list of build targets for the Debug build type."
    echo "  -r, --release"
    echo "           A list of build targets for the Release build type."
    echo "  -b, --benchmark"
    echo "           A list of build targets for the Benchmark build type."
    echo "  -q, --benchmarkfrng"
    echo "           A list of build targets for the Benchmark build type, extended with the fast rngs."
    echo "  -x, --extra ARGS"
    echo "           pass ARGS to CMake, if you need to override any compile time setting. Example : \"-DFASTRNG=ON -DPOOL_SINGLE_ARENA_DYNAMIC\" "
    echo "       legal:  -D{POOL_SINGLE_ARENA | POOL_SINGLE_STL | POOL_MULTI_STL | FASTRNG}=ON Note that benchmarkfrng implies FRNG"
    echo "               -DSHOWSTAT=ON enables statistics gathering."
    echo "       obviously only 1 value applies to SINGLE, and these extra args are passed only to the benchmark target"
    echo ""
    echo "${bold}notes:${normal}"
    echo " - If the script finds a makefile in one of the subfolders,"
    echo "   it will not try to overwrite it because that may corrupt the CMake cache."
    echo "   If you want to regenerate the makefile of that particular build type,"
    echo "   you can always just delete that folder."
    exit
    ;;
    -j)
    NRCPU="$2"
    shift # past argument
    ;;
    *)
    # unknown option
    # echo "$SCRIPT __debug build choice: $BUILDCHOICE"
    if [ $BUILDCHOICE = $e_DEBUG ]
    then
        # echo "$SCRIPT debug build selected!"
        BUILD_DEBUG+=($key)
    fi
    if [ $BUILDCHOICE = $e_RELEASE ]
    then
        # echo "$SCRIPT release build selected!"
        BUILD_RELEASE+=($key)
    fi
    if [ $BUILDCHOICE = $e_BENCHMARK ]
    then
        # echo "$SCRIPT benchmark build selected!"
        BUILD_BENCHMARK+=($key)
    fi
    if [ $BUILDCHOICE = $e_BENCHMARKFRNG ]
    then
        # echo "$SCRIPT benchmarkfrng build selected!"
        BUILD_BENCHMARKFRNG+=($key)
    fi
    ;;
esac
shift # past argument or value
done

echo "$SCRIPT debug builds planned: ${BUILD_DEBUG[@]}"
echo "$SCRIPT release builds planned: ${BUILD_RELEASE[@]}"
echo "$SCRIPT benchmark builds planned: ${BUILD_BENCHMARK[@]}"
echo "$SCRIPT benchmarkfrng builds planned: ${BUILD_BENCHMARKFRNG[@]}"

echo "$SCRIPT Setting up basic build environment."
echo "$SCRIPT data:  main build directory: $BUILD_DIR"
echo "$SCRIPT data: debug build directory: $BUILD_DIR/$DEBUG_DIR"
echo "$SCRIPT data: release build directory: $BUILD_DIR/$RELEASE_DIR"
echo "$SCRIPT data: benchmark build directory: $BUILD_DIR/$BMARK_DIR"
echo "$SCRIPT data: benchmarkfrng build directory: $BUILD_DIR/$BMARKFRNG_DIR"
echo "$SCRIPT data: compiler: $COMPILER"
echo "$SCRIPT data: force delete build folder if it already exists: $FORCE_DELETE"
echo "$SCRIPT data: number of simultaneous jobs: $NRCPU"

# If stale build is found, try to remove it.
if [ "$FORCE_DELETE" = true ]
  then
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
fi

echo "$SCRIPT moving to ./$BUILD_DIR"
mkdir -p $BUILD_DIR
cd $BUILD_DIR

echo "$SCRIPT moving to ./$DEBUG_DIR"
mkdir -p $DEBUG_DIR
cd $DEBUG_DIR
if [ ! -f "Makefile" ]
  then
  cmake "$CMAKE_ARG_STRING" -DCMAKE_BUILD_TYPE=Debug -DTOTOP="../../" ../../main
fi
if [ ${#BUILD_DEBUG[@]} -ne 0 ]
  then
  echo "$SCRIPT building debug targets ${BUILD_DEBUG[@]}"
  make -j$NRCPU $FORCE_BUILD $i ${BUILD_DEBUG[@]}
fi
echo "$SCRIPT moving back to parent directory."
cd ../

echo "$SCRIPT moving to ./$RELEASE_DIR"
mkdir -p $RELEASE_DIR
cd $RELEASE_DIR
if [ ! -f "Makefile" ]
  then
  cmake $EXTRA_ARGS -DCMAKE_CXX_COMPILER=$COMPILER -DFASTRNG=ON -DCMAKE_BUILD_TYPE=Release -DTOTOP="../../" ../../main
fi
if [ ${#BUILD_RELEASE[@]} -ne 0 ]
  then
  echo "$SCRIPT building debug targets ${BUILD_RELEASE[@]}"
  make -j$NRCPU $FORCE_BUILD $i ${BUILD_RELEASE[@]}
fi
echo "$SCRIPT moving back to parent directory."
cd ../

echo "$SCRIPT moving to ./$BMARK_DIR"
mkdir -p $BMARK_DIR
cd $BMARK_DIR
if [ ! -f "Makefile" ]
  then
  cmake $EXTRA_ARGS -DCMAKE_CXX_COMPILER=$COMPILER -DCMAKE_BUILD_TYPE=Benchmark -DTOTOP="../../" ../../main
fi
if [ ${#BUILD_BENCHMARK[@]} -ne 0 ]
  then
  echo "$SCRIPT building debug targets ${BUILD_BENCHMARK[@]}"
  make -j$NRCPU $FORCE_BUILD $i ${BUILD_BENCHMARK[@]}
fi
echo "$SCRIPT moving back to parent directory."
cd ../

echo "$SCRIPT moving to ./$BMARKFRNG_DIR"
mkdir -p $BMARKFRNG_DIR
cd $BMARKFRNG_DIR
if [ ! -f "Makefile" ]
  then
  cmake $EXTRA_ARGS -DFASTRNG=ON -DCMAKE_CXX_COMPILER="$COMPILER" -DCMAKE_BUILD_TYPE=BenchmarkFrng -DTOTOP="../../" ../../main
fi
if [ ${#BUILD_BENCHMARKFRNG[@]} -ne 0 ]
  then
  echo "$SCRIPT building debug targets ${BUILD_BENCHMARKFRNG[@]}"
  make -j$NRCPU $FORCE_BUILD $i ${BUILD_BENCHMARKFRNG[@]}
fi
echo "$SCRIPT moving back to parent directory."
cd ../

if [ "$DOECLIPSE" = true ]
  then
  echo "$SCRIPT generating Eclipse project files."
  cmake -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_CXX_COMPILER="$COMPILER" -DCMAKE_BUILD_TYPE=Debug -DTOTOP="../" ../main
fi

echo "$SCRIPT moving back to parent directory."
cd ../
echo "$SCRIPT done. targets built:"
echo "$SCRIPT  -> debug"
for i in "${BUILD_DEBUG[@]}" ; do
  echo "$SCRIPT        $i"
done
echo "$SCRIPT  -> release"
for i in "${BUILD_RELEASE[@]}" ; do
  echo "$SCRIPT        $i"
done
echo "$SCRIPT  -> benchmark"
for i in "${BUILD_BENCHMARK[@]}" ; do
  echo "$SCRIPT        $i"
done
echo "$SCRIPT  -> benchmarkfrng"
for i in "${BUILD_BENCHMARKFRNG[@]}" ; do
  echo "$SCRIPT        $i"
done
if [ "$DOECLIPSE" = true ]
  then
  echo "$SCRIPT  -> Eclipse project files"
fi