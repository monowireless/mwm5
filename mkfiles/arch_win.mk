#/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
# * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */
#####################################################################
# FOR macOS build, use GCC from homebrew package (fomula=gcc@9)
CC = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
AR = x86_64-w64-mingw32-gcc-ar

#####################################################################
# ADDITIONAL OS SPECIFIC SOURCE
APPSRC_CXX+=$(OSNAME)/msc_term.cpp

#####################################################################
# ADDITIONAL INCLUDE PATH
INCFLAGS += -I$(root_dir)/msc/lib/FTDI
INCFLAGS += -I$(root_dir)/msc/lib/SDL2/include
INCFLAGS += -I$(root_dir)/src/$(OSNAME)

#####################################################################

# STATIC BUILD for GCC RELATED 
LDFLAGS += -static -lstdc++ -lgcc -lwinpthread
LDFLAGS += -Wl,-Bstatic

# SDL2
#LDFLAGS += -L$(root_dir)/msc/lib/SDL2/lib/x64
CFLAGS += -D_REENTRANT
ADDITIONAL_LIBS += $(root_dir)/msc/lib/SDL2/lib/x64/SDL2.lib 

# FTDI D2XX
#LDFLAGS += -L$(root_dir)/msc/lib/FTDI/amd64
ADDITIONAL_LIBS += $(root_dir)/msc/lib/FTDI/amd64/ftd2xx.lib

# common for SDL2/FTDI
ADDITIONAL_LIBS += -lwinmm
#####################################################################
# Build type - suffix
TARGET_TYPE=exe