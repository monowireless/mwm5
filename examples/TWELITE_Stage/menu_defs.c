/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twesettings/tweset.h"
#include "menu_defs.h"
#include "menu_info.h"

/*!
 * Common settings
 */
const TWESTG_tsElement TWESTG_STAGE_BASE[] = {
	{ E_TWESTG_STAGE_START_APP, 
		{ TWESTG_DATATYPE_UINT8, sizeof(uint8), 0, 0, {.u8 = 0 }}, 
		{ "AID", "起動アプリ指定", 
		  "始動アプリケーション番号を指定します。\r\n"
		  "0は指定なし。 1..はメニュー番号。"
		},
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 8, 'a' },
		{ {.u32 = 0}, {.u32 = 10}, TWESTGS_VLD_u32MinMax, NULL },
	},
#ifndef ESP32
	{ E_TWESTG_STAGE_SCREEN_MODE,
		{ TWESTG_DATATYPE_UINT8,  sizeof(uint8),  0, 0, {.u8 = 0x00 }}, 
		{ "SCM", "画面サイズ・描画方法",
		  "XYの２桁の文字で指定します(X:画面サイズ Y:描画方法)\r\n"
		  "X 0:640x480 1:960x720 2:1280x720\r\n"
		  "  3:1280x960 4:1920x1080 5:320x240\r\n"
		  "Y 0:LCD風 1:CRT風 2:ぼやけ 3:ブロック"
		}, 
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 3, 'G' },
		{ {.u32 = 0}, {.u32 = 0xFF}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_STAGE_FTDI_ADDR,
		{ TWESTG_DATATYPE_STRING,  8, 0, 0, { .pu8 = (uint8*)"\0       " }}, 
		{ "FTA", "シリアルデバイスID",
		  "FTDIデバイスのシリアル番号を指定します。\r\n"
		  "指定しない場合は空白にします" }, 
		{ E_TWEINPUTSTRING_DATATYPE_STRING, 8, 'F' },
		{ {.u32 = 0}, {.u32 = 0}, TWESTGS_VLD_u32String, NULL },
	},
#else
	{ E_TWESTG_STAGE_KEYBOARD_LAYOUT,
		{ TWESTG_DATATYPE_UINT8,  sizeof(uint8),  0, 0, {.u8 = 1 }},
		{ "KYB", "PS/2キーボードレイアウト",
		  "PS/2キーボードのレイアウトを指定します。\r\n"
		  "  0: US 1: JP\r\n"
		  "※電源再投入後に反映されます。" },
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 1, 'k' },
		{ {.u32 = 0}, {.u32 = 1 }, TWESTGS_VLD_u32MinMax, NULL },
	},
#endif
	{ E_TWESTG_STAGE_FG_COLOR,
		{ TWESTG_DATATYPE_UINT32,  sizeof(uint32),  0, 0, {.u32 = 0xFFFFFF }},
		{ "FGC", "文字色",
		  "文字色を RRGGBB の16進6文字で指定します。\r\n" 
		  "例えば赤 FF0000、黒 0000000、白、FFFFFF" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 6, 'f' },
		{ {.u32 = 0}, {.u32 = 0xFFFFFF}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_STAGE_BG_COLOR,
		{ TWESTG_DATATYPE_UINT32,  sizeof(uint32),  0, 0, {.u32 = ((90 << 16) | 50) }},
		{ "BGC", "背景色",
		  "背景色を RRGGBB の16進6文字で指定します。\r\n" 
		  "例えば赤 FF0000、黒 0000000、白、FFFFFF" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 6, 'b' },
		{ {.u32 = 0}, {.u32 = 0xFFFFFF}, TWESTGS_VLD_u32MinMax, NULL },
	},
	
	{E_TWESTG_DEFSETS_VOID} // FINAL DATA
};

