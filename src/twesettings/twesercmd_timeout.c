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

#define IS_TRANSMIT_CHAR(c) (c=='\t' || ((c)>=0x20 && (c)!=0x7F)) // 送信対象文字列か判定

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/** @ingroup SERCMD
 * シリアルコマンドの状態定義
 */
typedef enum {
	E_TWESERCMD_TIMEOUT_CMD_EMPTY = 0,      //!< 入力されていない
	E_TWESERCMD_TIMEOUT_CMD_READPAYLOAD,    //!< E_TWESERCMD_TIMEOUT_CMD_READPAYLOAD
	E_TWESERCMD_TIMEOUT_CMD_COMPLETE = 0x80,//!< 入力が完結した(LCRチェックを含め)
	E_TWESERCMD_TIMEOUT_CMD_ERROR = 0x81,          //!< 入力エラー
} TWESRCMD_Timeout_teState;

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
 * 入力系列の解釈を行う。
 * ※ 完了条件はタイムアウトで、bComplete() 関数の呼び出しによりチェックされる。
 *
 * @param pCmd 管理構造体
 * @param u8byte 入力文字
 * @return 状態コード (teModbusCmdState 参照)
 */
static uint8 SerCmdTimeout_u8Parse(TWESERCMD_tsSerCmd_Context *pCmd, uint8 u8byte) {
	// check for complete or error status
	if (pCmd->u8state >= 0x80) {
		pCmd->u8state = E_TWESERCMD_TIMEOUT_CMD_EMPTY;
	}

	// run state machine
	switch (pCmd->u8state) {
	case E_TWESERCMD_TIMEOUT_CMD_EMPTY:
		{
			pCmd->u8state = E_TWESERCMD_TIMEOUT_CMD_READPAYLOAD;
			pCmd->au8data[0] = u8byte;

			pCmd->u32timestamp = u32TickCount_ms; // store received time for timeout
			pCmd->u16pos = 1;
			pCmd->u16len = 1;
			pCmd->u16cksum = 0;
			// memset(pCmd->au8data, 0, pCmd->u16maxlen);
		}
		break;

	case E_TWESERCMD_TIMEOUT_CMD_READPAYLOAD:
		{
			pCmd->au8data[pCmd->u16pos] = u8byte;

			pCmd->u16pos++;
			pCmd->u16len++;
			pCmd->u32timestamp = u32TickCount_ms; // 入力ごとにタイムアウトを設定していく

			// 最大長のエラー
			if (pCmd->u16pos == pCmd->u16maxlen - 1) {
				pCmd->u8state = E_TWESERCMD_TIMEOUT_CMD_ERROR;
			}
		}
		break;

	default:
		break;
	}

	return pCmd->u8state;
}

/** @ingroup SERCMD
 * 出力 (vOutputメソッド用)
 * - そのまま系列を出力する。
 *
 * @param pc
 * @param ps
 */
static void SerCmdTimeout_Output(TWESERCMD_tsSerCmd_Context *pc, TWE_tsFILE *ps) {
	int i;

	for (i = 0; i < pc->u16len; i++) {
		TWE_fputc(pc->au8data[i], ps);
	}
}


/** @ingroup SERCMD
 * 完了チェック
 *
 */
static bool_t SerCmdTimeout_bComplete(TWESERCMD_tsSerCmd_Context *pc) {
	bool_t bRet = FALSE;

	// タイムアウトは常に無効
	if (pc->u16timeout == 0) {
		return FALSE;
	}

	if (pc->u8state == E_TWESERCMD_TIMEOUT_CMD_READPAYLOAD) {
		if (u32TickCount_ms - pc->u32timestamp > pc->u16timeout) {
			// 100ms 経過したら確定
			pc->u8state = E_TWESERCMD_TIMEOUT_CMD_COMPLETE;
		}
	}

	if (pc->u8state == E_TWESERCMD_TIMEOUT_CMD_COMPLETE) {
		bRet = TRUE;
	}

	return bRet;
}

/** @ingroup SERCMD
 * 解析構造体を初期化する。
 *
 * @param pc
 * @param pbuff
 * @param u16maxlen
 */
void SerCmdTimeout_vInit(TWESERCMD_tsSerCmd_Context *pc, uint8 *pbuff, uint16 u16maxlen) {
	memset(pc, 0, sizeof(TWESERCMD_tsSerCmd_Context));

	pc->au8data = pbuff;
	pc->u16maxlen = u16maxlen;

	pc->u8Parse = SerCmdTimeout_u8Parse;
	pc->vOutput = SerCmdTimeout_Output;
	pc->bComplete = SerCmdTimeout_bComplete;
}

