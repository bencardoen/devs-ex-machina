# README #

### Requirements ###

* CMake
* Boost (heap, system ; header only)
* G++ (4.8.2|4.9), Cygwin, Clang(3.4|3.5)
* GTest 1.7

### Building & running ###

* **$ git clone git@bitbucket.org:bcardoen/bachelor-project.git**
* **$cd bachelor-project**

The default build type is Debug, with G++ as compiler:

* **$ main/generate_build.sh**
* To alter this behaviour : **$main/generate_build.sh Release COMPILER**
* Note that at this point, only Mingw, Clang and g++ are supported (with stringname : g++ , mingw, clang++)

The script makes a new directory build, next to directory main. It then instruments CMake to actually build the project, producing an executable in build/ .

Optionally, you can open the project in Eclipse (provided you have the CDT plugin):

* File > Import > General > Existing Projects into Workspace
* Select the clone directory (containing main, build)
* CMake has preconfigured all compile/link settings for you.

### ThreadSanitizer ###
* If you have clang and wish to run threadsanitizer (default if build type is Debug and compiler = clang++), you'll need to (re)compile Gtest:

* In the gtest sources :
In file internal_utils.cmake change line 65 to append the compile flag -fpie :

set(cxx_base_flags "-Wall -Wshadow** -fpie**")