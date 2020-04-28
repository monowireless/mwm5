/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

 /** @file
  *
  * @defgroup TWESTGS CMD ヘッダファイル
  *
  */

#ifndef TWESETTINGS_CMD_H_
#define TWESETTINGS_CMD_H_

#include "twecommon.h"
#include "twesettings0.h"
#include "twesettings_std.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * TWESTG_CMD_u32CmdOpのu8Opの指定
 * 
 * 電文注：
 * CmdOp = ... -> 応答メッセージのコマンド番号（原則として入力されたコマンド番号と同じ番号で戻す）
 * [??]    -> バイト(=OCTET)、??は１６進数表記
 * {OCTET} -> １バイト
 * {WORD}  -> ２バイト（ビッグエンディアン）
 * {DWORD} -> ４バイト（ビッグエンディアン）
 * 
 * 未対応メッセージのエラー: 以下のメッセージを返す
 * CmdOp = E_TWESTG_CMD_OP_ACK
 * [DE][AD][BE][EF]
 */
typedef enum {
	/*!
	 * ACK を返す（モジュールの生存確認をするなどの目的）
	 * 
	 * 入力ペイロード
	 *   任意
	 * 
	 * 出力ペイロード
	 *   入力ペイロードなしの場合
	 *   CmdOp = [F0] E_TWESTG_CMD_OP_ACK
	 *   [01]		: 0x01 を戻す
	 * 
	 *   入力ペイロードありの場合
	 *   CmdOp = [F0] E_TWESTG_CMD_OP_ACK
	 *   [...]		: 入力ペイロードと同じ系列
	 */
	E_TWESTG_CMD_OP_ACK = 0xF0,

	/*!
	 * モジュールアドレスなどの情報を戻す
	 * 
	 * 入力ペイロード
	 *   [01]		: モジュールのアドレス
	 * 
	 * 出力ペイロード
	 *   CmdOp = [F1] E_TWESTG_CMD_OP_QUERY_MODULE_INFO
	 *   [01]       : モジュールのアドレス
	 *   {DWORD}	: ビッグエンディアン形式でモジュールのアドレスを返す
	 */
	E_TWESTG_CMD_OP_QUERY_MODULE_INFO = 0xF1,

	/*!
	 * 現在選択されている KIND/SLOT に対して設定データを適用する（セーブはしない）
	 * 
	 * 入力ペイロード
	 *   {OCTET}	: オプション(0x00は未指定)
	 *   {シリアル化セーブ形式}
	 *              : 標準のセーブ形式
	 * 
	 * 出力ペイロード
	 *   CmdOp = [F3] E_TWESTG_CMD_OP_APPLY_SETTINGS
	 *   {OCTET}	: 戻り状況(0x00は特記なし)
	 *   {シリアル化セーブ形式}		
	 *              : 標準のセーブ形式（適用後のセーブデータ形式を戻す）
	 */
	E_TWESTG_CMD_OP_APPLY_SETTINGS = 0xF2,

	/*!
	 * 現在選択されている KIND/SLOT から設定データを得る。
	 * 
	 * 入力ペイロード
	 *   {OCTET}	: オプション(0x00は未指定,本バイトは省略可能)
	 *
	 * 	 * 出力ペイロード
	 *   CmdOp = [F3] E_TWESTG_CMD_OP_QUERY_SETTINGS
	 *   {OCTET}	: 戻り状況(0x00は特記なし)
	 *   {シリアル化セーブ形式}
	 *              : 標準のセーブ形式（適用後のセーブデータ形式を戻す）
	 */
	E_TWESTG_CMD_OP_QUERY_SETTINGS = 0xF3,

	/*!
	 * モジュール制御を行う
	 * 
	 * 入力ペイロード
	 *   {OCTET}	: モジュール制御コマンド番号
	 *   ...		: モジュール制御コマンドに依存
	 * 
	 * 出力ペイロード
	 *   {OCTET}	: モジュール制御コマンド番号
	 *   ...		: モジュール制御コマンドに依存
	 * 
	 * KIND/SLOTの設定（取得）
	 *   入力ペイロード
	 *   [20]		: モジュール制御コマンド番号
	 *   {OCTET}	: KIND設定 (0xFFは設定なし）
	 *   {OCTET}	: SLOT設定（0xFFは設定なし）
	 *  
	 *   出力ペイロード
	 *   [20]		: モジュール制御コマンド番号
	 *   {OCTET}	: KIND設定, 0xFF:失敗時(ここで終端)
	 *   {OCTET}	: SLOT設定
	 */
	E_TWESTG_CMD_OP_MODULE_CONTROL = 0xF8,

	/*!
	 * データセーブとモジュールリセットを行う
	 * 
	 *   入出力なし(PCテスト用に [FE][00] を戻すが、実機ではその前にリセット)
	 */
	E_TWESTG_CMD_OP_SAVE_AND_RESET = 0xFE,

	/*!
	 * モジュールリセットを行う
	 * 
	 *   入出力なし(PCテスト用に [FF][00] を戻すが、実機ではその前にリセット)
	 */
	E_TWESTG_CMD_OP_DO_MDDULE_RESET = 0xFF,
} TWESTG_teCMD_OP;

TWE_APIRET TWESTG_CMD_u32CmdOp(uint8 u8Op, TWE_tsBuffer *pBufIn, TWE_tsBuffer *pBufOut, TWESTG_tsFinal *psFinal);


#ifdef __cplusplus
}
#endif


#endif