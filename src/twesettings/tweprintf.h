#pragma once

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
#include <stdarg.h>

#include "twecommon.h"
#include "tweserial.h"
#include "../printf/printf.h"

#ifdef __cplusplus
extern "C" {
#endif

int TWE_fprintf(TWE_tsFILE *fp, const char *format, ...);
int TWE_vfprintf(TWE_tsFILE *fp, const char *format, va_list va);
#define TWE_snprintf(out,siz,fmt,...) snprintf_(out,siz,fmt,__VA_ARGS__)
// int TWE_snprintf(char *out, size_t siz, const char *format, ...);

#ifdef __cplusplus
}
#endif

