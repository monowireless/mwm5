#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "AppIDs.h"
#include "twe_stgsmenu.hpp"
#include "twesettings/tweset.h"

// to store application data.
struct _sAppData {
	uint8_t u8_TWESTG_STAGE_START_APP;
	uint32_t u32_TWESTG_STAGE_FG_COLOR;
	uint32_t u32_TWESTG_STAGE_BG_COLOR;
	uint32_t u32_TWESTG_STAGE_BAUD_TERM;
#ifdef ESP32
	uint8_t u8_TWESTG_STAGE_KEYBOARD_LAYOUT;
	uint8_t u8_TWESTG_STAGE_INTRCT_USE_SETPIN;
#else
	uint8_t u8_TWESTG_STAGE_SCREEN_MODE;
	uint8_t au8_TWESTG_STAGE_FTDI_ADDR[8];
	uint8_t u8_TWESTG_STAGE_APPWRT_BUILD_MAKE_JOGS;
#endif
	uint8_t u8_TWESTG_STAGE_APPWRT_BUILD_NEXT_SCREEN;
	uint8_t u8_TWESTG_STAGE_OPEN_CODE; // if set, open dir with "code" command.
	uint8_t u8_TWESTG_STAGE_APPWRT_FORCE_DISABLE_LTO;
};

extern struct _sAppData sAppData;

class MyTweMenu : public TWE::TweStgsMenu {
	std::unique_ptr<TWESTG_tsSettingsListItem[]> _spSetList;

public:
	MyTweMenu() : TWE::TweStgsMenu(32, 64, 4), _spSetList(nullptr) {
		set_max_slotnumber((int)E_APP_ID::_UTILS_END_ - 2); // skip enum entry _APPS_END_
		gen_SetList();
	}
    void vQueryAppData() override;
	const char *get_slot_name(uint8_t slot) override;

	const TWESTG_tsSettingsListItem *get_setlist() override { 
		const TWESTG_tsSettingsListItem *p = &(_spSetList[0]);
		return p;
	}

private:
	void gen_SetList();
};