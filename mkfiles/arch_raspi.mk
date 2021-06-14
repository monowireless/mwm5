#/* Copyright (C) 2019-2021 Mono Wireless Inc. All Rights Reserved.
# * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */
#####################################################################
# gcc/g++
# note: require g++-8 or above for C++17 feature.
CC = gcc
CXX = g++
AR = gcc-ar

## Dynamic or Static
# note: proper library installation is required for dynamic liking.
DO_DYNAMIC_LINK?=0

## SDL2 library for Raspberry PI
# fb: frame buffer (console), X11: for X Window System (desktop)
RPI_SDL2_LIB?=X11

## ARMv6 support (e.g. FTDI library)
RPI_ARMv6?=0

## disable fade/transparent effect for slower platform.
ifeq ($(RPI_ARMv6),1)
CFLAGS += -DMWM5_ENABLE_FADE_EFFECT=0
endif

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

# more includes for RasPi (video console)
INCFLAGS += -I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux 

#####################################################################
# LIBRARY
CFLAGS += -DMWM5_SERIAL_DUO

# SDL2
LDFLAGS += -L$(root_dir)/raspi/lib/SDL2/lib/$(RPI_SDL2_LIB)/
CFLAGS += -D_REENTRANT

# FTDI D2XX
LDFLAGS += -L$(root_dir)/raspi/lib/FTDI/
LDFLAGS += -L/opt/vc/lib

ifeq ($(DO_DYNAMIC_LINK),1)
##### build with dynamic link library #####
ADDITIONAL_LIBS += -lSDL2
ADDITIONAL_LIBS += -lftd2xx
else
##### build with static link library #####
ADDITIONAL_LIBS += -static-libstdc++ -static-libgcc -lstdc++fs

# FTDI library
ifeq ($(RPI_ARMv6),0)
ADDITIONAL_LIBS += $(root_dir)/raspi/lib/FTDI/libftd2xx.a
else
ADDITIONAL_LIBS += $(root_dir)/raspi/lib/FTDI/libftd2xx_v6hf.a
endif

ADDITIONAL_LIBS += $(root_dir)/raspi/lib/SDL2/lib/$(RPI_SDL2_LIB)/libSDL2.a -Wl,--enable-new-dtags -lSDL2 -Wl,--no-undefined -lm -ldl -Wl,-rpath,/opt/vc/lib -lbcm_host
endif

##### common library #####
ADDITIONAL_LIBS +=  -lpthread -lrt

#####################################################################
# Build type - suffix
TARGET_TYPE=run
