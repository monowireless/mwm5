/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <string.h>

#include "twecommon.h"
#include "twesettings0.h"
#include "tweinteractive.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
// this should be included for application using twesettings library.
#pragma comment(linker, "/alternatename:_TWEINTRCT_cbu32GenericHandler=_weak_TWEINTRCT_cbu32GenericHandler")
#pragma comment(linker, "/alternatename:_TWESTG_cbu32SaveSetting=_weak_TWESTG_cbu32SaveSetting")
#pragma comment(linker, "/alternatename:_TWESTG_cbu32LoadSetting=_weak_TWESTG_cbu32LoadSetting")
#endif

#if defined(_MSC_VER)
	#define __TWE_WEAK_ATTR extern
#elif defined(__MINGW32__)
	#define __TWE_WEAK_ATTR __declspec(weak)
#else // assume GCC
	#define __TWE_WEAK_ATTR __attribute__((weak))
#endif

__TWE_WEAK_ATTR TWE_APIRET TWEINTRCT_cbu32GenericHandler(TWEINTRCT_tsContext* pContext, uint32 u32Op, uint32 u32Arg1, uint32 u32Arg2, void* vpArg);
__TWE_WEAK_ATTR TWE_APIRET TWESTG_cbu32SaveSetting(TWE_tsBuffer* pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal* psFinal);
__TWE_WEAK_ATTR TWE_APIRET TWESTG_cbu32LoadSetting(TWE_tsBuffer* pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal* psFinal);

#ifdef __cplusplus
}
#endif