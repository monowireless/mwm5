/* Copyright (C) 2017,2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <string.h>

#include "twecommon.h"
#include "tweserial.h"
#include "twesercmd_gen.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/** @ingroup SERCMD
 * シリアルコマンド（アスキー形式）の状態定義
 */
typedef enum {
	E_TWESERCMD_ASCII_CMD_EMPTY = 0,      //!< 入力されていない
	E_TWESERCMD_ASCII_CMD_READCOLON,      //!< E_TWESERCMD_ASCII_CMD_READCOLON
	E_TWESERCMD_ASCII_CMD_READPAYLOAD,    //!< E_TWESERCMD_ASCII_CMD_READPAYLOAD
	E_TWESERCMD_ASCII_CMD_READCR,         //!< E_TWESERCMD_ASCII_CMD_READCR
	E_TWESERCMD_ASCII_CMD_READLF,         //!< E_TWESERCMD_ASCII_CMD_READLF
	E_TWESERCMD_ASCII_CMD_COMPLETE = 0x80,//!< 入力が完結した(LCRチェックを含め)
	E_TWESERCMD_ASCII_CMD_ERROR = 0x81,          //!< 入力エラー
	E_TWESERCMD_ASCII_CMD_CHECKSUM_ERROR = 0x82,       //!< LRCが間違っている
} TWESERCMD_Ascii_teState;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************///
#if defined(ESP32)
extern uint32_t _u32GetTick_ms();
#define u32TickCount_ms _u32GetTick_ms()
#else
extern volatile uint32 u32TickCount_ms; //!< ToCoNet での TickTimer
#endif

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/** @ingroup SERCMD
 * シリアルコマンドアスキー形式の入力系列の解釈を行う。\n
 * - :[0-9A-Z]+(CRLF) 系列の入力を行い、LRCチェックを行う
 *
 * @param pCmd 管理構造体
 * @param u8byte 入力文字
 * @return 状態コード (teModbusCmdState 参照)
 */
static uint8 SerCmdAscii_u8Parse(TWESERCMD_tsSerCmd_Context *pCmd, uint8 u8byte) {
	// check for timeout
	if (pCmd->u16timeout && pCmd->u8state != E_TWESERCMD_ASCII_CMD_EMPTY) {
		if (u32TickCount_ms - pCmd->u32timestamp > pCmd->u16timeout) {
			pCmd->u8state = E_TWESERCMD_ASCII_CMD_EMPTY; // start from new
		}
	}

	// check for complete or error status
	if (pCmd->u8state >= 0x80) {
		pCmd->u8state = E_TWESERCMD_ASCII_CMD_EMPTY;
	}

	// run state machine
	switch (pCmd->u8state) {
	case E_TWESERCMD_ASCII_CMD_EMPTY:
		if (u8byte == ':') {
			pCmd->u8state = E_TWESERCMD_ASCII_CMD_READPAYLOAD;

			pCmd->u32timestamp = u32TickCount_ms; // store received time for timeout
			pCmd->u16pos = 0;
			pCmd->u16cksum = 0;
			pCmd->u16len = 0;
			// memset(pCmd->au8data, 0, pCmd->u16maxlen);
		}
		break;

	case E_TWESERCMD_ASCII_CMD_READPAYLOAD:
		if ((u8byte >= '0' && u8byte <= '9') || (u8byte >= 'A' && u8byte <= 'F')) {
			/* オーバーフローのチェック */
			if (pCmd->u16pos / 2 == pCmd->u16maxlen) {
				pCmd->u8state = E_TWESERCMD_ASCII_CMD_ERROR;
				break;
			}

			/* 文字の16進変換 */
			const uint8 u8tbl[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15 };
			uint8 u8val = u8tbl[u8byte - '0'];

			/* バイナリ値として格納する */
			uint8 *p = &pCmd->au8data[pCmd->u16pos / 2];

			if (pCmd->u16pos & 1) {
				*p = (*p & 0xF0) | u8val;
				pCmd->u16cksum += *p;
			} else {
				*p = u8val << 4;
			}

			pCmd->u16pos++;
		} else if (u8byte == 0x0d || u8byte == 0x0a) { // CR入力
			if (pCmd->u16pos >= 4 && ((pCmd->u16pos & 1) == 0) // データ部１バイト、チェックサム１バイト以上
			) {
				// チェックサムの確認
				pCmd->u16cksum &= 0xFF; // チェックサムを 8bit に切り捨てる
				if (pCmd->u16cksum) { // 正しければ 0 になっているはず
					// 格納値
					uint8 u8lrc = pCmd->au8data[pCmd->u16pos / 2 - 1]; // u16posは最後のデータの次の位置
					// 計算値(二の補数の計算、ビット反転+1), デバッグ用に入力系列に対応する正しいLRCを格納しておく
					pCmd->u16cksum = (~(pCmd->u16cksum - u8lrc) + 1) & 0xFF;
					pCmd->u8state = E_TWESERCMD_ASCII_CMD_CHECKSUM_ERROR;
				} else {
					// LRCが正しければ、全部足したら 0 になる。
					pCmd->u8state = E_TWESERCMD_ASCII_CMD_COMPLETE; // 完了！
					pCmd->u16len = pCmd->u16pos / 2 - 1;
				}
			} else {
				pCmd->u8state = E_TWESERCMD_ASCII_CMD_ERROR;
			}
		} else if (u8byte == 'X') {
			// X で終端したらチェックサムの計算を省く
			if (pCmd->u16pos >= 2 && ((pCmd->u16pos & 1) == 0)) { // 入力データあり
				pCmd->u8state = E_TWESERCMD_ASCII_CMD_COMPLETE; // 完了！
				pCmd->u16len = pCmd->u16pos / 2;
			}
		} else {
			pCmd->u8state = E_TWESERCMD_ASCII_CMD_EMPTY;
		}
		break;

	default:
		break;
	}

	return pCmd->u8state;
}

