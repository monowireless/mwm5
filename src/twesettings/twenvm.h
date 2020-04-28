/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/** @file
 *
 * @defgroup EEPROM EEPROMメモリの読み書き関数群
 * EEPROM への読み書き関数
 */

#ifndef TWENVM_H_
#define TWENVM_H_

#if defined(JENNIC_CHIP_NAME)
#include <jendefs.h>
#include <eeprom_6x.h>
#include <jendefs.h>
#include <AppHardwareApi.h>
#elif defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)
#include "msc_eep.h"
#endif
#include "twecommon.h"

#ifdef __cplusplus
extern "C" {
#endif

bool_t TWENVM_bRead(TWE_tsBuffer *pBuff, uint8 u8sector);
bool_t TWENVM_bWrite(TWE_tsBuffer *pBuff, uint8 u8sector);
bool_t TWENVM_bErase(uint8 u8sector);
bool_t TWENVM_bReadSectorRaw(uint8 *u8buff, uint8 u8sector, uint8 u8len);


#ifdef __cplusplus
}
#endif


#endif /* TWENVM_H_ */
