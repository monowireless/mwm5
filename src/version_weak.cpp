/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "version_weak.h"

/* IMPLEMENTATION PART */
#ifndef _MSC_VER
// SET DUMMY
#ifndef VERSION_MAIN
# define VERSION_MAIN 0
#endif
#ifndef VERSION_SUB
# define VERSION_SUB 1
#endif
#ifndef VERSION_VAR
# define VERSION_VAR 0
#endif
#ifndef STR_MWM5_APP_NAME
# define STR_MWM5_APP_NAME "MWM5"
#endif

extern const unsigned char __TWE_WEAK_VAR_ATTR MWM5_APP_VERSION_MAIN = VERSION_MAIN;
extern const unsigned char __TWE_WEAK_VAR_ATTR MWM5_APP_VERSION_SUB = VERSION_SUB;
extern const unsigned char __TWE_WEAK_VAR_ATTR MWM5_APP_VERSION_VAR = VERSION_VAR;
extern const char* const __TWE_WEAK_VAR_ATTR MWM5_APP_NAME = STR_MWM5_APP_NAME;
#endif
