#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include "twecommon.h"

#ifdef __cplusplus
extern "C" {
#endif

	extern volatile uint32 u32TickCount_ms;

#ifdef __cplusplus
}
#endif

#endif // _MSC_VER