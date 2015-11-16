# README #

### Requirements ###

* CMake (> 2.8.8)
* Boost (heap, system ; header only)
* G++ (4.8.2|4.9), Clang (3.4|3.5)

And for tests:

* GTest 1.7

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

Note that at this point, only Clang and g++ are supported (with stringname : g++ , clang++)

The script makes a new directory build, next to directory main. This build directory will contain directories Debug, Release and Benchmark. Each with different compiler options and macros.

Optionally, you can open the project in Eclipse (provided you have the CDT plugin):

* File > Import > General > Existing Projects into Workspace
* Select the clone directory (containing main, build)
* CMake has preconfigured all compile/link settings for you.

### Benchmarking ###
If you wish to run different benchmarks and save the performance results in .csv files: **$ ./doBenchmark.py**

To find all the options of the doBenchmark script: **$ ./doBenchmark.py -h**

### ThreadSanitizer ###
* If you have clang and wish to run threadsanitizer (default if build type is Debug and compiler = clang++), you'll need to (re)compile Gtest:

* In the gtest sources :
In file internal_utils.cmake change line 65 to append the compile flag -fpie :
_set(cxx_base_flags "-Wall -Wshadow** -fpie**")_