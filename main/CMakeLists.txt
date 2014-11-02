cmake_minimum_required(VERSION 2.8)

### Generic CMakeLists.txt file ###############################################
#   Assumes you have the following project layout (with this file in ./main)  #
#               {root}      /main/src                                         #
#                           /main/CMake (for Cmake modules)                   #
#                           /build      {project is generated here}           #
#   @author Ben Cardoen                                                       #
###############################################################################


### PROJECT NAME ##############################################################
project(eindwerk)
###############################################################################


### DEPENDENCIES ##############################################################
# Use CMAKE to discover listed dependencies. If found, these set the
# respective include/library paths referenced later.
# find_package(BOOST COMPONENTS {your parts here})
###############################################################################


### COMPILER ##################################################################
# Change/set compiler flags here.
# Specify which compiler (explicit) to use in ./generate_build.sh.

### Debug build (slows by a factor of x4 for STL), but catches any [>=size] 
### and iterator past the end mistakes.

# Clang compatible
#SET(CMAKE_CXX_FLAGS "-march=native -g3 -O0 -std=c++11 -fsanitize=integer -Wall -Wextra -pedantic -fstack-protector -fsanitize=undefined -ftrapv -D_GLIBCXX_DEBUG  ")

# G++ compatible
SET(CMAKE_CXX_FLAGS "-march=native -ggdb -O0 -std=c++1y -Wall -Wextra -pedantic -fstack-protector-strong -D_GLIBCXX_DEBUG")

### Release build - All compilers
# Only use this for production code
# Fast code, if it crashes you can still debug without overhead.
# SET(CMAKE_CXX_FLAGS "-march=native -Og")

# Experimental - only if your code cannot crash ever.
# SET(CMAKE_CXX_FLAGS "-march=native -O3")

# Throw all standard compliance and math(IEEE754, assoc...)  out of the window for that extra 1e-12s.
# SET(CMAKE_CXX_FLAGS "-march=mative -Ofast")

###############################################################################


### Print everything CMake does to cout to verify the options are sane.
set(CMAKE_VERBOSE_MAKEFILE ON)

### SOURCES ###################################################################
# List headers only if they are never included by a TU (for example template
# implementation code in header instead of .tpp)
add_executable(eindwerk
    src/main.cpp
    )

###############################################################################

### INCLUDE PATHS #############################################################
#include_directories(${BOOST_INCLUDE_DIRS})             # Boost
include_directories(src)   # Project headers are in same folder as source

### LINKER ####################################################################
# Register libraries needed.
#set(LIBS ${LIBS} ${GSL_LIBRARIES} ${GSLCBLAS_LIBRARIES})

# Specify linker options
#SET( MY_LINK_FLAGS  "-lpthread")
#SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${MY_LINK_FLAGS}" )

# Link the required libraries.
# target_link_libraries( scicomp3 ${LIBS})
###############################################################################

### Register Cotire ###########################################################
### This speeds up recompilation if STL or other large (template impl) headers
# are used (Boost, ...)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")
include(cotire)
cotire(eindwerk) 
