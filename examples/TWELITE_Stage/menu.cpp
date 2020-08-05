/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "mwm5.h"
#include "menu_defs.h"
#include "menu.hpp"

#include "AppIDs.h"

/*!
 * 確定設定リスト(tsFinal)から各設定を読み出す。
 * ※ コード可読性の観点からイテレータ(TWESTG_ITER_tsFinal_*)を用いた読み出しを行う。
 */
void MyTweMenu::vQueryAppData() {
	// 設定のクエリ
	TWESTG_ITER_tsFinal sp;
    
	TWESTG_ITER_tsFinal_BEGIN(sp, &stg.sFinal); // init iterator
	if (!TWESTG_ITER_tsFinal_IS_VALID(sp)) return; //ERROR DATA

	while (!TWESTG_ITER_tsFinal_IS_END(sp)) { // end condition of iter
		switch (TWESTG_ITER_tsFinal_G_ID(sp)) { // get data as UINT32
		case E_TWESTG_STAGE_START_APP:
			sAppData.u8_TWESTG_STAGE_START_APP = TWESTG_ITER_tsFinal_G_U8(sp); break;
		case E_TWESTG_STAGE_FG_COLOR:
			sAppData.u32_TWESTG_STAGE_FG_COLOR = TWESTG_ITER_tsFinal_G_U32(sp); break;
		case E_TWESTG_STAGE_BG_COLOR:
			sAppData.u32_TWESTG_STAGE_BG_COLOR = TWESTG_ITER_tsFinal_G_U32(sp); break;
		case E_TWESTG_STAGE_APPWRT_BUILD_NEXT_SCREEN:
			sAppData.u8_TWESTG_STAGE_APPWRT_BUILD_NEXT_SCREEN = TWESTG_ITER_tsFinal_G_U8(sp); break;
		case E_TWESTG_STAGE_APPWRT_OPEN_CODE:
			sAppData.u8_TWESTG_STAGE_OPEN_CODE = TWESTG_ITER_tsFinal_G_U8(sp); break;
#ifndef ESP32
		case E_TWESTG_STAGE_SCREEN_MODE:
			sAppData.u8_TWESTG_STAGE_SCREEN_MODE = TWESTG_ITER_tsFinal_G_U8(sp); break;
		case E_TWESTG_STAGE_FTDI_ADDR:
			if(TWESTG_ITER_tsFinal_G_PU8(sp) != NULL) {
				memcpy(sAppData.au8_TWESTG_STAGE_FTDI_ADDR
					, TWESTG_ITER_tsFinal_G_PU8(sp)
					, sizeof(sAppData.au8_TWESTG_STAGE_FTDI_ADDR));
			}
			break;
		case E_TWESTG_STAGE_APPWRT_BUILD_MAKE_JOGS:
			sAppData.u8_TWESTG_STAGE_APPWRT_BUILD_MAKE_JOGS = TWESTG_ITER_tsFinal_G_U8(sp); break;
#else
		case E_TWESTG_STAGE_KEYBOARD_LAYOUT:
			sAppData.u8_TWESTG_STAGE_KEYBOARD_LAYOUT = TWESTG_ITER_tsFinal_G_U8(sp); break;
		case E_TWESTG_STAGE_INTRCT_USE_SETPIN:
			sAppData.u8_TWESTG_STAGE_INTRCT_USE_SETPIN = TWESTG_ITER_tsFinal_G_U8(sp); break;
#endif
		}
		TWESTG_ITER_tsFinal_INCR(sp); // incrment
	}
}

const char str_appnames[int(E_APP_ID::_END_)][STR_APPNAMES_STRLEN] = {
	"ビューア",
	"ターミナル",
	"標準アプリ ビューア",
	"ＰＡＬ ビューア",
	"グランサー(簡易モニタ)",
	"", // _APP_END_
	"アプリ書換",
	"インタラクティブモード",
	"", // _UTILS_END_
	"TWELITE® STAGEの設定",
#ifndef ESP32
	"シリアルポート選択",
#endif

};

/**
 * @fn	const char* MyTweMenu::get_slot_name(uint8_t slot)
 *
 * @brief	Gets slot name
 *
 * @param	slot	The slot.
 *
 * @returns	Null if it fails, else the slot name.
 */
const char* MyTweMenu::get_slot_name(uint8_t slot) {
	if (slot == 0) return "共通設定";
	else if (slot < int(E_APP_ID::_APPS_END_)) return str_appnames[slot];
	else if (slot < int(E_APP_ID::_UTILS_END_) - 1) return str_appnames[slot + 1];
	else return nullptr;
}

// generate setlist internally
void MyTweMenu::gen_SetList() {
	int i_ct_slots = (int)E_APP_ID::_UTILS_END_ - 1;
	_spSetList.reset(new TWESTG_tsSettingsListItem[i_ct_slots+ 1]); // slot + terminator

	// common settings
	_spSetList[0] = { 0, TWESTG_SLOT_DEFAULT, // common settings
		{ TWESTG_STAGE_BASE, NULL, NULL, NULL, NULL, NULL } };

	// for app slot
	for (uint8_t i = 1; i < (uint8_t)E_APP_ID::_APPS_END_; i++) {
		uint8_t i_slot = i;
		switch(E_APP_ID(i)) {
		case E_APP_ID::ROOT_MENU: // dummy label
		case E_APP_ID::_APPS_END_: // dummy label
		default:
			_spSetList[i_slot] =  { 0, i_slot, // slot#n (for app#1)
				{ TWESTG_STAGE_BASE, NULL, NULL,
				  NULL, au8CustomDefault_Unuse, NULL }};
			break;
		}
	}

	// for other utils
	for (uint8_t i = (uint8_t)E_APP_ID::_APPS_END_ + 1; i < (int)E_APP_ID::_UTILS_END_; i++) {
		uint8_t i_slot = i - 1;

		switch(E_APP_ID(i)) {
		case E_APP_ID::FIRM_PROG:
			_spSetList[i_slot] =  { 0, i_slot, // slot#n (for app#1)
				{ TWESTG_STAGE_BASE, NULL, TWESTG_SLOT_SCREEN_BUILD,
				  NULL, au8CustomDefault_Unuse, NULL }};
			break;
		case E_APP_ID::INTERACTIVE:
			_spSetList[i_slot] =  { 0, i_slot, // slot#n (for app#1)
				{ TWESTG_STAGE_BASE, NULL, TWESTG_SLOT_SCREEN_INTRCT,
				  NULL, au8CustomDefault_Unuse, NULL }};
			break;
		default:
			_spSetList[i_slot] =  { 0, i_slot, // slot#n (for app#1)
				{ TWESTG_STAGE_BASE, NULL, NULL,
				  NULL, au8CustomDefault_Unuse, NULL }};
			break;
		}
	}

	// TERMINATE
	_spSetList[i_ct_slots] = { TWESTG_KIND_VOID, TWESTD_SLOT_VOID, { NULL }}; // TERMINATE
}

/**
 * @fn	int appid_to_slotid(int appid)
 *
 * @brief	Appid to slotid
 *
 * @param	appid	The appid.
 *
 * @returns	An int.
 */
int appid_to_slotid(int appid) {
	if (appid > 0 && appid < (int)E_APP_ID::_APPS_END_) {
		return appid;
	}
	else if (appid < (int)E_APP_ID::_UTILS_END_) {
		return appid - 1;
	}
	else {
		return 0;
	}
}

static MyTweMenu my_menu;
TWE::TweStgsMenu& TWE::the_settings_menu = my_menu;

struct _sAppData sAppData;