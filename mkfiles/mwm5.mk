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
APPSRC+=twesettings/twesettings_weak.c

APPSRC_CXX+=twe_firmprog.cpp
APPSRC_CXX+=twe_fmt.cpp
APPSRC_CXX+=twe_sercmd_binary.cpp
APPSRC_CXX+=twe_cui_listview.cpp
APPSRC_CXX+=twe_printf.cpp
APPSRC_CXX+=twe_appdefs.cpp
APPSRC_CXX+=twe_sercmd_ascii.cpp
APPSRC_CXX+=twe_stream.cpp
APPSRC_CXX+=twe_csettings.cpp
APPSRC_CXX+=twe_console.cpp
APPSRC_CXX+=twe_utils_crc8.cpp
APPSRC_CXX+=twe_sercmd.cpp
APPSRC_CXX+=twe_sys.cpp
APPSRC_CXX+=twe_font.cpp
APPSRC_CXX+=twe_stgsmenu.cpp
APPSRC_CXX+=twe_file.cpp

APPSRC_CXX+=esp32/esp32_lcd_font.cpp
APPSRC_CXX+=esp32/esp32_common.cpp
APPSRC_CXX+=esp32/esp32_modctrl.cpp
APPSRC_CXX+=esp32/esp32_lcdconsole.cpp
APPSRC_CXX+=esp32/esp32_keyb.cpp

APPSRC_CXX+=font/lcd_font_shinonome14.cpp
APPSRC_CXX+=font/lcd_font_shinonome16.cpp
APPSRC_CXX+=font/lcd_font_MP10.cpp
APPSRC_CXX+=font/lcd_font_shinonome12.cpp
APPSRC_CXX+=font/lcd_font_MP12.cpp
APPSRC_CXX+=font/lcd_font_8x6.cpp

APPSRC_CXX+=twesettings/esp32_eep.cpp
APPSRC+=twesettings/twecrc8.c
APPSRC+=twesettings/twenvm.c
APPSRC+=twesettings/msc_eep.c
APPSRC+=twesettings/msc_main.c
APPSRC+=twesettings/twesettings_std.c
APPSRC+=twesettings/tweinteractive_settings.c
APPSRC+=twesettings/twesercmd_chat.c
APPSRC+=twesettings/twesettings_cmd.c
APPSRC+=twesettings/twesercmd_timeout.c
APPSRC+=twesettings/tweinteractive_nvmutils.c
APPSRC+=twesettings/msc_sys.c
APPSRC+=twesettings/tweinputstring.c
APPSRC+=twesettings/twesettings.c
APPSRC+=twesettings/twesettings_validator.c
APPSRC+=twesettings/twestring.c
APPSRC+=twesettings/twesercmd_plus3.c
APPSRC+=twesettings/twesercmd_binary.c
APPSRC+=twesettings/tweinteractive_defmenus.c
APPSRC+=twesettings/tweprintf.c
APPSRC+=twesettings/twesercmd_ascii.c
APPSRC+=twesettings/twesysutils.c
APPSRC+=twesettings/tweserial.c
APPSRC+=twesettings/tweserial_jen.c
APPSRC+=twesettings/tweinteractive.c
APPSRC+=twesettings/twesettings_std_defsets.c

APPSRC_CXX+=gen/sdl2_main.cpp
APPSRC_CXX+=gen/sdl2_keyb.cpp
APPSRC_CXX+=gen/sdl2_clipboard.cpp
APPSRC_CXX+=gen/sdl2_button.cpp
APPSRC_CXX+=gen/sdl2_icon.cpp
APPSRC_CXX+=gen/serial_ftdi.cpp
APPSRC_CXX+=gen/modctrl_ftdi.cpp

# thanks to open source contributions!
APPSRC+=printf/printf.c
APPSRC+=oss/oss_getopt.c
APPSRC_CXX+=oss/oss_regex.cpp

# weak vars
APPSRC_CXX+=version_weak.cpp

##########################################################################
# dirs
APP_SRC_DIR=$(abspath ..)
APP_MWM5_SRC_DIR=$(root_dir)/src

INCFLAGS += -I$(APP_MWM5_SRC_DIR)/gen
INCFLAGS += -I$(APP_MWM5_SRC_DIR)/twesettings

OBJDIR_SUB = esp32 font twesettings gen oss printf $(OSNAME)
##########################################################################
# LOAD others
include $(mkfile_dir)/rules.mk
#########################################################################
