
#/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
# * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */
#####################################################################
# FOR macOS build, use GCC from homebrew package (fomula=gcc@9)
CC = gcc-9
CXX = g++-9
AR = gcc-ar-9

#####################################################################
# ADDITIONAL OS SPECIFIC SOURCE
APPSRC_CXX+=$(OSNAME)/linux_term.cpp

#####################################################################
# ADDITIONAL INCLUDE PATH
INCFLAGS += -I$(root_dir)/linux/lib/FTDI
INCFLAGS += -I$(root_dir)/linux/lib/SDL2/include
INCFLAGS += -I$(root_dir)/src/$(OSNAME)

#####################################################################
# LIBRARY

# SDL2
LDFLAGS += -L$(root_dir)/linux/lib/SDL2/lib/
CFLAGS += -D_REENTRANT

# FTDI D2XX
LDFLAGS += -L$(root_dir)/linux/lib/FTDI/

ifeq ($(DEBUG_BUILD),1)
# build normally
ADDITIONAL_LIBS += -lSDL2
ADDITIONAL_LIBS += -lftd2xx
else
# static build
CFLAGS += -DUSE_CURSES
ADDITIONAL_LIBS += -static-libstdc++ -static-libgcc
ADDITIONAL_LIBS += -lcurses
ADDITIONAL_LIBS += $(root_dir)/linux/lib/FTDI/libftd2xx.a

#ADDITIONAL_LIBS += $(root_dir)/linux/lib/SDL2/lib/libSDL2.a -Wl,--no-undefined -lm -ldl -lasound -lm -ldl -lpthread -lpulse-simple -lpulse -lsndio -lX11 -lXext -lXcursor -lXinerama -lXi -lXrandr -lXss -lXxf86vm -lwayland-egl -lwayland-client -lwayland-cursor -lxkbcommon
ADDITIONAL_LIBS += $(root_dir)/linux/lib/SDL2/lib/libSDL2.a -Wl,--no-undefined -lm -ldl -lasound -lm -ldl -lpthread -lpulse-simple -lpulse -lX11 -lXext -lXcursor -lXinerama -lXi -lXrandr -lXss -lXxf86vm -lwayland-egl -lwayland-client -lwayland-cursor -lxkbcommon
ADDITIONAL_LIBS += $(root_dir)/linux/lib/sndio/libsndio.a
endif

# common for SDL2/FTDI
ADDITIONAL_LIBS +=  -lpthread -lrt

#####################################################################
# Build type - suffix
TARGET_TYPE=run