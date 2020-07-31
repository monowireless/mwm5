/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twecommon.h"
#include "twesettings0.h"
#include "twesettings_std.h"
#include "twesettings_std_defsets.h"
#include "twesettings_validator.h"
#include "tweinputstring.h"
#include "tweutils.h"

/*!
 * 共通定義
 * 基本設定を追加する。
 */
const TWESTG_tsElement TWESTG_DEFSETS_BASE[] = {
	{ E_TWESTG_DEFSETS_APPID,  // アプリケーションID
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = 0x67726301 }}, // 32bit (デフォルトのIDは配列決め打ちなので、ボード定義でオーバライドが必要)
		{ "AID", "Application ID [HEX:32bit]", "" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 8, 'a' },
		{ {.u32 = 0}, {.u32 = 0}, TWESTGS_VLD_u32AppId, NULL },
	},
	{ E_TWESTG_DEFSETS_LOGICALID,
		{ TWESTG_DATATYPE_UINT8,  sizeof(uint8),  0, 0, {.u8 = 1 }}, 
		{ "LID", "Device ID [1-100,etc]", "" }, 
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 3, 'i' },
		{ {.u32 = 0}, {.u32 = 100}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_CHANNEL,
		{ TWESTG_DATATYPE_UINT8,  sizeof(uint8),  0, 0, {.u8 = 18 }},
		{ "CHN", "Channel [11-26]", "" },
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 2, 'c' },
		{ {.u32 = 11}, {.u32 = 26}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_CHANNELS_3,
		{ TWESTG_DATATYPE_UINT16, sizeof(uint16), 0, 0, {.u16 = ((1UL << 18) >> 11) }},
		{ "CHL", "Channels Set", 
		  "Input up to 3 channels like '11,15,24'."
		},
		{ E_TWEINPUTSTRING_DATATYPE_CUSTOM_DISP_MASK | E_TWEINPUTSTRING_DATATYPE_STRING, 8, 'c' },
		{ {.u16 = 0}, {.u16 = 0xFFFF}, TWESTGS_VLD_u32ChList, NULL },
	},
	{ E_TWESTG_DEFSETS_POWER_N_RETRY,
		{ TWESTG_DATATYPE_UINT8,  sizeof(uint8),  0, 0, {.u8 = 0x03 }},
		{ "PWR", "RF Power/Retry [HEX:8bit]",
			"YZ Y=Retry(0:default,F:0,1-9:count"_TWELB
			"Z = Power(3:Max,2,1,0 : Min)" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 2, 'x' },
		{ {.u32 = 0}, {.u32 = 0xFF}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_UARTBAUD,
		{ TWESTG_DATATYPE_UINT16, sizeof(uint16), 0, 0, {.u16 = 384 }},
		{ "UOP", "UART Baud [9600-230400]", "" },
		{ E_TWEINPUTSTRING_DATATYPE_CUSTOM_DISP_MASK | E_TWEINPUTSTRING_DATATYPE_STRING, 10, 'b' },
		{ {.u16 = 0}, {.u16 = 0}, TWESTGS_VLD_u32UartBaudOpt, NULL },
	},
	{ E_TWESTG_DEFSETS_OPTBITS, 
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = 0x00000000 }}, 
		{ "OPT", "Option Bits [HEX:32bit]", "" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 8, 'o' },
		{ {.u32 = 0}, {.u32 = 0xFFFFFFFF}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{E_TWESTG_DEFSETS_VOID} // FINAL DATA
};

/*!
 * 共通定義
 * 基本設定を追加する。
 */
const TWESTG_tsElement TWESTG_DEFSETS_BASE_MWX[] = {
	{ E_TWESTG_DEFSETS_APPID,  // アプリケーションID
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = 0x67726301 }}, // 32bit (デフォルトのIDは配列決め打ちなので、ボード定義でオーバライドが必要)
		{ "AID", "Application ID [HEX:32bit]", "" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 8, 'a' },
		{ {.u32 = 0}, {.u32 = 0}, TWESTGS_VLD_u32AppId, NULL },
	},
	{ E_TWESTG_DEFSETS_LOGICALID,
		{ TWESTG_DATATYPE_UINT8,  sizeof(uint8),  0, 0, {.u8 = 1 }}, 
		{ "LID", "Device ID [1-100,etc]", "" }, 
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 3, 'i' },
		{ {.u32 = 0}, {.u32 = 100}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_CHANNEL,
		{ TWESTG_DATATYPE_UINT8,  sizeof(uint8),  0, 0, {.u8 = 18 }},
		{ "CHN", "Channel [11-26]", "" },
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 2, 'c' },
		{ {.u32 = 11}, {.u32 = 26}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_POWER_N_RETRY,
		{ TWESTG_DATATYPE_UINT8,  sizeof(uint8),  0, 0, {.u8 = 0x03 }},
		{ "PWR", "RF Power/Retry [HEX:8bit]",
			"YZ Y=Retry(0:default,F:0,1-9:count"_TWELB
			"Z = Power(3:Max,2,1,0 : Min)" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 2, 'x' },
		{ {.u32 = 0}, {.u32 = 0xFF}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_OPTBITS, 
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = 0x00000000 }}, 
		{ "OPT", "Option0 [HEX:32bit]", "" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 8, 'o' },
		{ {.u32 = 0}, {.u32 = 0xFFFFFFFF}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_OPT_DWORD1, 
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = 0x00000000 }}, 
		{ "OPT", "Option1 [HEX:32bit]", "" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 8, '1' },
		{ {.u32 = 0}, {.u32 = 0xFFFFFFFF}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_OPT_DWORD2, 
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = 0x00000000 }}, 
		{ "OPT", "Option2 [HEX:32bit]", "" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 8, '2' },
		{ {.u32 = 0}, {.u32 = 0xFFFFFFFF}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_OPT_DWORD3, 
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = 0x00000000 }}, 
		{ "OPT", "Option3 [HEX:32bit]", "" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 8, '3' },
		{ {.u32 = 0}, {.u32 = 0xFFFFFFFF}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{E_TWESTG_DEFSETS_VOID} // FINAL DATA
};

