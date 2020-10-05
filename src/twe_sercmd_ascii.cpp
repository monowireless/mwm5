/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_sercmd.hpp"
#include "twe_sercmd_ascii.hpp"

using namespace TWE;
using namespace TWESERCMD;
using namespace TWEUTILS;

namespace TWESERCMD {
	typedef enum {
		E_SERCMD_ASCII_CMD_EMPTY = 0,      //!< 入力されていない
		E_SERCMD_ASCII_CMD_READCOLON,      //!< E_SERCMD_ASCII_CMD_READCOLON
		E_SERCMD_ASCII_CMD_READPAYLOAD,    //!< E_SERCMD_ASCII_CMD_READPAYLOAD
		E_SERCMD_ASCII_CMD_READCR,         //!< E_SERCMD_ASCII_CMD_READCR
		E_SERCMD_ASCII_CMD_READLF,         //!< E_SERCMD_ASCII_CMD_READLF
		E_SERCMD_ASCII_CMD_COMPLETE = 0x80,//!< 入力が完結した(LCRチェックを含め)
		E_SERCMD_ASCII_CMD_ERROR = 0x81,          //!< 入力エラー
		E_SERCMD_ASCII_CMD_CHECKSUM_ERROR = 0x82,       //!< LRCが間違っている
	} teSercmdAsciiState;
}

/// <summary>
/// ASCII形式のパース
/// </summary>
/// <param name="u8byte"></param>
/// <returns></returns>
uint8_t AsciiParser::_u8Parse(uint8_t u8byte) {
	// check for timeout
	if (TimeOut::is_enabled() && TimeOut::is_timeout()) {
		u8state = E_SERCMD_ASCII_CMD_EMPTY;
	}

	// check for complete or error status
	if (u8state >= 0x80) {
		u8state = E_SERCMD_ASCII_CMD_EMPTY;
	}

	// run state machine
	switch (u8state) {
	case E_SERCMD_ASCII_CMD_EMPTY:
		if (u8byte == ':') {
			u8state = E_SERCMD_ASCII_CMD_READPAYLOAD;

			u16pos = 0;
			u16cksum = 0;
			payload.redim(0);

			// start new timer (if set timeout)
			TimeOut::start();
		}
		break;

	case E_SERCMD_ASCII_CMD_READPAYLOAD:
		if ((u8byte >= '0' && u8byte <= '9') || (u8byte >= 'A' && u8byte <= 'F')) {
			/* オーバーフローのチェック */
			//if (payload.length_max() >= (u16pos / 2) - 1) {
			//	u8state = E_SERCMD_ASCII_CMD_ERROR;
			//	break;
			//}

			/* 文字の16進変換 */
			const uint8_t u8tbl[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15 };
			uint8_t u8val = u8tbl[u8byte - '0'];

			/* バイナリ値として格納する */
			// int i = u16pos / 2;
			// unsigned char* p = &payload[u16pos / 2];

			if (!(u16pos & 1)) {
				if (!payload.append(u8val << 4)) {
					u8state = E_SERCMD_ASCII_CMD_ERROR;
				}
			}
			else {
				uint8_t c = payload[-1]; // -1 は末尾
				c |= u8val;

				payload[-1] = c;
				u16cksum += c;
			}

			u16pos++; // [0-9A-F]を１文字入れるごとにインクリメントする
		}
		else if (u8byte == 0x0d || u8byte == 0x0a) { // CR入力
			if (u16pos >= 4 && ((u16pos & 1) == 0) // データ部１バイト、チェックサム１バイト以上
				) {
				// チェックサムの確認
				u16cksum &= 0xFF; // チェックサムを 8bit に切り捨てる
				if (u16cksum) { // 正しければ 0 になっているはず
					// 格納値
					uint8_t u8lrc = payload[-1]; // u16posは最後のデータの次の位置
					// 計算値(二の補数の計算、ビット反転+1), デバッグ用に入力系列に対応する正しいLRCを格納しておく
					u16cksum = (~(u16cksum - u8lrc) + 1) & 0xFF;
					u8state = E_SERCMD_ASCII_CMD_CHECKSUM_ERROR;
				}
				else {
					// LRCが正しければ、全部足したら 0 になる。
					u8state = E_SERCMD_ASCII_CMD_COMPLETE; // 完了！
					payload.redim(payload.length() - 1); // 末尾の１文字を削除
				}
			}
			else {
				u8state = E_SERCMD_ASCII_CMD_ERROR;
			}
		}
		else if (u8byte == 'X') {
			// X で終端したらチェックサムの計算を省く
			if (u16pos >= 2 && ((u16pos & 1) == 0)) { // 入力データあり
				u8state = E_SERCMD_ASCII_CMD_COMPLETE; // 完了！
			}
		}
		else {
			u8state = E_SERCMD_ASCII_CMD_EMPTY;
		}
		break;

	default:
		break;
	}

	return u8state;
}



/** @ingroup SERCMD
 * シリアルコマンドアスキー形式の出力補助関数。１バイトを１６進数２文字で出力する (0xA5 -> "A5")
 *
 * @param psSerStream 出力先
 * @param u8byte 出力したいバイト列
 * @param pu8lrc LRC計算用
 */
void AsciiParser::vPutByte(uint8_t u8byte, uint8_t* pu8lrc, IStreamOut& vPutChar) {
	const uint8_t au8tbl[] = "0123456789ABCDEF";
	unsigned char u8OutH, u8OutL;

	u8OutH = au8tbl[u8byte >> 4];
	u8OutL = au8tbl[u8byte & 0x0F];

	vPutChar(u8OutH);
	vPutChar(u8OutL);

	if (pu8lrc) {
		*pu8lrc += u8byte;
	}
}

/// <summary>
/// ASCII形式の出力(staticメソッド)
/// </summary>
/// <param name="payload"></param>
/// <param name="vPutChar"></param>
void AsciiParser::s_vOutput(SmplBuf_Byte& payload, IStreamOut& vPutChar) {
	unsigned char u8lrc = 0;

	// 先頭の :
	vPutChar(':');

	uint8_t* p = payload.begin().raw_ptr();
	while (p != payload.end().raw_ptr()) {
		vPutByte(*p, &u8lrc, vPutChar);
		p++;
	}

	// ２の補数の計算 (LRC)
	u8lrc = ~u8lrc + 1;
	vPutByte(u8lrc, NULL, vPutChar);

	// Trailer byte
	vPutChar(0x0D);
	vPutChar(0x0A);
}