static const uint8 DEF_TWESTG_DATATYPE_STRING[32] = ""; 
const TWESTG_tsElement TWESTG_SLOT_SCREEN_BUILD[] = {
#ifndef ESP32
	{ E_TWESTG_STAGE_APPWRT_BUILD_MAKE_JOGS,
		{ TWESTG_DATATYPE_UINT8, sizeof(uint8), 0, 0, {.u8 = 0 }},
		{ "JOBS", "ビルド時のmakeジョブ数",
		  "ビルド時のmakeで並列処理するジョブ数です。\r\n"
		  "0: 既定値で論理CPU数の半分を指定します。\r\n"
		  "1: 並列処理をしません。\r\n"
		  "1以上: ジョブ数。物理CPU数前後が最適です。" },
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 2, 'j' },
		{ {.u32 = 0}, {.u32 = 0xFFFFFF}, TWESTGS_VLD_u32MinMax, NULL } },
	{ E_TWESTG_STAGE_APPWRT_OPEN_CODE,
		{ TWESTG_DATATYPE_UINT8, sizeof(uint8), 0, 0, {.u8 = 0 }},
		{ "VSC", "codeでフォルダを開く(VSCode)",
		  "フォルダを code で開きます\r\n"
		  "0: 通常の方法でフォルダを開きます。\r\n"
		  "1: code コマンドでフォルダを開きます。\r\n"
		  "   VSCodeのｲﾝｽﾄｰﾙや設定が必要になります。"},
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 2, 'v' },
		{ {.u32 = 0}, {.u32 = 1}, TWESTGS_VLD_u32MinMax, NULL } },
# if defined(_MSC_VER) || defined(__MINGW32__)
	{ E_TWESTG_STAGE_APPWRT_FORCE_DISABLE_LTO,
		{ TWESTG_DATATYPE_UINT8, sizeof(uint8), 0, 0, {.u8 = 0 }},
		{ "LTO", "LTOを行わない",
		  "LTOを行うとBINサイズが5%前後小さくなります。\r\n"
		  "行わないを選択するとリンク時間が短くなります。\r\n"
		  "0: LTOを行う。\r\n"
		  "1: LTOを行わない。" },
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 2, 'l' },
		{ {.u32 = 0}, {.u32 = 1}, TWESTGS_VLD_u32MinMax, NULL } },
# endif
#endif
	{ E_TWESTG_STAGE_APPWRT_BUILD_NEXT_SCREEN,
		{ TWESTG_DATATYPE_UINT8, sizeof(uint8), 0, 0, {.u8 = 0 }},
		{ "NXT", "書換完了後の画面",
		  "書換完了後の画面を選択します\r\n"
		  "0: 既定値でｲﾝﾀﾗｸﾃｨﾌﾞﾓｰﾄﾞにします。\r\n"
		  "1: ターミナル画面にします。\r\n"
		  "2: 書換メニューに戻ります。"
		},
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 2, 'n' },
		{ {.u32 = 0}, {.u32 = 16}, TWESTGS_VLD_u32MinMax, NULL } },
	{E_TWESTG_DEFSETS_VOID} // TERMINATOR
};

const TWESTG_tsElement TWESTG_SLOT_SCREEN_INTRCT[] = {
#ifdef ESP32
	{ E_TWESTG_STAGE_INTRCT_USE_SETPIN,
		{ TWESTG_DATATYPE_UINT8,  sizeof(uint8),  0, 0, {.u8 = 1 }},
		{ "GP5", "GPIO5をSETピン制御に使用する",
		  "GPIO5を使用するハードウェア(Facesキーパッドなど)\r\n"
		  "を接続する場合は OFF にします。\r\n"
		  "  0:OFF  1:ON(GPIO5=SETピン制御) \r\n"
		  "※電源再投入後に反映されます。" },
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 1, '5' },
		{ {.u32 = 0}, {.u32 = 1 }, TWESTGS_VLD_u32MinMax, NULL },
	},
#endif
	{E_TWESTG_DEFSETS_VOID}
};

