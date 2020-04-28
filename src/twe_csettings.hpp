#pragma once 

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#ifndef USE_CSETTINGS
#define USE_CSETTINGS
#endif

#ifdef USE_CSETTINGS
#include "twe_common.hpp"
#include "twe_stream.hpp"

#include "twesettings/twecommon.h"
#include "twesettings/tweserial.h"
#include "twesettings/tweprintf.h"

/// <summary>
/// Bridge functions to TWE_fprintf(),... functions in C based library.
/// - provide put/get stream functions for TWE_tsFILE struct
///   using IStreamOut/IStreamIn classes.
/// </summary>
class C_TWE_printf_support {
public:
	static void s_init(struct _tsTWETERM_FILE* fp, TWE::IStreamOut* pStrm, TWE::IStreamIn* pInpt);
	static int s_getc(struct _tsTWETERM_FILE* fp);
	static int s_putc(int c, struct _tsTWETERM_FILE* fp);
	static int s_puts(const char* s, struct _tsTWETERM_FILE* fp);
	static void s_flush(struct _tsTWETERM_FILE* fp);
};

#endif