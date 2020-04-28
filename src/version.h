#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

// these defs are provided as CFLAGS, just for coding assist.
extern const unsigned char MWM5_APP_VERSION_MAIN;
extern const unsigned char MWM5_APP_VERSION_SUB;
extern const unsigned char MWM5_APP_VERSION_VAR;
extern const char* const MWM5_APP_NAME;

#ifndef _MSC_VER
// for GCC
#define MWM5_DECL_APP_VERSION_MAIN(c) const unsigned char MWM5_APP_VERSION_MAIN = c;
#define MWM5_DECL_APP_VERSION_SUB(c) const unsigned char MWM5_APP_VERSION_SUB = c;
#define MWM5_DECL_APP_VERSION_VAR(c) const unsigned char MWM5_APP_VERSION_VAR = c;
#define MWM5_DECL_APP_NAME(c) const char* const MWM5_APP_NAME = c;
#else
// for VC++, selectany is using for weak defs.
#define MWM5_DECL_APP_VERSION_MAIN(c) const __declspec(selectany) unsigned char MWM5_APP_VERSION_MAIN = c;
#define MWM5_DECL_APP_VERSION_SUB(c) const __declspec(selectany) unsigned char MWM5_APP_VERSION_SUB = c;
#define MWM5_DECL_APP_VERSION_VAR(c) const __declspec(selectany) unsigned char MWM5_APP_VERSION_VAR = c;
#define MWM5_DECL_APP_NAME(c) const __declspec(selectany) char* const MWM5_APP_NAME = c;
#endif
