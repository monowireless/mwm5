/* Copyright (C) 2017,2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/** @file
 *
 * @defgroup SERCMD_PLUS3 プラス入力３回の判定を行う
 *
 */

#ifndef TWESERCMD_PLUS3_H_
#define TWESERCMD_PLUS3_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @ingroup SERCMD_PLUS3
 * 内部状態
 */
typedef enum {
	E_TWESERCMD_PLUS3_EMPTY = 0,      //!< 入力されていない
	E_TWESERCMD_PLUS3_PLUS1,          //!< E_PLUS3_CMD_PLUS1
	E_TWESERCMD_PLUS3_PLUS2,          //!< E_PLUS3_CMD_PLUS2
	E_TWESERCMD_PLUS3_ERROR = 0x81,   //!< エラー状態
	E_TWESERCMD_PLUS3_VERBOSE_OFF = 0x90,     //!< verbose モード ON になった
	E_TWESERCMD_PLUS3_VERBOSE_ON     //!< verbose モード OFF になった
} TWESERCMD_tePlus3CmdState;

/** @ingroup SERCMD_PLUS3
 * 管理構造体
 */
typedef struct {
	uint32 u32timestamp; //!< タイムアウトを管理するためのカウンタ
	uint8 u8state; //!< 状態

	bool_t bverbose; //!< TRUE: verbose モード
	uint32 u32timevbs; //!< verbose モードになったタイムスタンプ
} TWESERCMD_tsPlus3_Context;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

uint8 TWESERCMD_Plus3_u8Parse(TWESERCMD_tsPlus3_Context *pCmd, uint8 u8byte);
#define TWESERCMD_Plus3_bVerbose(pCmd) (pCmd->bverbose) //!< VERBOSEモード判定マクロ @ingroup MBUSA


#ifdef __cplusplus
}
#endif


#endif /* MODBUS_ASCII_H_ */
