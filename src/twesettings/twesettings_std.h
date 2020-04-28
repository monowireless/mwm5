/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

 /** @file
  *
  * @defgroup TWESTGS ヘッダファイル
  *
  */

#ifndef TWESETTINGS_STD_H_
#define TWESETTINGS_STD_H_

#include "twecommon.h"
#include "twesettings0.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TWESTG_STD_SAVEBUFF_SIZE 128 // セーブバッファの最大サイズ
#define TWESTG_STD_OPT_NOLOAD 0x1 // データをロードせずに初期化する

// 設定データ(tsFinal)初期化後に、必要情報(appIDや設定バージョン)を反映させる
TWE_APIRET TWESTG_u32SetBaseInfoToFinal(TWESTG_tsFinal * psFinal, uint32 u32AppId, uint32 u32AppVer, uint8 u8SetVer, uint8 u8SetCompat);
// 設定データ(tsFinal)に、種別・スロット・設定定義(tsSettings)を反映させる
TWE_APIRET TWESTG_u32SetSettingsToFinal(TWESTG_tsFinal *psFinal, uint8 u8kind, uint8 u8slot, const TWESTG_tsSettingsListItem *pList);

// アプリケーションの保存領域(AppStrg, EEPROMなど)から設定データをロードする
TWE_APIRET TWESTG_u32LoadDataFrAppstrgSingle(TWESTG_tsFinal *pFinal, uint8 u8kind, uint8 u8slot, uint32 u32AppId, uint8 u8Ver, uint32 u32Opt);
// アプリケーションの保存領域(AppStrg, EEPROMなど)から設定データをロードする(slot0 も同時にロードする)
TWE_APIRET TWESTG_u32LoadDataFrAppstrg(TWESTG_tsFinal* pFinal, uint8 u8kind, uint8 u8slot, uint32 u32AppId, uint8 u8Ver, uint32 u32Op);
// アプリケーションの保存領域(AppStrg, EEPROMなど)に設定データをセーブする(選択スロットのみ)
TWE_APIRET TWESTG_u32SaveDataToAppStrg(TWESTG_tsFinal *pFinal, uint8 u8kind, uint8 u8slot, uint32 u32AppId, uint8 u8Ver, uint32 u32Opt);

// 設定データ(tsFinal)からシリアル化したバイト列を生成
TWE_APIRET TWESTG_STD_u32FinalToSerializedBuff(TWE_tsBuffer *pBuff, TWESTG_tsFinal *psFinal, uint8 u8FormatVer, uint32 u32Opt);
// シリアル化したバイト列から設定データ(tsFinal)を生成
TWE_APIRET TWESTG_CMD_u32SerializedBuffToFinal(TWESTG_tsFinal *psFinal, TWE_tsBuffer *pBuff, uint32 u32Opt);


#ifdef __cplusplus
}
#endif


#endif