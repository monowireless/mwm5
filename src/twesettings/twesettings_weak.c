/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <string.h>
#include "twecommon.h"
#include "twesettings_weak.h"

/*************************************************************************
 * DUMMY FUNCTION (weak linking)
 *************************************************************************/

#ifdef _MSC_VER
#define F(a) weak_##a
#else
#define F(a) a
#endif

TWE_APIRET F(TWEINTRCT_cbu32GenericHandler)(TWEINTRCT_tsContext* pContext, uint32 u32Op, uint32 u32Arg1, uint32 u32Arg2, void* vpArg) { return 0; }
TWE_APIRET F(TWESTG_cbu32SaveSetting)(TWE_tsBuffer* pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal* psFinal) { return 0; }
TWE_APIRET F(TWESTG_cbu32LoadSetting)(TWE_tsBuffer* pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal* psFinal) { return 0; }