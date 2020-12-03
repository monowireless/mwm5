/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

 /** @file
  *
  * @defgroup TWESETTINGS_STD_DEFSETS ヘッダファイル
  * 
  * 
  * 
  */

#ifndef TWESETTINGS_STD_DEFSETS_H_
#define TWESETTINGS_STD_DEFSETS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	E_TWESTG_DEFSETS_APPID = 1,
	E_TWESTG_DEFSETS_LOGICALID,
	E_TWESTG_DEFSETS_CHANNEL,
	E_TWESTG_DEFSETS_CHANNELS_3,
	E_TWESTG_DEFSETS_POWER_N_RETRY,
	E_TWESTG_DEFSETS_OPTBITS,
	E_TWESTG_DEFSETS_UARTBAUD,
	
	E_TWESTG_DEFSETS_OPT_DWORD1,
	E_TWESTG_DEFSETS_OPT_DWORD2,
	E_TWESTG_DEFSETS_OPT_DWORD3,

	E_TWESTG_DEFSETS_VOID = 0xFF,
} teTWESTG_STD_DEFSETS;

extern const uint8 TWESTG_DEFCUST_U8_SLOT_NUMBER; //! スロットの最大数 [0..TWESTG_DEFCUST_U8_SLOT_NUMBER]

extern const TWESTG_tsElement TWESTG_DEFSETS_BASE[];
extern const TWESTG_tsElement TWESTG_DEFSETS_SLOT[][2];
extern const uint8 TWESTG_DEFCUST_SLOT[][4];
extern const uint8 TWESTG_DEFCUST_REMOVE_CHAN3[];
extern const uint8 TWESTG_DEFCUST_REMOVE_CHAN1[];

#ifdef __cplusplus
}
#endif


#endif