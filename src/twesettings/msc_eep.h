#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

// MSCデバッグ目的：TWENET用EEPROM APIのダミー

#include "twecommon.h"

// just simulate eeprom on JN51XX.
#define EEPROM_6X_SEGMENT_SIZE  (64)
#define EEPROM_6X_USER_SEGMENTS (60)

 /* Values derived from configurable parameters */
#define EEPROM_6X_USER_SIZE      (EEPROM_6X_SEGMENT_SIZE * EEPROM_6X_USER_SEGMENTS)

bool_t EEP_6x_bRead(uint16  u16StartAddr, uint16 u16Bytes, uint8 *pu8Buffer);
bool_t EEP_6x_bWrite(uint16 u16StartAddr, uint16 u16Bytes, uint8 *pu8Buffer);

#endif