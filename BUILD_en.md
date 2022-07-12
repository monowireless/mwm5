# mwm5 - Building notes

Mono Wireless TWELITE STAGE Application and C++ library for Windows/Linux/macOS/(M5Stack)



---

Building EPS32 (M5) with Version 1.3 or later is not considered in the source code.

---



## Required environment for build

Depending on the environment, packages other than those listed may need to be installed.



### Windows10,11

* We are using Visual Studio 2019 (VC++) and have project files under the msc directory.
* Make the project under the Sketch filter (e.g. TWELITE_Stage) the startup project and select Release/Debug, 64bit/32bit to build.


Only VC++ has 32-bit build definitions.



### macOS

You can build with macOS 10.14 or later, but as a general rule, we target builds with the most recent macOS release.

* Please install gcc (gcc-11, g++-11 for this version) with brew.
  Build definitions by clang are also available, but some C++ libraries are only supported on macOS 15 (Catalina) or later, so they will not build or work on earlier versions of macOS.
* Make sure you have access to the make included with XCode.
* Run make on the examples/????? Run make on the /build directory.

```
make 
  MACOS_TARGET=X86_64_GCC -> for Homebrew gcc-11,g++-11
  MACOS_TARGET=ARM64	  -> for clang++ of Apple Silicon
  DEBUG_BUILD=1           -> debug option
```

See [osx/readme.txt](osx/readme.txt) as well.


### Linux

Can be built on Ubuntu 16.04 18.04 20.04.

* gcc (gcc-11, g++-11 for this version) must be installed.
* Install SDL2 (libsdl2-dev package).
* Run make on the examples/? Run make on the /build directory.

```
make オプション
  DEBUG_BUILD=1           -> debug build
```



see [linux/readme.txt](linux/readme.txt) as well.





## Attached Libraries

The following is attached library source code distributed by external projects to simplify the build process and match the environment.

* SDL2 sources and libraries
* FTDI D2XX driver
* sqlite3, SQLiteCpp



### macOS

* Compiler for the library
    * Intel version build with gcc-9, g++-9 or gcc-11, g++-11 installed from homebrew
    * Apple Silicon version is built with clang


### Linux

* sndio (because it depends on SDL2 but cannot be statically linked)

