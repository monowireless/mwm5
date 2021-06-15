#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twesettings/tweset.h"

#define MWM5_DEFAULT_BAUD 115200

#ifdef __cplusplus
	extern "C" {
#endif // C++

typedef enum {
	E_TWESTG_STAGE_START_APP = 1,
#ifndef ESP32
	E_TWESTG_STAGE_SCREEN_MODE,
	E_TWESTG_STAGE_FTDI_ADDR,
#else
	E_TWESTG_STAGE_KEYBOARD_LAYOUT,
#endif
	E_TWESTG_STAGE_FG_COLOR,
	E_TWESTG_STAGE_BG_COLOR,
	E_TWESTG_STAGE_BAUD_TERM,
	// APP WRT
	E_TWESTG_STAGE_APPWRT_START = 0x30,
	E_TWESTG_STAGE_APPWRT_BUILD_NEXT_SCREEN,
#ifndef ESP32
	E_TWESTG_STAGE_APPWRT_OPEN_CODE,
	E_TWESTG_STAGE_APPWRT_BUILD_MAKE_JOGS,
	E_TWESTG_STAGE_APPWRT_FORCE_DISABLE_LTO, // force DISALBE_LTO=1
#endif
	// INTRCT
#ifdef ESP32
	E_TWESTG_STAGE_INTRCT_START = 0x40,
	E_TWESTG_STAGE_INTRCT_USE_SETPIN,
#endif
	E_TWESTG_STAGE_VOID = 0xFF,
} teTWESTG_STAGE;

extern const TWESTG_tsElement TWESTG_STAGE_BASE[];
extern const TWESTG_tsSettingsListItem SetList[];
extern const TWEINTRCT_tsFuncs asFuncs[];
extern const TWESTG_tsElement TWESTG_SLOT_SCREEN_BUILD[];
extern const TWESTG_tsElement TWESTG_SLOT_SCREEN_INTRCT[];
extern uint8 au8CustomDefault_Unuse[];
#ifdef __cplusplus
}
#endif // C++
