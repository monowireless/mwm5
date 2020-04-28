#pragma once

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

	TWE_APIRET TWEINTRCT_cbu32GenericHandler(TWEINTRCT_tsContext* pContext, uint32 u32Op, uint32 u32Arg1, uint32 u32Arg2, void* vpArg);
	TWE_APIRET TWESTG_cbu32SaveSetting(TWE_tsBuffer* pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal* psFinal);
	TWE_APIRET TWESTG_cbu32LoadSetting(TWE_tsBuffer* pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal* psFinal);

#ifdef __cplusplus
}
#endif
