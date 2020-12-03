#/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
# * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */
#####################################################################

MACOS_TARGET?=X86_64_GCC
#MACOS_TARGET=X86_64_CLANG
#MACOS_TARGET=ARM64

### NOTES FOR UNIVERSAL APP
# To combine x86_64 app and arm64 app
#   lipo -create -output TWELITE_Stage.command TWELITE_Stage.x86_64 TWELITE_Stage.x86_app.arm64
# To identify binary nature
#   lipo -archs TWELITE_Stage.command
#   objdump -a TWELITE_Stage.command


### defines 
ifeq ($(MACOS_TARGET),X86_64_GCC)
MACOS_COMPILERTYPE=gcc-9
MACOS_ARCH=x86_64
MWM5_SERIAL_NO_FTDI=0
$(info ...build for x86_64 system with gcc-9 which may work on older macOS)
else ifeq ($(MACOS_TARGET),X86_64_CLANG)
MACOS_COMPILERTYPE=clang
MACOS_ARCH=x86_64
MWM5_SERIAL_NO_FTDI=0
$(info ...build for x86_64 system with clang for Catalina or later.)
else ifeq ($(MACOS_TARGET),ARM64)
MACOS_COMPILERTYPE=clang
MACOS_ARCH=arm64
MWM5_SERIAL_NO_FTDI=1
$(info ...build for arm64 system with clang for BigSur or later.)
endif

### SERIAL
# use Dummy Serial Driver (do nothing)
#MWM5_SERIAL_DUMMY ?= 1

### Dynamic link (deprecated)
# if 0, most SDL2/FTDI library is linked statically.
DYNAMIC_LINK = 0

### Post config process
ifeq ($(MACOS_COMPILERTYPE),clang)
# FOR macOS clang (note: macOS 10.15 catalina required for C++17)
CC = clang
CXX = clang++
AR = ar
else ifeq ($(MACOS_COMPILERTYPE),gcc-9)
# FOR macOS build, use GCC from homebrew package (fomula=gcc@9)
CC = gcc-9
CXX = g++-9
AR = gcc-ar-9
else
$(error "MACOS_COMPILERTYPE=$(MACOS_COMPILERTYPE) is not supported.")
endif

CFLAGS += -arch $(MACOS_ARCH)
LDFLAGS += -arch $(MACOS_ARCH)

ifeq ($(MWM5_SERIAL_NO_FTDI),1)
CFLAGS += -DMWM5_SERIAL_NO_FTDI
endif

ifeq ($(MWM5_SERIAL_DUMMY),1)
CFLAGS += -DMWM5_SERIAL_DUMMY
endif

#####################################################################
# ADDITIONAL OS SPECIFIC SOURCE
APPSRC_CXX+=$(OSNAME)/osx_term.cpp

#####################################################################
# ADDITIONAL INCLUDE PATH
INCFLAGS += -I$(root_dir)/osx/lib/FTDI
INCFLAGS += -I$(root_dir)/osx/lib/SDL2/include/SDL2
INCFLAGS += -I$(root_dir)/osx/lib/SDL2_net/include
INCFLAGS += -I$(root_dir)/src/$(OSNAME)

#####################################################################
# LIBRARY

# SDL2
LDFLAGS += -L$(root_dir)/osx/lib/SDL2/lib/
CFLAGS += -D_REENTRANT

# FTDI D2XX
ifeq ($(MWM5_SERIAL_NO_FTDI),0)
LDFLAGS += -L$(root_dir)/osx/lib/FTDI/
endif

ifeq ($(DYNAMIC_LINK),1)
  # dynamic link build
  ADDITIONAL_LIBS += -lftd2xx -lpthread -lobjc -framework IOKit -framework CoreFoundation
  ADDITIONAL_LIBS += -lSDL2 -lSDL2_net
else
  # static build
  ADDITIONAL_LIBS += -lcurses

  ifeq ($(MACOS_COMPILERTYPE),clang)
  LDFLAGS += -stdlib=libc++ 
  ADDITIONAL_LIBS += -lc++
  else
  ADDITIONAL_LIBS += -static-libstdc++ -static-libgcc
  endif

  ifeq ($(MWM5_SERIAL_NO_FTDI),0)
  ADDITIONAL_LIBS += $(root_dir)/osx/lib/FTDI/libftd2xx.a -lpthread -lobjc -framework IOKit -framework CoreFoundation
  endif
  ADDITIONAL_LIBS += $(root_dir)/osx/lib/SDL2/lib/libSDL2.a -lm -liconv -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-framework,ForceFeedback -lobjc -Wl,-framework,CoreVideo -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,IOKit -Wl,-weak_framework,QuartzCore -Wl,-weak_framework,Metal
  # ADDITIONAL_LIBS += $(root_dir)/osx/lib/SDL2_net/lib/libSDL2_net.a
endif


#####################################################################
# Build type - suffix
TARGET_TYPE=command
