/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

#ifdef _MSC_VER
#pragma once

#include "twesettings/twecommon.h"
#include "twesettings/twesettings.h"

#ifdef __cplusplus
extern "C" {
#endif // C++

#define APPID 0x12345678   //! アプリケーションID
#define APPID_CRC8 0x37    //! アプリケーションIDのCRC8(計算した値)
#define APPVER 0x01020300  //! バージョン番号
#define APPVER_CRC8 0x3A   //! バージョン番号のCRC8(計算した値)

#define STGS_SET_VER 0x01         //! 設定バージョン
#define STGS_SET_VER_COMPAT 0x01  //! 互換性のある設定バージョン

#define STGS_MAX_SETTINGS_COUNT 32 //! 設定数の最大(確定設定リスト tsFinal の配列数を決める)

#define STGS_KIND_USER_1 0x01      //! 種別ID

extern const TWESTG_tsElement SetBrd[];
extern uint8 au8CustomDefault_Base[];
extern uint8 au8CustomDefault_Brd_Chan3[];
extern uint8 au8CustomDefault_Brd_Def[];
extern const TWESTG_tsSettingsListItem SetList[];
extern const TWEINTRCT_tsFuncs asFuncs[], asFuncs_en[];

#ifdef __cplusplus
}
#endif // C++
#endif // _MSC_VER