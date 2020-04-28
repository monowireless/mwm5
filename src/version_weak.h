#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER)
#define __TWE_WEAK_VAR_ATTR 
#elif defined(__MINGW32__)
#define __TWE_WEAK_VAR_ATTR __declspec(weak)
#else // assume GCC
#define __TWE_WEAK_VAR_ATTR  __attribute__((weak))
#endif

// #include "version.h"
extern const unsigned char __TWE_WEAK_VAR_ATTR MWM5_APP_VERSION_MAIN;
extern const unsigned char __TWE_WEAK_VAR_ATTR MWM5_APP_VERSION_SUB;
extern const unsigned char __TWE_WEAK_VAR_ATTR MWM5_APP_VERSION_VAR;
extern const char* const __TWE_WEAK_VAR_ATTR MWM5_APP_NAME;
