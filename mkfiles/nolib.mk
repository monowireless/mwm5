#/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
# * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

##########################################################################
# Get the file path/dir of THIS makefile
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))

root_dir = $(abspath $(mkfile_dir)/..)

##########################################################################
# USE MWX library
MWM5=1

# makefile's debug message.
DEBUGINFO?=0

# target type (suffix) is set in arch_???.mk
TARGET_TYPE ?= bin

##########################################################################
# SET PROJNAME and TARGET_DIR
# assume Wks_MWX/TARGET_DIR/build dir structure.
PROJNAME=
DIRUP_LST = $(subst /, ,$(abspath ..))
TARGET_DIR = $(word $(words $(DIRUP_LST)), $(DIRUP_LST))

##########################################################################
# check arch
include $(mkfile_dir)/arch.mk

##########################################################################
# display debug info
ifeq ($(DEBUGINFO),1)
$(info !root_dir=$(root_dir))
$(info !mkfile_path=$(mkfile_path))
$(info !mkfile_dir=$(mkfile_dir))
$(info !DIRUP_LST=$(DIRUP_LST))
$(info !TARGET_DIR=$(TARGET_DIR))
$(info !OSNAME=$(OSNAME))
$(info !CC=$(CC))
$(info !CXX=$(CXX))
$(info !AR=$(AR))
endif

##########################################################################
# add project source
APPSRC_CXX += $(subst ../,,$(wildcard ../*.cpp))
_APPSRC_CXX = $(sort $(APPSRC_CXX))
APPSRC_CXX := $(_APPSRC_CXX)
APPSRC += $(subst ../,,$(wildcard ../*.c))
_APPSRC = $(sort $(APPSRC))
APPSRC := $(_APPSRC)

##########################################################################
# add default source

##########################################################################
# dirs
APP_SRC_DIR=$(abspath ..)
#APP_MWM5_SRC_DIR=$(root_dir)/src

#INCFLAGS += -I$(APP_MWM5_SRC_DIR)/gen
#INCFLAGS += -I$(APP_MWM5_SRC_DIR)/twesettings

#CFLAGS += -DMWM5_BUILD_$(shell echo $(OSNAME) | tr '[:lower:]' '[:upper:]')

#OBJDIR_SUB += esp32 font twesettings gen oss printf $(OSNAME)
##########################################################################
# LOAD others
include $(mkfile_dir)/rules.mk
#########################################################################