/*!
 * Custom default (another default value / hide an item)
 *   - hide items which is not necessary under sub settings.
 */
uint8 au8CustomDefault_Unuse[] = {
#ifdef ESP32
	4,   // total bytes afterwards 
	E_TWESTG_STAGE_START_APP, TWESTG_DATATYPE_UNUSE, // hide an item
	E_TWESTG_STAGE_KEYBOARD_LAYOUT, TWESTG_DATATYPE_UNUSE, // hide an item
#else
	6,   // total bytes afterwards
	E_TWESTG_STAGE_START_APP, TWESTG_DATATYPE_UNUSE, // hide an item
	E_TWESTG_STAGE_SCREEN_MODE, TWESTG_DATATYPE_UNUSE, // hide an item
	E_TWESTG_STAGE_FTDI_ADDR, TWESTG_DATATYPE_UNUSE, // hide an item
#endif
};

/*!
 * Combination of settings mapping into each slot (tsSettings)
 */
#if 0 // this list will be generated by void MyTweMenu::gen_SetList()
const TWESTG_tsSettingsListItem SetList[] = {
	{ 0, TWESTG_SLOT_DEFAULT, // common settings
		{ TWESTG_STAGE_BASE, NULL, NULL,
		  NULL, NULL, NULL } },
	{ 0, TWESTG_SLOT_1, // slot#1 (for app#1)
		{ TWESTG_STAGE_BASE, NULL, NULL,
		  NULL, au8CustomDefault_Unuse, NULL }},
	{ 0, TWESTG_SLOT_2,
		{ TWESTG_STAGE_BASE, NULL, NULL,
		  NULL, au8CustomDefault_Unuse, NULL }},
	{ 0, TWESTG_SLOT_3,
		{ TWESTG_STAGE_BASE, NULL, NULL,
		  NULL, au8CustomDefault_Unuse, NULL }},
	{ 0, TWESTG_SLOT_4,
		{ TWESTG_STAGE_BASE, NULL, TWESTG_SLOT_SCREEN_BUILD,
		  NULL, au8CustomDefault_Unuse, NULL }},
	{ 0, TWESTG_SLOT_5,
		{ TWESTG_STAGE_BASE, NULL, TWESTG_SLOT_SCREEN_INTRCT,
		  NULL, au8CustomDefault_Unuse, NULL }},
	{ TWESTG_KIND_VOID, TWESTD_SLOT_VOID, { NULL }} // TERMINATE
};
#endif

/**
 * menu items for interactive menus.
 */
const TWEINTRCT_tsFuncs asFuncs[] = {
	{ 0, (uint8*)"設定メニュー", TWEINTCT_vSerUpdateScreen_defmenus, TWEINTCT_vProcessInputByte_defmenus, TWEINTCT_vProcessInputString_defmenus, TWEINTCT_u32ProcessMenuEvent_defmenus }, // standard settings
	{ 1, (uint8*)"STAGE設定", TWEINTCT_vSerUpdateScreen_settings, TWEINTCT_vProcessInputByte_settings, TWEINTCT_vProcessInputString_settings, TWEINTCT_u32ProcessMenuEvent_settings  }, // standard settings
	{ 2, (uint8*)"ｾｰﾌﾞﾃﾞｰﾀ ﾕｰﾃｨﾘﾃｨ(ﾀﾞﾝﾌﾟ/消去)", TWEINTCT_vSerUpdateScreen_nvmutils, TWEINTCT_vProcessInputByte_nvmutils, TWEINTCT_vProcessInputString_nvmutils, TWEINTCT_u32ProcessMenuEvent_nvmutils }, // standard settings
	{ 3, (uint8*)"情報", TWEINTCT_vSerUpdateScreen_stage_info, TWEINTCT_vProcessInputByte_stage_info, TWEINTCT_vProcessInputString_stage_info, TWEINTCT_u32ProcessMenuEvent_stage_info }, // standard settings
	{ 0xFF, NULL, NULL, NULL }
};

