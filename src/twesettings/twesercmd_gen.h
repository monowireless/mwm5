/* Copyright (C) 2017,2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/** @file
 *
 * @defgroup SERCMD シリアルコマンドの解釈を行う
 * シリアルコマンドの解釈を行う。解釈部は書式に応じてアスキー形式、バイナリ形式、必要で
 * あるなら本定義に基づき拡張も可能である。
 *
 * - 入力の仕様（アスキー形式）
 *   - :([0-9A-F][0-9A-F])+[0-9A-F][0-9A-F][CR][LF]
 *   - 最後の 2 バイトは LRC となっていてLRCを含む 16進変換したバイトを加算すると 0 になる。\n
 *     つまり LRC は全データの和を取り、符号を反転（二の補数）させたもの。
 * - 入力の仕様（バイナリ形式）
 *   - 0xA5 0x5A [LenH] [LenL] [Payload] [XOR] [EOT]
 *      - LenH: ペイロードの長さ(バイト)の上位バイト (MSB を 0x80 にすること)
 *      - LenL: ペイロードの長さ(バイト)の下位バイト
 *      - Payload: ペイロード
 *      - XOR: ペイロード部の各バイトの XOR 値を用いたチェックサム
 *      - EOT: 省略可能。vOutput() 出力時には付与される。
 *
 * - API 利用方法
 *   - 準備
 *     - 事前に TWESERCMD_tsSerCmd_Context sSC 構造体および入力バッファデータ領域を確保しておく
 *     - SerCmdAscii_vInit() SerCmdBinary_vInit() 関数により構造体を初期化する
 *     - 入力タイムアウトを設定する場合は TWESERCMD_tsSerCmd_Context 構造体の u16timeout メンバーの値[ms]を設定する
 *
 *   - 解釈実行
 *     - UART からの入力バイト(u8Byte)を  uint8 u8ret = sSC.u8Parse(&sSC, u8Byte) に入力する
 *     - u8ret の値が 0x80 以上になると、解釈が終了したことを示す
 *        - E_TWESERCMD_COMPLETE: エラーなく系列が入力された。この呼び出しコンテキスト中で処理すること。
 *                             続く入力処理 (->u8Parse(), ->bComplete())が行われた時点でデータの一貫性
 *                             は保たれない。
 *        - E_TWESERCMD_ERROR: 解釈エラー
 *        - E_TWESERCMD_CHECKSUM_ERROR: チェックサムだけ合わなかった。このとき sSC.u16cksum に期待したチェックサム値が格納される。
 *     - 続けて ->u8Parse() を実行する。直前の状態が終了・エラー状態であっても、新たな系列として解釈を始めます。
 *
 *   - タイムアウトの処理
 *     外部のタイマから関数 ->bComplete() を実行するとタイムアウトのチェックを行う。
 *     - bComplete() が実装されていない(=NULL)の場合はタイムアウト処理は行えない。
 *     - 関数 ->bComplete() が TRUE を戻した場合は、呼び出しコンテキスト中でデータを利用すること。
 *       続く入力処理 (->u8Parse(), ->bComplete())が行われた時点でデータの一貫性は保たれない。
 *
 */

#ifndef TWESERCMD_GEN_H_
#define TWESERCMD_GEN_H_

#include "twecommon.h"
#include "tweserial.h"

// Serial options
//#include <serial.h>
//#include <fprintf.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @ingroup SERCMD
 * シリアルコマンド解釈の内部状態。
 * - 本状態は各解釈部共通の定義とする
 */
typedef enum {
	E_TWESERCMD_EMPTY = 0,                //!< 入力されていない
	E_TWESERCMD_COMPLETE = 0x80,          //!< 入力が完結した(LCRチェックを含め)
	E_TWESERCMD_ERROR = 0x81,             //!< 入力エラー
	E_TWESERCMD_CHECKSUM_ERROR = 0x82,   //!< チェックサムが間違っている
} teSerCmdGenState;

/** @ingroup SERCMD
 * シリアルコマンド解釈の管理構造体への型定義
 */
typedef struct _sSerCmd_Context TWESERCMD_tsSerCmd_Context;

/** @ingroup SERCMD
 * シリアルコマンド解釈の管理構造体
 */
struct _sSerCmd_Context {
	uint16 u16len; //!< ペイロード長
	uint8 *au8data; //!< バッファ（初期化時に固定配列へのポインタを格納しておく)
	uint16 u16maxlen; //!< バッファの最大長

	uint32 u32timestamp; //!< タイムアウトを管理するためのカウンタ
	uint16 u16timeout; //!< 入力系列のタイムアウト (0ならタイムアウト無し)

	uint16 u16pos; //!< 入力位置（内部処理用）
	uint16 u16cksum; //!< チェックサム

	uint8 u8state; //!< 状態

	void *vExtraData; //!< 処理系独特の追加データ

	uint8 (*u8Parse)(TWESERCMD_tsSerCmd_Context *pc, uint8 u8byte); //!< 入力用の関数
	void (*vOutput)(TWESERCMD_tsSerCmd_Context *pc, TWE_tsFILE *ps); //!< 出力用の関数
	bool_t (*bComplete)(TWESERCMD_tsSerCmd_Context *pc); //!< 完了確認
};

/*
 * 初期化関数群
 */

void TWESERCMD_Binary_vInit(TWESERCMD_tsSerCmd_Context *pc, uint8 *pbuff, uint16 u16maxlen);
void TWESERCMD_Ascii_vInit(TWESERCMD_tsSerCmd_Context *pc, uint8 *pbuff, uint16 u16maxlen);
void TWESERCMD_Chat_vInit(TWESERCMD_tsSerCmd_Context *pc, uint8 *pbuff, uint16 u16maxlen);
//void SerCmdTimeout_vInit(TWESERCMD_tsSerCmd_Context *pc, uint8 *pbuff, uint16 u16maxlen);

/*
 * 系列を出力する関数
 */
void TWESERCMD_Binary_vOutput(TWE_tsFILE* ps, uint8* pbuf, uint16 u16len);
void TWESERCMD_Ascii_vOutput(TWE_tsFILE* ps, uint8* pbuf, uint16 u16len);


#ifdef __cplusplus
}
#endif

#endif /* SERCMD_GEN_H_ */