/*!
 * カスタムデフォルト対応スロット数
 */
const uint8 TWESTG_DEFCUST_U8_SLOT_NUMBER = 8; // 0...8 までの配列

/*!
 * カスタムデフォルト、スロット番号ごとに LID を設定する
 *
 * const uint8 XXX[][各スロットで格納される最大バイト] = {
 *    { SLOT定義のデータ長, ID, データ型, データ, ID, データ型, データ, ... },  // SLOT定義0
 *    ...
 *    { SLOT定義のデータ長, ID, データ型, データ, ID, データ型, データ, ... }}; // SLOT定義N
 */
const uint8 TWESTG_DEFCUST_SLOT[][4] = {
	{ 1, E_TWESTG_DEFSETS_VOID },                                           // 0 はダミー
	{ 3, E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8 << 4) | 1, 2 }, // SLOT1
	{ 3, E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8 << 4) | 1, 3 }, // SLOT2
	{ 3, E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8 << 4) | 1, 4 }, // ...
	{ 3, E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8 << 4) | 1, 5 },
	{ 3, E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8 << 4) | 1, 6 },
	{ 3, E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8 << 4) | 1, 7 },
	{ 3, E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8 << 4) | 1, 8 },
};

/*!
 * E_TWESTG_DEFSETS_CHANNELS_3（チャネル設定、最大３チャネル）を削除する
 */
const uint8 TWESTG_DEFCUST_REMOVE_CHAN3[] = {
	2,   // 総バイト数(このバイトは含まない。手計算で間違えないように入力！)
	E_TWESTG_DEFSETS_CHANNELS_3, TWESTG_DATATYPE_UNUSE
};

/*!
 * E_TWESTG_DEFSETS_CHANNEL (チャネル設定：１チャンネルのみ）を削除する
 */
const uint8 TWESTG_DEFCUST_REMOVE_CHAN1[] = {
	2,   // 総バイト数(このバイトは含まない。手計算で間違えないように入力！)
	E_TWESTG_DEFSETS_CHANNEL, TWESTG_DATATYPE_UNUSE
};

/*!
 * 設定定義(tsSettings)
 *   スロット0..7までの定義を記述 - MWX 標準
 */
const TWESTG_tsSettingsListItem stgsListBaseMWX[] = {
	{ 1, TWESTG_SLOT_DEFAULT, 
		{ TWESTG_DEFSETS_BASE_MWX, NULL, NULL, 
		  NULL, NULL, NULL } },
	{ 1, TWESTG_SLOT_1, 
		{ TWESTG_DEFSETS_BASE_MWX, NULL, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  NULL, NULL, TWESTG_DEFCUST_SLOT[1] }}, // slot1 固有定義
	{ 1, TWESTG_SLOT_2,
		{ TWESTG_DEFSETS_BASE_MWX, NULL, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  NULL, NULL, TWESTG_DEFCUST_SLOT[2] }}, // slot2 固有定義
	{ 1, TWESTG_SLOT_3,
		{ TWESTG_DEFSETS_BASE_MWX, NULL, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  NULL, NULL, TWESTG_DEFCUST_SLOT[3] }}, // slot3 固有定義
	{ 1, TWESTG_SLOT_4,
		{ TWESTG_DEFSETS_BASE_MWX, NULL, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  NULL, NULL, TWESTG_DEFCUST_SLOT[4] }}, // slot4 固有定義
	{ 1, TWESTG_SLOT_5,
		{ TWESTG_DEFSETS_BASE_MWX, NULL, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  NULL, NULL, TWESTG_DEFCUST_SLOT[5] }}, // slot5 固有定義
	{ 1, TWESTG_SLOT_6,
		{ TWESTG_DEFSETS_BASE_MWX, NULL, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  NULL, NULL, TWESTG_DEFCUST_SLOT[6] }}, // slot6 固有定義
	{ 1, TWESTG_SLOT_7,
		{ TWESTG_DEFSETS_BASE_MWX, NULL, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  NULL, NULL, TWESTG_DEFCUST_SLOT[7] }}, // slot7 固有定義
	{ TWESTG_KIND_VOID, TWESTD_SLOT_VOID, { NULL }} // TERMINATE
};