#/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
# * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */
#####################################################################

# compiler selection (set clang or gcc-9)
#OSX_COMPILERTYPE ?= clang
OSX_COMPILERTYPE ?= gcc-9

# if 0, most SDL2/FTDI library is linked statically.
DYNAMIC_LINK = 0

ifeq ($(OSX_COMPILERTYPE),clang)
# FOR macOS clang (note: macOS 10.15 catalina required for C++17)
CC = gcc
CXX = g++
AR = ar
else ifeq ($(OSX_COMPILERTYPE),gcc-9)
# FOR macOS build, use GCC from homebrew package (fomula=gcc@9)
CC = gcc-9
CXX = g++-9
AR = gcc-ar-9
else
$(error "OSX_COMPILERTYPE=$(OSX_COMPILERTYPE) is not supported.")
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
LDFLAGS += -L$(root_dir)/osx/lib/FTDI/

ifneq ($(DEBUG_BUILD),1)
#CFLAGS += -DUSE_CURSES
endif

ifeq ($(DYNAMIC_LINK),1)
  # dynamic link build
  ADDITIONAL_LIBS += -lftd2xx -lpthread -lobjc -framework IOKit -framework CoreFoundation
  ADDITIONAL_LIBS += -lSDL2 -lSDL2_net
else
  # static build
  ADDITIONAL_LIBS += -lcurses

  ifeq ($(OSX_COMPILERTYPE),clang)
  LDFLAGS += -stdlib=libc++ 
  ADDITIONAL_LIBS += -lc++
  else
  ADDITIONAL_LIBS += -static-libstdc++ -static-libgcc
  endif

  ADDITIONAL_LIBS += $(root_dir)/osx/lib/FTDI/libftd2xx.a -lpthread -lobjc -framework IOKit -framework CoreFoundation
  ADDITIONAL_LIBS += $(root_dir)/osx/lib/SDL2/lib/libSDL2.a -lm -liconv -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-framework,ForceFeedback -lobjc -Wl,-framework,CoreVideo -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,IOKit -Wl,-weak_framework,QuartzCore -Wl,-weak_framework,Metal
  ADDITIONAL_LIBS += $(root_dir)/osx/lib/SDL2_net/lib/libSDL2_net.a
endif


#####################################################################
# Build type - suffix
TARGET_TYPE=command
