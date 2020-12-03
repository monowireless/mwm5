###### SOURCES ######
APPSRC_CXX += glancer.cpp

APPSRC_CXX += twe_fmt.cpp
APPSRC_CXX += twe_fmt_actstd.cpp
APPSRC_CXX += twe_fmt_appio.cpp
APPSRC_CXX += twe_fmt_apptag.cpp
APPSRC_CXX += twe_fmt_appuart.cpp
APPSRC_CXX += twe_fmt_common.cpp
APPSRC_CXX += twe_fmt_pal.cpp
APPSRC_CXX += twe_fmt_twelite.cpp
APPSRC_CXX += twe_sercmd.cpp
APPSRC_CXX += twe_sercmd_ascii.cpp
APPSRC_CXX += twe_utils_crc8.cpp

APPSRC_HPP += twe_common.hpp
APPSRC_HPP += twe_fmt.hpp
APPSRC_HPP += twe_fmt_actstd.hpp
APPSRC_HPP += twe_fmt_appio.hpp
APPSRC_HPP += twe_fmt_apptag.hpp
APPSRC_HPP += twe_fmt_appuart.hpp
APPSRC_HPP += twe_fmt_common.hpp
APPSRC_HPP += twe_fmt_pal.hpp
APPSRC_HPP += twe_fmt_twelite.hpp
APPSRC_HPP += twe_sercmd.hpp
APPSRC_HPP += twe_sercmd_ascii.hpp
APPSRC_HPP += twe_stream.hpp
APPSRC_HPP += twe_utils.hpp
APPSRC_HPP += twe_utils_crc8.hpp
APPSRC_HPP += twe_utils_fixedque.hpp
APPSRC_HPP += twe_utils_simplebuffer.hpp

###### SRC PATH ######
INCLUDES += -I../../src
PATH_LIBSRC = ../../src

###### MACROS ######
DEFINES += -DTWE_STDINOUT_ONLY
DEFINES += -DTWE_HAS_MILLIS

###### COMMON DEFS ######
# check OS
ifeq ($(OS),Windows_NT)
 OSNAME=win
 CXX=g++-9
else
 UNAME_S := $(shell uname -s)
 ifeq ($(UNAME_S),Darwin)
  OSNAME=mac
  CXX=g++-9
 endif
 ifeq ($(UNAME_S),Linux)
  OSNAME=linux
  CXX=g++
 endif
endif

CFLAGS += -g

###### RULES ######
OBJDIR=objs
APPOBJS_CXX = $(APPSRC_CXX:%.cpp=$(OBJDIR)/%.o)
vpath % $(PATH_LIBSRC):.

all: objs glancer

$(OBJDIR)/%.o: %.cpp $(APPSRC_HPP)
	$(CXX) -c -o $@ $(CFLAGS) $(DEFINES) $(INCLUDES) $< 

glancer: $(APPOBJS_CXX)
	$(CXX) -o $@  $(CFLAGS) $(APPOBJS_CXX)

clean: 
	rm -f glancer $(APPOBJS_CXX)

objs:
	mkdir -p objs
