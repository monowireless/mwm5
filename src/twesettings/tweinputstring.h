/* Copyright (C) 2017,2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/** @file
 *
 * @defgroup INPSTR UARTのインタラクティブな１行文字列入力処理
 *
 * UARTよりインタラクティブに文字列を入力する。
 *
 * - 入力の仕様
 *   - 入力された文字はエコーバックする。
 *   - Enter で確定する。
 *   - BS/DEL を入力すると、一文字削除しターミナルにも BS を出力する。
 *   - 他のコントロールコードを受け取るとキャンセルとする。
 *
 * - API 利用方法
 *   - 事前に INPSTR_vInit() により初期化しておくこと
 *   - 文字列入力が必要になった時点で INPSTR_vStart() を呼び出し、それ以降 UART からの入力バイトを１バイトずつ INPSTR_u8InputByte() により入力する。
 *   - INPSTR_bActive() により、入力中であるかどうかを確認してから INPSTR_u8InputByte() を呼び出す
 *   - INPSTR_u8InputByte() の戻り値が E_INPUTSTRING_STATE_COMPLETE, E_INPUTSTRING_STATE_CANCELED なら入力は完了。
 *   - 入力文字列 au8Data[] は '\0' で終端されており u32string2dec() や u32string2hex() により uint32 値への変換が可能である。
 */

#ifndef INPUT_STRING_H_
#define INPUT_STRING_H_

#include "twecommon.h"
#include "tweserial.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @ingroup INPSTR
 * 入力文字列の種別
 */
enum {
	E_TWEINPUTSTRING_DATATYPE_STRING = 0,   //!< 文字列型入力
	E_TWEINPUTSTRING_DATATYPE_DEC,          //!< １０進数 [0-9]
	E_TWEINPUTSTRING_DATATYPE_HEX,          //!< １６進数 [0-9a-fA-F]
	E_TWEINPUTSTRING_DATATYPE_DEC_DECA,	    //!< １０進数（設定値の1/10で保存）
	E_TWEINPUTSTRING_DATATYPE_DEC_HECTO,	//!< １０進数（設定値の1/100で保存）
	
	E_TWEINPUTSTRING_DATATYPE_STANDARD_MASK = 0x0F,
	E_TWEINPUTSTRING_DATATYPE_CUSTOM_DISP_MASK = 0x80
};


/** @ingroup INPSTR
 * 入力状態
 */
enum {
	E_TWEINPUTSTRING_STATE_IDLE = 0, //!< 入力中でない
	E_TWEINPUTSTRING_STATE_ACTIVE,   //!< 入力中
	E_TWEINPUTSTRING_STATE_COMPLETE, //!< 完了
	E_TWEINPUTSTRING_STATE_CANCELED, //!< キャンセル
};

/** @ingroup INPSTR
 * 管理構造体
 */
typedef struct _tsInpStr_Context {
	uint8 au8Data[32]; //!< 入力文字バッファ
	uint8 u8State; //!< 状態
	uint8 u8Idx; //!< 入力文字数（＝配列上のインデックス）
	uint8 u8DataType; //!< データ種別
	uint8 u8MaxLen; //!< 入力時の最大長
	uint32 u32Opt; //!< アプリケーション側で保存しておきたい情報
	TWE_tsFILE *pSerStream; //!< 入出力ストリーム
} TWEINPSTR_tsInpStr_Context;

void TWEINPSTR_vInit(TWEINPSTR_tsInpStr_Context*, TWE_tsFILE*);
void TWEINPSTR_vStart(TWEINPSTR_tsInpStr_Context*, uint8 u8DataType, uint8 u8Maxlen, uint32 u32Opt);
#define INPSTR_bActive(s) ((s)->u8State == E_TWEINPUTSTRING_STATE_IDLE ? FALSE : TRUE) //!< tsInpStr_Context 管理構造体が ACTVE かどうか判定する @ingroup INPSTR
uint8 TWEINPSTR_u8InputByte(TWEINPSTR_tsInpStr_Context*, uint8);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_STRING_H_ */
