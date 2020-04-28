/* Copyright (C) 2017,2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "twecommon.h"
#include "tweserial.h"
#include "twesercmd_plus3.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

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

/** @ingroup MBUSA
 * Modbus ASCII 入力系列の解釈を行う。\n
 * - :[0-9A-Z]+(CRLF) 系列の入力を行い、LRCチェックを行う
 * - + + + を入力したときに Verbose モードの判定を行う
 *
 * @param pCmd 管理構造体
 * @param u8byte 入力文字
 * @return 状態コード (teModbusCmdState 参照)
 */
uint8 TWESERCMD_Plus3_u8Parse(TWESERCMD_tsPlus3_Context *pCmd, uint8 u8byte) {
	// check for complete or error status
	if (pCmd->u8state >= 0x80) {
		pCmd->u8state = E_TWESERCMD_PLUS3_EMPTY;
	}

	// run state machine
	switch (pCmd->u8state) {
	case E_TWESERCMD_PLUS3_EMPTY:
		if (u8byte == '+') {
			// press "+ (pause) + (pause) +" to toggle verbose mode.
			pCmd->u8state = E_TWESERCMD_PLUS3_PLUS1;
			pCmd->u32timestamp = u32TickCount_ms; // store received time for timeout
		}
		break;

	case E_TWESERCMD_PLUS3_PLUS1: // second press of '+'
		if ((u8byte == '+') && (u32TickCount_ms - pCmd->u32timestamp > 200) && (u32TickCount_ms - pCmd->u32timestamp < 1000)) {
			pCmd->u8state = E_TWESERCMD_PLUS3_PLUS2;
			pCmd->u32timestamp = u32TickCount_ms;
		} else {
			pCmd->u8state = E_TWESERCMD_PLUS3_ERROR;
		}
		break;

	case E_TWESERCMD_PLUS3_PLUS2: // third press of '+'
		if ((u8byte == '+') && (u32TickCount_ms - pCmd->u32timestamp > 200) && (u32TickCount_ms - pCmd->u32timestamp < 1000)) {
			pCmd->bverbose = pCmd->bverbose ? FALSE : TRUE;
			if (pCmd->bverbose) {
				pCmd->u32timevbs = u32TickCount_ms;
				pCmd->u8state = E_TWESERCMD_PLUS3_VERBOSE_ON;
			} else {
				pCmd->u8state = E_TWESERCMD_PLUS3_VERBOSE_OFF;
			}
		} else {
			pCmd->u8state = E_TWESERCMD_PLUS3_ERROR;
		}
		break;

	default:
		break;
	}

	return pCmd->u8state;
}
