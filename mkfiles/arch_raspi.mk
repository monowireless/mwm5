
#/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
# * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */
#####################################################################
# gcc/g++
# note: require g++-8 or above for C++17 feature.

CC = gcc
CXX = g++
#CC = clang-9
#CXX = clang++-9

AR = gcc-ar

DO_DYNAMIC_LINK=0

# no use of dependency
USE_APPDEPS?=0

#####################################################################
# ADDITIONAL OS SPECIFIC SOURCE
APPSRC_CXX+=linux/linux_term.cpp
OBJDIR_SUB+=linux

#####################################################################
# ADDITIONAL INCLUDE PATH
INCFLAGS += -I$(root_dir)/raspi/lib/FTDI
INCFLAGS += -I$(root_dir)/raspi/lib/SDL2/include
INCFLAGS += -I$(root_dir)/src/$(OSNAME)

INCFLAGS += -I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux 

#####################################################################
# LIBRARY

CFLAGS += -DMWM5_SERIAL_DUO

# SDL2
LDFLAGS += -L$(root_dir)/raspi/lib/SDL2/lib/fb/
CFLAGS += -D_REENTRANT

# FTDI D2XX
LDFLAGS += -L$(root_dir)/raspi/lib/FTDI/
LDFLAGS += -L/opt/vc/lib

ifeq ($(DO_DYNAMIC_LINK),1)
# build normally
ADDITIONAL_LIBS += -lSDL2
ADDITIONAL_LIBS += -lftd2xx
else
# static build
ADDITIONAL_LIBS += -static-libstdc++ -static-libgcc -lstdc++fs
ADDITIONAL_LIBS += $(root_dir)/raspi/lib/FTDI/libftd2xx.a

ADDITIONAL_LIBS += $(root_dir)/raspi/lib/SDL2/lib/fb/libSDL2.a -Wl,--enable-new-dtags -lSDL2 -Wl,--no-undefined -lm -ldl -Wl,-rpath,/opt/vc/lib -lbcm_host
endif

# common for SDL2/FTDI
ADDITIONAL_LIBS +=  -lpthread -lrt

#####################################################################
# Build type - suffix
TARGET_TYPE=run
