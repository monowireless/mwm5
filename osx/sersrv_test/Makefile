#####################################################################
#/* Copyright (C) 2020-2020 Mono Wireless Inc. All Rights Reserved.
# * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE
#   LICENSE AGREEMENT). */
#####################################################################

APPNAME = sersrv_test

###### SOURCES ######
APPSRC_CXX += main.cpp
APPSRC_CXX += twe_file.cpp
APPSRC_CXX += twe_sys.cpp
APPSRC_CXX += serial_common.cpp
APPSRC_CXX += serial_srv_pipe.cpp
APPSRC_CXX += twe_sercmd.cpp
APPSRC_CXX += twe_sercmd_ascii.cpp
APPSRC_CXX += twe_firmprog.cpp
APPSRC_CXX += twe_utils_crc8.cpp

#APPSRC_HPP += twe_file.hpp

###### SRC PATH ######
INCLUDES += -I../../src -I../../src/gen
PATH_LIBSRC = ../../src:../../src/gen

###### MACROS ######
#DEFINES += -DMY_DEFS

###### COMMON DEFS ######
OSNAME = mac
CXX = clang++ -std=c++17 -arch arm64
CFLAGS += -O1
APPNAME_EXE = $(APPNAME).command

###### LIBRARY DEFS ######
INCLUDES += -I../lib/FTDI
#ADDITIONAL_LIBS += ../lib/FTDI/libftd2xx.a -lpthread -lobjc -framework IOKit -framework CoreFoundation

###### RULES ######
OBJDIR=objs
APPOBJS_CXX = $(APPSRC_CXX:%.cpp=$(OBJDIR)/%.o)
vpath % $(PATH_LIBSRC):.

all: objs $(APPNAME_EXE)

$(OBJDIR)/%.o: %.cpp $(APPSRC_HPP)
	$(CXX) -c -o $@ $(CFLAGS) $(DEFINES) $(INCLUDES) $< 

$(APPNAME_EXE): $(APPOBJS_CXX)
	$(CXX) -o $@  $(CFLAGS) $(APPOBJS_CXX) $(ADDITIONAL_LIBS)

clean:
	rm -f $(APPNAME_EXE) $(APPOBJS_CXX)

objs:
	mkdir -p objs
