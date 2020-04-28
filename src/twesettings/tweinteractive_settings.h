/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#ifndef TWEINTERACTIVE_SETTINGS_H_
#define TWEINTERACTIVE_SETTINGS_H_

#include "twecommon.h"
#include "tweinteractive.h"
#include "tweinputstring.h"

#ifdef __cplusplus
extern "C" {
#endif

void TWEINTCT_vSerUpdateScreen_settings(TWEINTRCT_tsContext *psIntr);
void TWEINTCT_vProcessInputByte_settings(TWEINTRCT_tsContext *psIntr, int16 i16Byte);
void TWEINTCT_vProcessInputString_settings(TWEINTRCT_tsContext *psIntr, TWEINPSTR_tsInpStr_Context *pContext);
TWE_APIRET TWEINTCT_u32ProcessMenuEvent_settings(TWEINTRCT_tsContext *pContext, uint32 u32Op, uint32 u32Arg1, uint32 u32Arg2, void *vpArg);

#ifdef __cplusplus
}
#endif

#endif