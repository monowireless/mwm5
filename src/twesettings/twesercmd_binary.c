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
#define TWESERCMD_SYNC_1 0xA5
#define TWESERCMD_SYNC_2 0x5A

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/** @ingroup SERCMD
 * シリアルコマンドバイナリ形式の状態定義
 */
typedef enum {
	E_SERCMD_BINARY_EMPTY = 0,      //!< E_SERCMD_BINARY_EMPTY
	E_SERCMD_BINARY_READSYNC,       //!< E_SERCMD_BINARY_READSYNC
	E_SERCMD_BINARY_READLEN,        //!< E_SERCMD_BINARY_READLEN
	E_SERCMD_BINARY_READLEN2,       //!< E_SERCMD_BINARY_READLEN2
	E_SERCMD_BINARY_READPAYLOAD,    //!< E_SERCMD_BINARY_READPAYLOAD
	E_SERCMD_BINARY_READCRC,        //!< E_SERCMD_BINARY_READCRC
	E_SERCMD_BINARY_PLUS1,          //!< E_SERCMD_BINARY_PLUS1
	E_SERCMD_BINARY_PLUS2,          //!< E_SERCMD_BINARY_PLUS2
	E_SERCMD_BINARY_COMPLETE = 0x80,//!< E_SERCMD_BINARY_COMPLETE
	E_SERCMD_BINARY_ERROR = 0x81,   //!< E_SERCMD_BINARY_ERROR
	E_SERCMD_BINARY_CRCERROR = 0x82 //!< E_SERCMD_BINARY_CRCERROR
} TWESERCMD_Binary_teState;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************///

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: u8ProcessSerCmd
 *
 * DESCRIPTION: Process incoming byte to form binary input sequence.
 *   format:
 *     +0 +1 +2  +3 +4...4+len-1 4+len
 *     A5 5A lenH,L payload      XOR
 *
 *   timeout:
 *     the timestamp of first byte is recorded by reffering external value
 *     u32TickCount_ms, then calucalted elapsed time from starting byte.
 *     if the elapsed time excess SERCMD_TIMEOUT_ms, the coming byte is
 *     considered as first byte.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   state information is returned. once E_SERCMD_BINARY_COMPLETE is returned,
 *   the input data shall be handled before calling this function.
 *   the next call will set tsSerCmd structure as new status.
 *
 *     - E_SERCMD_BINARY_EMPTY : new state (no input)
 *     - E_SERCMD_BINARY_ERROR : some error
 *     - E_SERCMD_BINARY_READCRC : CRC error
 *     - E_SERCMD_BINARY_COMPLETE : complete to input a command
 *
 * NOTES:
 ****************************************************************************/
#if defined(ESP32)
extern uint32_t _u32GetTick_ms();
#define u32TickCount_ms _u32GetTick_ms()
#else
extern volatile uint32 u32TickCount_ms; //!< ToCoNet での TickTimer
#endif

/** @ingroup SERCMD
 * シリアルコマンド系列の解釈を行う状態遷移マシン
 *
 * @param pCmd
 * @param u8byte
 * @return
 */
