#/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
# * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */
##########################################################################
# check OS
ifeq ($(OS),Windows_NT)
 OSNAME=win
else
 UNAME_S := $(shell uname -s)
 ifeq ($(UNAME_S),Darwin)
  OSNAME=mac
 endif
 ifeq ($(UNAME_S),Linux)
  OSNAME=linux
 endif
endif

include $(mkfile_dir)/arch_$(OSNAME).mk