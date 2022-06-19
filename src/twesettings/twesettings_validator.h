/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

 /** @file
  *
  * @defgroup TWESTGS ヘッダファイル
  *
  */

#ifndef TWESETTINGS_VLD_H_
#define TWESETTINGS_VLD_H_

#include "twecommon.h"
#include "twesettings0.h"

#ifdef __cplusplus
extern "C" {
#endif


#define TWESTGS_VLD_OP_VALIDATE 0x01
#define TWESTGS_VLD_OP_CUSTDISP 0x02

#define TWESTGS_VLD_MAX_CUSTOMSTR_BUFF 32 //! 終端文字列を含む

TWE_APIRET TWESTGS_VLD_u32MinMax(struct _TWESTG_sElement* pMe, TWESTG_tsDatum* psDatum, uint16 u16OpId, TWE_tsBuffer *pBuf);
TWE_APIRET TWESTGS_VLD_u32AppId(struct _TWESTG_sElement* pMe, TWESTG_tsDatum* psDatum, uint16 u16OpId, TWE_tsBuffer* pBuf);
TWE_APIRET TWESTGS_VLD_u32ChList(struct _TWESTG_sElement* pMe, TWESTG_tsDatum* psDatum, uint16 u16OpId, TWE_tsBuffer* pBuf);
TWE_APIRET TWESTGS_VLD_u32UartOpt(struct _TWESTG_sElement* pMe, TWESTG_tsDatum* psDatum, uint16 u16OpId, TWE_tsBuffer* pBuf);
TWE_APIRET TWESTGS_VLD_u32UartBaudOpt(struct _TWESTG_sElement* pMe, TWESTG_tsDatum* psDatum, uint16 u16OpId, TWE_tsBuffer* pBuf);
TWE_APIRET TWESTGS_VLD_u32String(struct _TWESTG_sElement* pMe, TWESTG_tsDatum* psDatum, uint16 u16OpId, TWE_tsBuffer* pBuf);

#ifdef __cplusplus
}
#endif



#endif