/** @ingroup SERCMD
 * シリアルコマンドアスキー形式の出力補助関数。１バイトを１６進数２文字で出力する (0xA5 -> "A5")
 *
 * @param psSerStream 出力先
 * @param u8byte 出力したいバイト列
 * @param pu8lrc LRC計算用
 */
static inline void vPutByte(TWE_tsFILE *ps, uint8 u8byte, uint8 *pu8lrc) {
	static const uint8 au8tbl[] = "0123456789ABCDEF";
	uint8 u8OutH, u8OutL;

	u8OutH = au8tbl[u8byte >> 4];
	u8OutL = au8tbl[u8byte & 0x0F];

	TWE_fputc(u8OutH, ps);
	TWE_fputc(u8OutL, ps);

	if (pu8lrc) {
		*pu8lrc += u8byte;
	}
}

/*!
 * ASCII形式の出力 (汎用関数)
 *   ":00112233..." 形式で出力ストリーム ps に出力する
 * 
 * \param ps        出力先のストリーム
 * \param pbuf      データバッファ
 * \param u16len    データ長
 */
void TWESERCMD_Ascii_vOutput(TWE_tsFILE* ps, uint8* pbuf, uint16 u16len) {
	int i;
	uint8 u8lrc = 0;

	// 先頭の :
	TWE_fputc(':', ps);

	for (i = 0; i < u16len; i++) {
		vPutByte(ps, pbuf[i], &u8lrc);
	}

	// ２の補数の計算 (LRC)
	u8lrc = ~u8lrc + 1;
	vPutByte(ps, u8lrc, NULL);

	// Trailer byte
	TWE_fputc(0x0D, ps);
	TWE_fputc(0x0A, ps);
}

/*!
 * Ascii形式の vOutput 用メソッド
 * 
 * \param pc        TWESERCMD_tsSerCmd_Context構造体へのポインタ
 * \param ps        出力先のストリーム
 */
static void SerCmdAscii_Output(TWESERCMD_tsSerCmd_Context *pc, TWE_tsFILE *ps) {
	TWESERCMD_Ascii_vOutput(ps, pc->au8data, pc->u16len);
}

/*!
 * Ascii 形式の解析構造体を初期化する。
 * 
 * \param pc        TWESERCMD_tsSerCmd_Context構造体へのポインタ
 * \param pbuff     事前確保済みの内部用のバッファ
 * \param u16maxlen バッファ長
 */
void TWESERCMD_Ascii_vInit(TWESERCMD_tsSerCmd_Context *pc, uint8 *pbuff, uint16 u16maxlen) {
	memset(pc, 0, sizeof(TWESERCMD_tsSerCmd_Context));

	pc->au8data = pbuff;
	pc->u16maxlen = u16maxlen;

	pc->u8Parse = SerCmdAscii_u8Parse;
	pc->vOutput = SerCmdAscii_Output;
}
