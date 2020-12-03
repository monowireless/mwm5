##############################################################################
#/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
# * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */
##############################################################################
# COMMON BUILD RULES
##############################################################################

##############################################################################
# VERSION
NAME_DEBUG_SUFF=debug
NAME_OBJDIR=objs

##############################################################################
# VERSION
VERSION_MAIN ?= 0
VERSION_SUB ?= 1
VERSION_VAR ?= 0

CFLAGS += -DVERSION_MAIN=$(VERSION_MAIN)
CFLAGS += -DVERSION_SUB=$(VERSION_SUB)
CFLAGS += -DVERSION_VAR=$(VERSION_VAR)

##############################################################################
# Target

ifeq ($(subst $(space),,$(TARGET_TYPE)),a)
  TARGET = lib$(TARGET_DIR)
  $(warning "NOTICE: building library file.")
else
  TARGET = $(TARGET_DIR)
endif

ifeq ($(DEBUG_BUILD),1)
TARGET_BIN = $(TARGET)-$(NAME_DEBUG_SUFF)
else
#TARGET_BIN = $(TARGET)_$(VERSION_MAIN)-$(VERSION_SUB)-$(VERSION_VAR)
TARGET_BIN = $(TARGET)
endif
CFLAGS += -DSTR_MWM5_APP_NAME=\"$(TARGET_DIR)\"

##############################################################################
# Objects
ifneq ($(OBJDIR_SUB),)
_OBJDIR_SUB=$(OBJDIR_SUB:%=$(OBJDIR)/%)
endif

ifeq ($(DEBUG_BUILD),1)
OBJDIR   = $(NAME_OBJDIR)-$(NAME_DEBUG_SUFF)
else
OBJDIR   = $(NAME_OBJDIR)
endif

##############################################################################
# Objects

#APPOBJS_BASE = $(APPSRC:.c=.o)
APPOBJS = $(APPSRC:%.c=$(OBJDIR)/%.o)

#APPOBJS_CXX_BASE = $(APPSRC_CXX:.cpp=.o)
APPOBJS_CXX = $(APPSRC_CXX:%.cpp=$(OBJDIR)/%.o)


##############################################################################
# Application include path
ifneq ($(APP_SRC_DIR),)
INCFLAGS += -I$(APP_SRC_DIR)
endif
ifneq ($(APP_MWM5_SRC_DIR),)
INCFLAGS += -I$(APP_MWM5_SRC_DIR)
endif
ifneq ($(ADDITIONAL_SRC_DIR),)
INCFLAGS += -I$(ADDITIONAL_SRC_DIR)
endif
ifneq ($(APP_COMMON_SRC_DIR_ADD1),)
INCFLAGS += -I$(APP_COMMON_SRC_DIR_ADD1)
endif
ifneq ($(APP_COMMON_SRC_DIR_ADD2),)
INCFLAGS += -I$(APP_COMMON_SRC_DIR_ADD2)
endif
ifneq ($(APP_COMMON_SRC_DIR_ADD3),)
INCFLAGS += -I$(APP_COMMON_SRC_DIR_ADD3)
endif
ifneq ($(APP_COMMON_SRC_DIR_ADD4),)
INCFLAGS += -I$(APP_COMMON_SRC_DIR_ADD4)
endif
 
##############################################################################
# Application dynamic dependencies
USE_APPDEPS?=1
ifeq ($(USE_APPDEPS),1)
  APPDEPS = $(APPOBJS:.o=.d) $(APPOBJS_CXX:.o=.d)
  DEPFLAGS = -MT $@ -MMD -MP -MF $(OBJDIR)/$*.d
else
endif

#########################################################################
# C++ compiler
ifneq ($(subst $(space),,$(APPSRC_CXX)),)
$(info !!!g++ is used for compiling .cpp and linking)
BUILD_CXX=1
endif

# C++17
CXXSTD = -std=c++17

# C11
CSTD = -std=c11

# OPT FLAGS
ifeq ($(DEBUG_BUILD),1)
CFLAGS += -g -D_DEBUG
else
CFLAGS += -O2
endif

