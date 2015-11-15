# README #

### Requirements ###

* CMake
* Boost (heap, pools, system )
* g++ >= 4.8 , clang++ >= 3.4
* GTest >= 1.7

### Optional ###
* Python 2.7; 3.4 for the profiling/benchmark script respectively.
* R for the statistical analysis of the benchmarks
* Perf (+kernel support) for the profiling/benchmark analysis.
* libubsan (clang or g++ should provide this) for leak/race/undefined detection
* tcmalloc for a significant speedup (CMake will try to find it, if not found not an error).

For reference the project is developed using g++ 5.1/2 and clang 3.5.

### Building & running ###

* **$ git clone git@bitbucket.org:bcardoen/devs-ex-machina.git**
* **$cd devs-ex-machina**
* **$ ./setup.sh -h**

Will give the full instructions to generate all build targets. The script allows you to choose your preferred compiler as well.

Example: generate dnd build debug (tests) in folder build/Debug

**$./setup.sh -c g++ -d**

Optionally, you can open the project in Eclipse (provided you have the CDT plugin):

* File > Import > General > Existing Projects into Workspace
* Select the clone directory (containing main, build)
* CMake has preconfigured all compile/link settings for you.

### ThreadSanitizer ###
LLVM's sanitizer framework requires position indepent code.
* If you have clang and wish to run threadsanitizer (default if build type is Debug and compiler = clang++), you'll need to (re)compile Gtest:

Some Linux distributions (Red Hat based) already ship libraries compiled with fpie, if not the case:
* In the gtest sources :
In file internal_utils.cmake change line 65 to append the compile flag -fpie :

set(cxx_base_flags "-Wall -Wshadow** -fpie**")