static uint8 SerCmdBinary_u8Parse(TWESERCMD_tsSerCmd_Context *pCmd, uint8 u8byte) {
	// check for timeout
	if (pCmd->u16timeout && pCmd->u8state != E_SERCMD_BINARY_EMPTY) {
		if (u32TickCount_ms - pCmd->u32timestamp > pCmd->u16timeout) {
			pCmd->u8state = E_SERCMD_BINARY_EMPTY; // start from new
		}
	}

	// check for complete or error status
	if (pCmd->u8state >= 0x80) {
		pCmd->u8state = E_SERCMD_BINARY_EMPTY;
	}

	// run state machine
	switch (pCmd->u8state) {
	case E_SERCMD_BINARY_EMPTY:
		if (u8byte == TWESERCMD_SYNC_1) {
			pCmd->u8state = E_SERCMD_BINARY_READSYNC;
			pCmd->u32timestamp = u32TickCount_ms; // store received time
		}
		break;

	case E_SERCMD_BINARY_READSYNC:
		if (u8byte == TWESERCMD_SYNC_2) {
			pCmd->u8state = E_SERCMD_BINARY_READLEN;
		} else {
			pCmd->u8state = E_SERCMD_BINARY_ERROR;
		}
		break;

	case E_SERCMD_BINARY_READLEN:
		if (u8byte & 0x80) {
			// long length mode (1...
			u8byte &= 0x7F;
			pCmd->u16len = u8byte;
			if (pCmd->u16len <= pCmd->u16maxlen) {
				pCmd->u8state = E_SERCMD_BINARY_READLEN2;
			} else {
				pCmd->u8state = E_SERCMD_BINARY_ERROR;
			}
		} else {
			// short length mode (1...127bytes)
			pCmd->u16len = u8byte;
			if (pCmd->u16len <= pCmd->u16maxlen) {
				pCmd->u8state = E_SERCMD_BINARY_READPAYLOAD;
				pCmd->u16pos = 0;
				pCmd->u16cksum = 0;
			} else {
				pCmd->u8state = E_SERCMD_BINARY_ERROR;
			}
		}
		break;

	case E_SERCMD_BINARY_READLEN2:
		pCmd->u16len = (pCmd->u16len * 256) + u8byte;
		if (pCmd->u16len <= pCmd->u16maxlen) {
			pCmd->u8state = E_SERCMD_BINARY_READPAYLOAD;
			pCmd->u16pos = 0;
			pCmd->u16cksum = 0;
		} else {
			pCmd->u8state = E_SERCMD_BINARY_ERROR;
		}
		break;

	case E_SERCMD_BINARY_READPAYLOAD:
		pCmd->au8data[pCmd->u16pos] = u8byte;
		pCmd->u16cksum ^= u8byte; // update XOR checksum
		if (pCmd->u16pos == pCmd->u16len - 1) {
			pCmd->u8state = E_SERCMD_BINARY_READCRC;
		}
		pCmd->u16pos++;
		break;

	case E_SERCMD_BINARY_READCRC:
		pCmd->u16cksum &= 0xFF;
		if (u8byte == pCmd->u16cksum) {
			pCmd->u8state = E_SERCMD_BINARY_COMPLETE;
		} else {
			pCmd->u8state = E_SERCMD_BINARY_CRCERROR;
		}
		break;

	default:
		break;
	}

	return pCmd->u8state;
}


/*!
 * Binary形式の出力 (汎用関数)
 *   ":00112233..." 形式で出力ストリーム ps に出力する
 *
 * \param ps        出力先のストリーム
 * \param pbuf      データバッファ
 * \param u16len    データ長
 */
void TWESERCMD_Binary_vOutput(TWE_tsFILE* ps, uint8 *pbuf, uint16 u16len) {
	int i;
	uint8 u8xor = 0;

	TWE_fputc(TWESERCMD_SYNC_1, ps);
	TWE_fputc(TWESERCMD_SYNC_2, ps);
	TWE_fputc((uint8)(0x80 | (u16len >> 8)), ps);
	TWE_fputc((u16len & 0xff), ps);

	for (i = 0; i < u16len; i++) {
		u8xor ^= pbuf[i];
		TWE_fputc(pbuf[i], ps);
	}

	TWE_fputc(u8xor, ps); // XOR check sum
	TWE_fputc(0x4, ps); // EOT
}

/*!
 * Binary形式の vOutput 用メソッド
 *
 * \param pc        TWESERCMD_tsSerCmd_Context構造体へのポインタ
 * \param ps        出力先のストリーム
 */
static void SerCmdBinary_Output(TWESERCMD_tsSerCmd_Context* pc, TWE_tsFILE* ps) {
	TWESERCMD_Binary_vOutput(ps, pc->au8data, pc->u16len);
}

/*!
 * Binary 形式の解析構造体を初期化する。
 *
 * \param pc        TWESERCMD_tsSerCmd_Context構造体へのポインタ
 * \param pbuff     事前確保済みの内部用のバッファ
 * \param u16maxlen バッファ長
 */
void TWESERCMD_Binary_vInit(TWESERCMD_tsSerCmd_Context *pc, uint8 *pbuff, uint16 u16maxlen) {
	memset(pc, 0, sizeof(TWESERCMD_tsSerCmd_Context));

	pc->au8data = pbuff;
	pc->u16maxlen = u16maxlen;

	pc->u8Parse = SerCmdBinary_u8Parse;
	pc->vOutput = SerCmdBinary_Output;
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
