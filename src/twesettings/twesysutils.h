/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/** @file
 *
 * @defgroup TWESYSUTILS ヘッダファイル
 *
 */

#ifndef TWESYSUTILS_H_
#define TWESYSUTILS_H_

#include "twecommon.h"

#ifdef __cplusplus
extern "C" {
#endif

void TWESYSUTL_vWaitPoll(uint32 u32WaitCt);
void TWESYSUTL_vWaitPollMicro(uint32 u32WaitCt);

#ifdef __cplusplus
}
#endif

#endif // TWESYSUTILS_H_