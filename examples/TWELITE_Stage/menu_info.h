/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#ifndef TWEINTERACTIVE_STAGE_INFO_H_
#define TWEINTERACTIVE_STAGE_INFO_H_

#include "twesettings/twecommon.h"
#include "twesettings/tweinteractive.h"
#include "twesettings/tweinputstring.h"

#ifdef __cplusplus
extern "C" {
#endif

void TWEINTCT_vSerUpdateScreen_stage_info(TWEINTRCT_tsContext *psIntr);
void TWEINTCT_vProcessInputByte_stage_info(TWEINTRCT_tsContext *psIntr, int16 i16Byte);
void TWEINTCT_vProcessInputString_stage_info(TWEINTRCT_tsContext *psIntr, TWEINPSTR_tsInpStr_Context *pContext);
TWE_APIRET TWEINTCT_u32ProcessMenuEvent_stage_info(TWEINTRCT_tsContext *pContext, uint32 u32Op, uint32 u32Arg1, uint32 u32Arg2, void *vpArg);


#ifdef __cplusplus
}
#endif

#endif