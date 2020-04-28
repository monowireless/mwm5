#/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
# * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#ifdef _MSC_VER
#include "version_msc.h"
#include "version_weak.h"

// if version is not set in Makefile, use dummy.
#ifndef VERSION_MAIN
#define VERSION_MAIN 0
#endif
#ifndef VERSION_SUB
#define VERSION_SUB 1
#endif
#ifndef VERSION_VAR
#define VERSION_VAR 0
#endif

// NOTE: weak definition using __declspec(selectany),
//       however it's determined linking order. 
//       (if compiling order is earlier, needs to modify VC++ project file.)
//       
const __declspec(selectany) unsigned char MWM5_APP_VERSION_MAIN = VERSION_MAIN;
const __declspec(selectany) unsigned char MWM5_APP_VERSION_SUB = VERSION_SUB;
const __declspec(selectany) unsigned char MWM5_APP_VERSION_VAR = VERSION_VAR;
const __declspec(selectany) char* const MWM5_APP_NAME = STR_MWM5_APP_NAME;
#endif