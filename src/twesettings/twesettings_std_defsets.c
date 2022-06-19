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