#########################################################################
# Linker
# LDLIBS := $(LDLIBS)

# C/C++ Linker (if .cpp files are in the build target, use g++ for linking)
ifeq ($(BUILD_CXX),1)
LINKERCMD=$(CXX)
else
LINKERCMD=$(CC)
endif

ifeq ($(DEBUGINFO),1)
$(info APP_SRC_DIR $(APP_SRC_DIR))
$(info APP_COMMON_SRC_DIR $(APP_COMMON_SRC_DIR))
$(info APP_STACK_SRC_DIR_ADD1 $(APP_STACK_SRC_DIR_ADD1))
$(info APP_STACK_SRC_DIR_ADD2 $(APP_STACK_SRC_DIR_ADD2))
$(info APP_COMMON_SRC_DIR_ADD1 $(APP_COMMON_SRC_DIR_ADD1))
$(info APP_MWX_SRC_DIR $(APP_MWX_SRC_DIR))
$(info TARGET_BIN $(TARGET_BIN))
$(info APPOBJS $(APPOBJS))
$(info APPOBJS_CXX $(APPOBJS_CXX))
$(info INCFLAGS $(INCFLAGS))
endif

#########################################################################
# Main Section
.PHONY: all clean objdir cleanall

# Path to directories containing application source 
vpath % $(APP_SRC_DIR):$(ADDITIONAL_SRC_DIR):$(APP_COMMON_SRC_DIR_ADD1):$(APP_COMMON_SRC_DIR_ADD2):$(APP_COMMON_SRC_DIR_ADD3):$(APP_COMMON_SRC_DIR_ADD4):$(APP_MWM5_SRC_DIR)

all: $(TARGET_BIN).$(TARGET_TYPE) 

# objdir should make before compiling (for parallel build.)
$(APPOBJS): | objdir
$(APPOBJS_CXX): | objdir

# include appdep
$(APPDEPS):
include $(wildcard $(APPDEPS))

# build rules
ifeq ($(USE_APPDEPS),1)
$(OBJDIR)/%.o: %.c $(OBJDIR)/%.d
else
$(OBJDIR)/%.o: %.c
endif
	$(info Compiling $< ...)
	$(CC) $(CSTD) -c -o $(subst Source,Build,$@) $(DEPFLAGS) $(CFLAGS) $(INCFLAGS) $(realpath $<)
	@echo

ifeq ($(USE_APPDEPS),1)
$(OBJDIR)/%.o: %.cpp $(OBJDIR)/%.d
else
$(OBJDIR)/%.o: %.cpp
endif
	$(info Compiling $< ...)	
	$(CXX) $(CXXSTD) -c -o $(subst Source,Build,$@) $(DEPFLAGS) $(CXXFLAGS) $(CFLAGS) $(INCFLAGS) $(realpath $<)
	@echo

# generate elf file
$(OBJDIR)/$(TARGET_BIN): $(APPOBJS) $(APPOBJS_CXX) 
	$(info Linking $@ ...)
	$(LINKERCMD) -o $@ $(LDFLAGS) \
		 $(APPOBJS) $(APPOBJS_CXX) $(ADDITIONAL_OBJS) $(ADDITIONAL_LIBS) 

# generate bin file
$(TARGET_BIN).$(TARGET_TYPE): $(OBJDIR)/$(TARGET_BIN)
	@mv -fv $< $@
	@chmod +x $@
	
# generate .a file
$(TARGET_BIN).a: $(APPOBJS) $(APPOBJS_CXX)
	@rm -f $(TARGET_BIN).a
	$(AR) $(ARFLAGS) $@ $(APPOBJS) $(APPOBJS_CXX) $(ADDITIONAL_OBJS)


#########################################################################
# create sub-dirs for obj files.
objdir:
	@mkdir -p $(OBJDIR) $(_OBJDIR_SUB)

# note: clean/cleanall will keep a target `.bin' file.
clean:
	@rm -rfv $(NAME_OBJDIR) $(NAME_OBJDIR)-$(NAME_DEBUG_SUFF)

#########################################################################
