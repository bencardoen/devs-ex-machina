# README #

### About ###

This is the main repository for the Devs Ex Machina ('dxex') simulator, a [PythonPDEVS](http://msdl.cs.mcgill.ca/projects/DEVS/PythonPDEVS ) inspired C++ PDEVS simulation engine supporting conservative and optimistic synchronization. Models written for dxex can be written agnostic of the runtime synchronization protocol.
A quick overview of features:

* High performance (P)DEVS simulation engine (see publications folder for results)

* Extensible Tracing module, output to Json, XML, ... . Custom tracers supported. (compile time optional)

* High performance logging framework (compile time optional)

* Visualization and internal tracing of a simulation (compile time optional)

* Implemented in standard C++ (C++11, C++14 optional) for maximum portability.



For a more in depth review of the design and features we refer you the to publications. 

### Publications ###

If you want to cite this project, kindly use the following bibtex

```
#!tex
@inproceedings{Dxex,
    author                  = {Cardoen, Ben and
                               Manhaeve, Stijn and
                               Tuijn, Tim and
                               Van Tendeloo, Yentl and
                               Vanmechelen, Kurt and
                               Vangheluwe, Hans and
                               Broeckhove, Jan},
    title                   = {Performance Analysis of a {PDEVS} Simulator Supporting Multiple Synchronization Protocols},
    booktitle               = {Proceedings of the 2016 Symposium on Theory of Modeling and Simulation - DEVS},
    series                  = {TMS/DEVS '16, part of the Spring Simulation Multi-Conference},
    month                   = APR,
    pages                   = {614 -- 621},
    year                    = {2016},
    location                = {Pasadena, CA, USA},
    publisher               = {Society for Computer Simulation International},
}

```


### License ###

This project is licensed under the EUPL, see COPYING.txt
Copyright 2014-2016 University of Antwerp

### Requirements ###

* CMake >= 2.8.8
* [Boost](http://www.boost.org/) >= 1.57
* g++ >= 4.8 , clang++ >= 3.4 (versions of GCC up to 6.1 are supported)
* [GTest](https://github.com/google/googletest/releases ) >= 1.7 (1.6 may work)
* *Nix environment (Linux, Cygwin, MacOS). The project is developed on Fedora Rawhide, Ubuntu 14 LTS and Linux Mint 17, these are guaranteed to work.

Optional:

* Python 3.4 for the benchmark script.
* R for the statistical analysis of the benchmarks
* Perf (+kernel support) for the profiling/benchmark analysis.
* libubsan (clang or g++ should provide this) for leak/race/undefined detection
* Valgrind for memory profiling
* tcmalloc for a significant speedup (CMake will try to find it, if not found not an error).
* adevs for comparison

### Building & running ###

* **$ git clone git@bitbucket.org:bcardoen/devs-ex-machina.git**
* **$ cd devs-ex-machina**
* **$ ./setup.sh OPTIONS**

To find all options of the setup script: 

* **$ ./setup.sh -h**
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

#### IDE Support ####

Eclipse 

Optionally, you can open the project in Eclipse (provided you have the CDT plugin):

* File > Import > General > Existing Projects into Workspace
* Select the clone directory (containing main, build)
* CMake has preconfigured all compile/link settings for you.

NetBeans (version >= 7.0 with C++ support)

* File > New Project > C++ Project with existing sources
* Select the devsexmachina/build/{Debug|Release|Benchmark|BenchmarkFrng} as root folder
* The remainder of configuration can be left at default, NetBeans will use the make files generated by CMake
* Optionally, select 'custom' for configuration mode and add "-j {nrcores*2}" in "Build Actions after "Build Command"
* NetBeans will build the project and parse the source code, depending on which build you selected (default = dxexmachina kernel) you will have full IDE support for the project (Code completion, refactoring etc... .) 

### Benchmarking ###
All benchmark data used in the paper can be found in 

* Rscript/data
* paper/fig/final_libreoffice_plots.ods

First, to be able to build our adevs models the source of adevs is required.
Create a file in {project_root}/main named "localpreferences.txt" where you can inform CMake of adevs' path :   
`set(ADEVS_ROOT mypath)`

If you wish to run different benchmarks and save the performance results in .csv files: 

**$ ./doBenchmark.py**

To find all the options of the doBenchmark script: 

**$ ./doBenchmark.py -h**

The memory benchmarks (only if you have at least 8GB RAM) requires a POSIX shell, and valgrind. Each benchmark that is profiled for memory can take up to a factor 100 longer to execute under valgrind, please take this into account. The actual memory usage changes little over time (except with optimistic), so you can manually adjust the runtime in the script should this be required.

**$ ./membench.sh**


### ThreadSanitizer ###
LLVM's sanitizer framework requires position indepent code.
* If you have clang and wish to run threadsanitizer (default if build type is Debug and compiler = clang++), you'll need to (re)compile Gtest:

Some Linux distributions (Red Hat based) already ship libraries compiled with fpie, if not the case:

* In the {gtest_root_folder}/cmake/internal_utils.cmake, line 75(v1.7) || line 65(v1.6):
`_set(cxx_base_flags "-Wall -Wshadow **-fpie**")_`

### Windows ###
Building on Windows is possible using [Cygwin 64-bit](https://cygwin.com/install.html ).

The Cygwin installer will give you the opportunity to install, with the exception of gtest, all dependencies. From inside the Cygwin shell you can follow the build instructions above.

Note that the compiler will issue warnings about -fPic having no effect, this is a harmless side-effect.

### Troubleshooting ###
* CMake fails to find dependencies : 
if Boost or GTest are not located where cmake looks for them, make a local file in the root project directory named _localpreferences.txt_ where you put
`set({BOOST||GTEST}_ROOT{my_path})`

* After any change to the CMakeLists.txt, remove your build directory to prevent stale CMake configuration files corrupting the build. The setup.sh script is capable of doing this for you.