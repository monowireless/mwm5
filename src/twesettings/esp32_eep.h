#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twecommon.h"

#if defined(ESP32)

// just simulate eeprom on JN51XX.
#define EEPROM_6X_SEGMENT_SIZE  (64)
#define EEPROM_6X_USER_SEGMENTS (60)

 /* Values derived from configurable parameters */
#define EEPROM_6X_USER_SIZE      (EEPROM_6X_SEGMENT_SIZE * EEPROM_6X_USER_SEGMENTS)

#ifdef __cplusplus
extern "C" {
#endif

void EEP_6x_vInit();
bool_t EEP_6x_bRead(uint16  u16StartAddr, uint16 u16Bytes, uint8* pu8Buffer);
bool_t EEP_6x_bWrite(uint16 u16StartAddr, uint16 u16Bytes, uint8* pu8Buffer);

#ifdef __cplusplus
}
#endif

#endif //  ESP32