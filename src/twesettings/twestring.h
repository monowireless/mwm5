/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/** @file
 *
 * @defgroup TWESER ヘッダファイル
 *
 * シリアル等の入出力の抽象定義を行う。
 */

#ifndef TWESTRING_H_
#define TWESTRING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "twecommon.h"

uint8 TWESTR_u8SplitTokens(uint8 *pstr, uint8 u8max_len, uint8 **auptr, uint8 u8max_entry);
uint32 TWESTR_u32HexstrToNum(uint8 *p, uint8 u8len);
int32 TWESTR_i32DecstrToNum(uint8 *p, uint8 u8len);


#ifdef __cplusplus
}
#endif


#endif
