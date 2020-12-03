#!/bin/bash

### build both intel/arm64 on macOS with clang, then make them universal.

### make arm64 build
make clean
make -j8 MACOS_TARGET=ARM64
mv TWELITE_Stage.command TWELITE_Stage.arm64

### make x86_64 build
#   CLANG -> works on macOS 10.15 or 11, require macOS10.15 or later to compile.
#   GCC   -> works older macOS, require GCC9.3 to compile.
make clean
make -j8 MACOS_TARGET=X86_64_CLANG
#make -j8 MACOS_TARGET=X86_64_GCC
mv TWELITE_Stage.command TWELITE_Stage.x86_64

### make universal binary
lipo -create -output TWELITE_Stage.command TWELITE_Stage.x86_64 TWELITE_Stage.arm64

### remove objs dir
make clean
