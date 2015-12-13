# README #

### Requirements ###

* CMake >= 2.8.8
* [Boost](http://www.boost.org/) >= 1.57
* g++ >= 4.8 , clang++ >= 3.4

And for tests:

* [GTest](https://github.com/google/googletest/releases ) >= 1.7 (1.6 may work)

### Optional ###
* Python 3.4 for the benchmark script.
* R for the statistical analysis of the benchmarks
* Perf (+kernel support) for the profiling/benchmark analysis.
* libubsan (clang or g++ should provide this) for leak/race/undefined detection
* tcmalloc for a significant speedup (CMake will try to find it, if not found not an error).

### Building & running ###

* **$ git clone git@bitbucket.org:bcardoen/devs-ex-machina.git**
* **$ cd devs-ex-machina**
* **$ ./setup.sh OPTIONS**

To find all options of the setup script: **$ ./setup.sh -h**

* **$ cd ./build/Release/**
* **$ make TARGET OPTIONS**

A list of targets:

* dxexmachina
* dxexmachina_devstone
* dxexmachina_interconnect
* dxexmachina_phold
* dxexmachina_priority

Note that at this point, only Clang and g++ are supported (with stringname : g++ , clang++)

The script makes a new directory build, next to directory main. This build directory will contain directories Debug, Release and Benchmark. Each with different compiler options and macros.

Optionally, you can open the project in Eclipse (provided you have the CDT plugin):

* File > Import > General > Existing Projects into Workspace
* Select the clone directory (containing main, build)
* CMake has preconfigured all compile/link settings for you.

### Benchmarking ###
If you wish to run different benchmarks and save the performance results in .csv files: 

**$ ./doBenchmark.py**

To find all the options of the doBenchmark script: 

**$ ./doBenchmark.py -h**

### ThreadSanitizer ###
LLVM's sanitizer framework requires position indepent code.
* If you have clang and wish to run threadsanitizer (default if build type is Debug and compiler = clang++), you'll need to (re)compile Gtest:

Some Linux distributions (Red Hat based) already ship libraries compiled with fpie, if not the case:

* In the {gtest_root_folder}/cmake/internal_utils.cmake, line 75(v1.7) || line 65(v1.6): <br/>
_set(cxx_base_flags "-Wall -Wshadow **-fpie**")_

### Windows ###
Building on Windows is possible using [Cygwin 64-bit](https://cygwin.com/install.html ).

The Cygwin installer will give you the opportunity to install, with the exception of gtest, all dependencies. From inside the Cygwin shell you can follow the build instructions above.

Note that the compiler will issue warnings about -fPic having no effect, this is a harmless side-effect.

### Troubleshooting ###
* CMake fails to find dependencies : <br/> if Boost or GTest are not located where cmakes looks for them, make a local file in the root project directory named _localpreferences.txt_ where you set 
{BOOST||GTEST}_ROOT={my_path}
* After any change to the CMakeLists.txt, remove your build directory to prevent stale CMake configuration files corrupting the build. The setup.sh script is capable of doing this for you.

