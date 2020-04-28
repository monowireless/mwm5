/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

#ifdef _MSC_VER

#include "twecommon.h"
#include "tweutils.h"

#include "twesettings.h"
#include "twesettings_std.h"
#include "twesettings_validator.h"

#include "twesettings_std_defsets.h"
#include "tweinteractive_defmenus.h"
#include "tweinteractive_settings.h"
#include "tweinteractive_nvmutils.h"

#include "msc_defs.h"

 /*!
  * ボード設定データ例（独自の設定を追加している）
  */
const TWESTG_tsElement SetBrd[] = {
	{ 0x82,
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = 38400}},
		{ "UBA", "UART Baud [9600-230400]", "" },
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 6, 'b' },
		{ {.u32 = 0}, {.u32 = 115200}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ 0x81,
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = (1UL << 13 | 1UL << 18)}},
		{ "TE1", "test for ChList", "" },
		{ E_TWEINPUTSTRING_DATATYPE_CUSTOM_DISP_MASK | E_TWEINPUTSTRING_DATATYPE_STRING, 8, 'A' },
		{ {.u32 = 0}, {.u32 = 0xFFFFFFFF}, TWESTGS_VLD_u32ChList, NULL },
	},
	{E_TWESTG_DEFSETS_VOID} // FINAL DATA
};

/*!
 * カスタムデフォルト(BASE)
 *   APPIDのデフォルト値を書き換えている
 */
uint8 au8CustomDefault_Base[] = {
	6,   // 総バイト数
	E_TWESTG_DEFSETS_APPID, (TWESTG_DATATYPE_UINT32 << 4) | 4, 0xa1, 0xb2, 0x12, 0x34, // 6bytes
};

/*!
 * カスタムデフォルト(BOARD) - [シリアル化要素データ集合]
 *   E_TWESTG_DEFSETS_CHANNELS_3を未使用にしている
 */
uint8 au8CustomDefault_Brd_Chan3[] = {
	2,   // 総バイト数(このバイトは含まない。手計算で間違えないように入力！)
	E_TWESTG_DEFSETS_CHANNELS_3, TWESTG_DATATYPE_UNUSE
};

/*!
 * カスタムデフォルト(BOARD) - [シリアル化要素データ集合]
 *   E_TWESTG_DEFSETS_CHANNELを未使用にしている
 */
uint8 au8CustomDefault_Brd_Def[] = {
	2,   // 総バイト数(このバイトは含まない。手計算で間違えないように入力！)
	E_TWESTG_DEFSETS_CHANNEL, TWESTG_DATATYPE_UNUSE
};

/*!
 * 設定定義(tsSettings)
 *   スロット0..7までの定義を記述
 */
const TWESTG_tsSettingsListItem SetList[] = {
	{ STGS_KIND_USER_1, TWESTG_SLOT_DEFAULT,
		{ TWESTG_DEFSETS_BASE, NULL, NULL,
		  au8CustomDefault_Base, au8CustomDefault_Brd_Def, NULL } },
	{ STGS_KIND_USER_1, TWESTG_SLOT_1,
		{ TWESTG_DEFSETS_BASE, NULL, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  au8CustomDefault_Base, au8CustomDefault_Brd_Def, TWESTG_DEFCUST_SLOT[1] }}, // slot1 固有定義
	{ STGS_KIND_USER_1, TWESTG_SLOT_2,
		{ TWESTG_DEFSETS_BASE, NULL, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  au8CustomDefault_Base, au8CustomDefault_Brd_Def, TWESTG_DEFCUST_SLOT[2] }}, // slot2 固有定義
	{ STGS_KIND_USER_1, TWESTG_SLOT_3,
		{ TWESTG_DEFSETS_BASE, SetBrd, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  au8CustomDefault_Base, au8CustomDefault_Brd_Chan3, TWESTG_DEFCUST_SLOT[3] }}, // slot3 固有定義
	{ STGS_KIND_USER_1, TWESTG_SLOT_4,
		{ TWESTG_DEFSETS_BASE, SetBrd, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  au8CustomDefault_Base, au8CustomDefault_Brd_Def, TWESTG_DEFCUST_SLOT[4] }}, // slot4 固有定義
	{ STGS_KIND_USER_1, TWESTG_SLOT_5,
		{ TWESTG_DEFSETS_BASE, NULL, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  au8CustomDefault_Base, au8CustomDefault_Brd_Def, TWESTG_DEFCUST_SLOT[5] }}, // slot5 固有定義
	{ STGS_KIND_USER_1, TWESTG_SLOT_6,
		{ TWESTG_DEFSETS_BASE, NULL, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  au8CustomDefault_Base, au8CustomDefault_Brd_Def, TWESTG_DEFCUST_SLOT[6] }}, // slot6 固有定義
	{ STGS_KIND_USER_1, TWESTG_SLOT_7,
		{ TWESTG_DEFSETS_BASE, NULL, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  au8CustomDefault_Base, au8CustomDefault_Brd_Def, TWESTG_DEFCUST_SLOT[7] }}, // slot7 固有定義
	{ TWESTG_KIND_VOID, TWESTD_SLOT_VOID, { NULL }} // TERMINATE
};


/**
 * インタラクティブモードのメニュー構成
 */
const TWEINTRCT_tsFuncs asFuncs[] = {
	{ 0, (uint8*)"ROOT MENU", TWEINTCT_vSerUpdateScreen_defmenus, TWEINTCT_vProcessInputByte_defmenus, TWEINTCT_vProcessInputString_defmenus, TWEINTCT_u32ProcessMenuEvent_defmenus }, // standard settings
	{ 1, (uint8*)"CONFIG", TWEINTCT_vSerUpdateScreen_settings, TWEINTCT_vProcessInputByte_settings, TWEINTCT_vProcessInputString_settings, TWEINTCT_u32ProcessMenuEvent_settings  }, // standard settings
	{ 2, (uint8*)"EEPROM UTIL", TWEINTCT_vSerUpdateScreen_nvmutils, TWEINTCT_vProcessInputByte_nvmutils, TWEINTCT_vProcessInputString_nvmutils, TWEINTCT_u32ProcessMenuEvent_nvmutils }, // standard settings
	{ 0xFF, NULL, NULL, NULL }
};

#endif // _MSC_VER