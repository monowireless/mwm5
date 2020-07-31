/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <string.h>
#include "twecommon.h"
#include "twesettings0.h"
#include "twesettings_std.h"
#include "twesettings_validator.h"

#include "tweinteractive.h"
#include "tweinteractive_keycode.h"

#include "twesettings_weak.h"


#if defined(ESP32)
extern uint32_t _u32GetTick_ms();
#define u32TickCount_ms _u32GetTick_ms()
#elif defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)
extern volatile uint32 u32TickCount_ms;
#else
extern volatile uint32 u32TickCount_ms; //!< ToCoNet での TickTimer
#endif

// static uint16 u16HoldUpdateScreen; //!< 画面更新のためのカウンタ
static uint16 u16lastupd_ms; //!< 画面更新の最後のアップデートタイミング

//static uint8 u8RevertStatus; //!< 設定の再初期化 0:通常 1:セーブデータ 2:デーブデータ破棄

static TWESERCMD_tsPlus3_Context sSerCmd_P3; //!< + + + を処理するパーサー
static TWEINPSTR_tsInpStr_Context sSerInpStr; //!< １行入力処理を行う

static TWEINTRCT_tsContext sIntrct_Context, *psIntr = &sIntrct_Context; //! インタラクティブモードのコンテキスト

static void(*pf_vProcessInputByte)(TWEINTRCT_tsContext *, int16) = NULL; //!< シリアルパーサー

bool_t (*TWEINTRCT_pfbCbUnHandledCommand)(TWEINTRCT_tsContext *, int16, int16) = NULL; //!< インタラクティブモードの未処理コマンドのコールバック

/*!
 * インタラクティブモードの初期化
 * 
 * \param psFinal             確定設定リスト(tsFinal)
 * \param pSerCmd             シリアルコマンド
 * \param fp                  入出力ストリーム
 * \param pfProcessInputByte  通常のシリアル通常処理用の処理関数（アプリケーション定義）, NULL の場合はインタラクティブモードのみ動作
 */
TWEINTRCT_tsContext* TWEINTRCT_pscInit(TWESTG_tsFinal *psFinal, TWESERCMD_tsSerCmd_Context *pSerCmd, TWE_tsFILE *fp, void *pfProcessInputByte, const TWEINTRCT_tsFuncs *pFuncs) {
	memset(psIntr, 0, sizeof(TWEINTRCT_tsContext));

	psIntr->psFinal = psFinal;
	psIntr->pSerCmd = pSerCmd;
	psIntr->pSerInpStr = &sSerInpStr;
	psIntr->pStream = fp;
	psIntr->pFuncs = pFuncs;
	pf_vProcessInputByte = pfProcessInputByte;

	// メニュー数をカウントする
	while(pFuncs[psIntr->u8screen_max].u8MenuId != 0xFF) {
		psIntr->u8screen_max++;
	}

	// １行入力の初期化
	TWEINPSTR_vInit(&sSerInpStr, psIntr->pStream);

	// +++ 判定用
	memset(&sSerCmd_P3, 0x00, sizeof(TWESERCMD_tsPlus3_Context)); // ＋＋＋判定、ゼロクリアだけでOK

	return psIntr;
}

/*!
 * オプション等の再コンフィグを行う
 * 
 * \param _psIntr
 */
void TWEINTRCT_vReConf(TWEINTRCT_tsContext* _psIntr) {
	if (_psIntr->config.u8Mode == 1) {
		TWEINTRCT_bSetInteractive(_psIntr, TRUE);
	}
}


/*!
 * @brief parse keysequence (e.g. ESC seq, ¥r¥n)
 *        When completed, this function will return TRUE.
 *        The converted keycode(s) are stored in psIntr->keyseq[]
 *        with the lenght of psIntr->u8keyseq_count.
 * 
 * @param psIntr  the context of Interactive Mode
 * @param keycode the coming keycode
 * @param tick    tick count of now (last 16bit of ms based tick count)
 * 
 * @returns TRUE:the sequence is completed FALSE:waiting more data.
 */
static bool_t s_decode_keysequence(TWEINTRCT_tsContext* psIntr, TWEINTRCT_tkeycode keycode, uint16 tick) {
	// first byte
	if (keycode != -1) {
		psIntr->keyseq[psIntr->u8keyseq_count] = keycode;
		psIntr->u16keyseq_tick = tick;
		psIntr->u8keyseq_count++;
	} else {
		; // timeout check
	}

	// timeout, put a sequence as-is.
	#ifdef _DEBUG
	#define KEYSEQ_TIMEOUT 100 // 2000
	#else
	#define KEYSEQ_TIMEOUT 100
	#endif
	if (psIntr->u8keyseq_count >= 1 && (tick - psIntr->u16keyseq_tick > KEYSEQ_TIMEOUT)) {
		return TRUE;
	}

	// not timeout
	if (keycode == -1) return FALSE;

	// parsing
	if (psIntr->u8keyseq_count == 1) {
		switch(keycode) {
			case _TWEESC: return FALSE; // expect [
			case 0x0d: return FALSE; // expect 0x0a
			default: return TRUE;
		}
	} else if (psIntr->u8keyseq_count == 2) {
		switch(keycode) {
			case '[': return FALSE; // expect A..D
			case 0x0a: psIntr->keyseq[0] = _TWECR; psIntr->u8keyseq_count = 1; return TRUE;
			default: return TRUE;
		}
	} else if (psIntr->u8keyseq_count == 3) {
		switch(keycode) {
			case 'A': psIntr->keyseq[0] = TWEINTRCT_KEY_UP; psIntr->u8keyseq_count = 1; return TRUE;
			case 'B': psIntr->keyseq[0] = TWEINTRCT_KEY_DOWN; psIntr->u8keyseq_count = 1; return TRUE;
			case 'C': psIntr->keyseq[0] = TWEINTRCT_KEY_RIGHT; psIntr->u8keyseq_count = 1; return TRUE;
			case 'D': psIntr->keyseq[0] = TWEINTRCT_KEY_LEFT; psIntr->u8keyseq_count = 1; return TRUE;
			default: return TRUE;
		}
	} else {
		return TRUE;
	}
}

/*!
 * シリアル入力チェック
 * 
 * 本関数は以下のタイミングで呼び出すこと。
 * - 割り込み等が発生したタイミング
 * - 定周期タイマー発生時
 * 
 */
void TWEINTRCT_vHandleSerialInput() {
	// シリアル画面制御のためのカウンタを減算
	uint16 u16t = u32TickCount_ms & 0xffff;
	if ((uint16)(u16t - u16lastupd_ms) >= 16) {
		if (sSerCmd_P3.bverbose && psIntr->u16HoldUpdateScreen) {
			if (!(--psIntr->u16HoldUpdateScreen)) {
				psIntr->pFuncs[psIntr->u8screen].pf_vSerUpdateScreen(psIntr);
			}
		}
		u16lastupd_ms = u16t;
	}

	/* UARTポートの処理 */
	while (1) {
		TWEINTRCT_tkeycode keycode = TWE_fgetc(psIntr->pStream);

		// timeout check.
		if (keycode == -1) {
			if (psIntr->u8keyseq_count > 0) {
				if (!s_decode_keysequence(psIntr, -1, u16t)) {
					break;
				}
			} else {
				break;
			}
		}

		// parse keysequence
		if (psIntr->pSerCmd && psIntr->pSerCmd->u8state > 0) {
			// now in parser mode...
			psIntr->keyseq[0] = keycode;
			psIntr->u8keyseq_count = 1;
		} else {
			if (!s_decode_keysequence(psIntr, keycode, u16t)) continue;
		}

		// process
		int i_keyseq = 0;
		const int i_keyseq_ct = psIntr->u8keyseq_count;
		psIntr->u8keyseq_count = 0; // reset the var
		for (i_keyseq = 0; i_keyseq < i_keyseq_ct; i_keyseq++) { // i16Char >= 0 && i16Char <= 0xFF) { // skip this check (for special key handling)
			TWEINTRCT_tkeycode i16Char = psIntr->keyseq[i_keyseq];
			//DBGOUT(0, "[%02x]", i16Char);

			// check the special chars.
			bool_t b_special = FALSE;
			if (i16Char < 0 || i16Char > 255) b_special = TRUE;

			if (INPSTR_bActive(&sSerInpStr) && !b_special) { // so far, skip special chars
				// 文字列入力モード
				if (i16Char >= 0 && i16Char <= 0xFF) {
					uint8 u8res = TWEINPSTR_u8InputByte(&sSerInpStr, (uint8)i16Char);

					if (u8res == E_TWEINPUTSTRING_STATE_COMPLETE) {
						psIntr->pFuncs[psIntr->u8screen].pf_vProcessInputString(psIntr, &sSerInpStr);
					}
					else if (u8res == E_TWEINPUTSTRING_STATE_CANCELED) {
						TWE_fprintf(psIntr->pStream, "(canceled)");
						psIntr->u16HoldUpdateScreen = 64;
					}
				}
				continue;
			}

			if (!b_special) {
				// コマンド書式の系列解釈、および verbose モードの判定
				uint8 u8res;

				u8res = TWESERCMD_Plus3_u8Parse(&sSerCmd_P3, (uint8)i16Char);

				if (u8res != E_TWESERCMD_PLUS3_EMPTY) {
					if (u8res == E_TWESERCMD_PLUS3_VERBOSE_ON) {
						// verbose モードの判定があった
						TWEINTRCT_bSetInteractive(psIntr, TRUE);
						continue;
					}

					if (u8res == E_TWESERCMD_PLUS3_VERBOSE_OFF) {
						TWEINTRCT_bSetInteractive(psIntr, FALSE);
						continue;
					}
				}
				else {
					; // コマンド解釈モードではない
				}
			}

			if (sSerCmd_P3.bverbose) {
				// Verbose モード=インタラクティブモードのときは、s_vProcessInputByte()にて入力系列を処理する。
				psIntr->pFuncs[psIntr->u8screen].pf_vProcessInputByte(psIntr, i16Char);

				if (psIntr->config.u8AlwaysKeyReport && pf_vProcessInputByte) {
					pf_vProcessInputByte(psIntr, i16Char);
				}
			}
			else {
				if (pf_vProcessInputByte) {
					bool_t bCallBack = FALSE;
					if (psIntr->pSerCmd) {
						// SERCMD の解釈を行う
						if(psIntr->config.u8OptSerCmd & 0x01) TWE_fputc(i16Char, psIntr->pStream); // エコーバック

						uint8 u8stat = psIntr->pSerCmd->u8Parse(psIntr->pSerCmd, (uint8)i16Char);
						if (u8stat & 0x80) {
							bCallBack = TRUE;
						}
					}
					else {
						// pSerCmd が NULL の場合は、毎回コールバックする
						bCallBack = TRUE;
					}

					// コールバック
					if ((bCallBack || psIntr->config.u8AlwaysKeyReport) && pf_vProcessInputByte) {
						pf_vProcessInputByte(psIntr, i16Char);
					}
				}
			}
		}


	}
}

/*!
 * インタラクティブモードで表示される先頭行を表示する
 * 
 * \param psIntr インタラクティブモードの管理構造体
 * \param u32Op 表示オプション(現時点では未実装）
 */
void TWEINTRCT_vSerHeadLine(TWEINTRCT_tsContext *psIntr, uint32 u32Op) {
	TWE_fputs("\033[2J\033[H", psIntr->pStream); // CLEAR SCREEN
	if(psIntr->pStreamTitle) TWE_fputs("\033[2J\033[H", psIntr->pStreamTitle); // CLEAR TITLE BAR

	TWE_tsFILE *pStream = psIntr->pStreamTitle ? psIntr->pStreamTitle : psIntr->pStream;

	uint8 strAppName[32] = "", *pstrAppName = strAppName;
	uint8 strKindName[32] = "", *pstrKindName = strKindName;
	uint8 strSlotName[32] = "", *pstrSlotName = strSlotName;
	uint8 strOptMsg[32] = "", *pstrOptMsg = strOptMsg;
	uint32 u32Sid = 0x0;

	// 先頭行に必要な情報の取り出し
	TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_GET_APPNAME, 0, 0, &pstrAppName);
	TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_GET_KINDNAME, psIntr->psFinal->u8Kind, psIntr->psFinal->u8Slot, &pstrKindName);
	TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_GET_SLOTNAME, psIntr->psFinal->u8Kind, psIntr->psFinal->u8Slot, &pstrSlotName);
	TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINTCT_OP_GET_SID, 0, 0, &u32Sid);
	TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINTCT_OP_GET_OPTMSG, 0, 0, &pstrOptMsg);

	// TITLE
	TWE_fprintf(pStream, "[%s%s%s"
		, psIntr->pFuncs[psIntr->u8screen].pu8MenuString
		, pstrAppName[0] == '\0' ? "" : "/"
		, pstrAppName
		);

	if (u32Op & 0x01) {
		// KIND
		if (pstrKindName && pstrKindName[0] != '\0') {
			TWE_fprintf(pStream, ":%s", pstrKindName);
		}

		// SLOT
		if (pstrSlotName && pstrSlotName[0] != '\0') {
			TWE_fprintf(pStream, ":%s", pstrSlotName);
		} else {
			TWE_fprintf(pStream, ":%d", psIntr->psFinal->u8Slot);
		}
	}

	// OPT MSG
	if (pstrOptMsg && pstrOptMsg[0] != '\0') {
		TWE_fputc('/', pStream);
		TWE_fputs((char*)pstrOptMsg, pStream);
	}

	TWE_fputs("]", pStream);
	TWE_fflush(pStream);
}

/*!
 * インタラクティブモードで表示される末尾行を表示する
 * 
 * \param psIntr インタラクティブモードの管理構造体
 * \param u32Op 表示オプション(現時点では未実装）
 */
void TWEINTRCT_vSerFootLine(TWEINTRCT_tsContext *pContext, uint32 u32Op) {
	//TWE_fprintf(psIntr->pStream, "%d/%d", psIntr->u8screen, psIntr->u8screen_max); // TODO
	TWE_fputs(_TWET_INV "\033[G\033[K", psIntr->pStream);
	TWE_fputs(" [ESC]:Back", psIntr->pStream);
	if (!pContext->config.u8NoSysReset) {
		TWE_fputs(" [!]:Reset System", psIntr->pStream);
	}
	if (pContext->config.u8screen_default != 0 && pContext->u8screen != 0) {
		TWE_fputs(" [M]:Extr Menu", psIntr->pStream);
	}
	TWE_fputs(_TWET_RST "\033[G", psIntr->pStream);
}

/*!
 * インタラクティブモードのメニュー操作の共通処理
 * 
 * \param psIntr インタラクティブモードの管理構造体
 * \param u8Key 入力キー
 * \return SUCCESS:処理された FAIL:処理されない
 */
TWE_APIRET TWEINTRCT_u32MenuOpKey(TWEINTRCT_tsContext *pContext, TWEINTRCT_tkeycode keycode) {
	TWE_APIRET ret = TWE_APIRET_FAIL;

	switch(keycode) {
	case _TWELF: // LF
		ret = TWE_APIRET_SUCCESS_W_VALUE(0);
		break;
    case _TWECR: // CR
	case _TWECLR: // Ctrl+L
		// 画面の書き換え
		ret = TWE_APIRET_SUCCESS_W_VALUE(1);
		break;

	case '!': // MODULE RESET
		if(!psIntr->config.u8NoSysReset) {
			TWE_fprintf(psIntr->pStream, _TWELB"!INF RESET SYSTEM.");
			TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_WAIT, 100, 0, NULL);
			TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_RESET, 0, 0, NULL);
		}
		ret = TWE_APIRET_SUCCESS_W_VALUE(1);
		break;
	
	case 'M':
		TWEINTRCT_u32MenuChange(psIntr, 0);
		ret = TWE_APIRET_SUCCESS_W_VALUE(1);
		break;

	// case _TWEBS:
	case _TWEESC:
		if (psIntr->u8screen == psIntr->config.u8screen_default) {
			TWEINTRCT_bSetInteractive(psIntr, FALSE);
			ret = TWE_APIRET_SUCCESS_W_VALUE(1);
		} else {
			TWEINTRCT_u32MenuChange(psIntr, psIntr->config.u8screen_default);
			ret = TWE_APIRET_SUCCESS_W_VALUE(1);
		}
		break;
	}

	return ret;
}

/**
 * @brief メニュー切り替えを行う。同時にアプリケーションにもコールバックで切り替えを伝達する。
 * 
 * 
 * @param pContext 
 * @param u8MenuIdx 
 * @return TWE_APIRET 
 */
TWE_APIRET TWEINTRCT_u32MenuChange(TWEINTRCT_tsContext *psIntr, uint8 u8NewScreen) {
	if (u8NewScreen < psIntr->u8screen_max) {
		psIntr->u8screen = u8NewScreen;
		if (psIntr->pFuncs[psIntr->u8screen].pf_u32ProcessMenuEvent != NULL) {
			TWE_APIRET apiRet = psIntr->pFuncs[psIntr->u8screen].pf_u32ProcessMenuEvent(psIntr, E_TWEINTCT_MENU_EV_LOAD, psIntr->u8screen, 0UL, NULL);
			(void)apiRet;
		}
		return TWE_APIRET_SUCCESS_W_VALUE(u8NewScreen);
	} else {
		return TWE_APIRET_FAIL;
	}
}

/**
 * @brief Query if entering interactive mode.
 * 
 * @return bool_t TRUE:entering
 */

bool_t TWEINTRCT_bGetInteractive(TWEINTRCT_tsContext* psIntr) {
	return sSerCmd_P3.bverbose;
}

/**
 * @brief Enter/Exit interactive mode.
 * 
 * @return bool_t TRUE:entering
 */
bool_t TWEINTRCT_bSetInteractive(TWEINTRCT_tsContext* psIntr, bool_t bEnter) {
	if (bEnter) {
		sSerCmd_P3.bverbose = TRUE;
		TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINTCT_OP_ENTER, psIntr->u8screen, 0, NULL);
		TWEINTRCT_u32MenuChange(psIntr, psIntr->config.u8screen_default); // back to root menu
		if (psIntr->u16HoldUpdateScreen == 0) {
			psIntr->pFuncs[psIntr->u8screen].pf_vSerUpdateScreen(psIntr);
		}

		return TRUE;
	} else {
		sSerCmd_P3.bverbose = FALSE;
		TWE_fprintf(psIntr->pStream, "!INF EXIT INTERACTIVE MODE."_TWELB);
		TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINTCT_OP_EXIT, psIntr->u8screen | 0x80, 0, NULL);

		return FALSE;
	}
}

/**
 * @brief 現在インタラクティブモードかどうかを返す
 * 
 * @return bool_t
 */
bool_t TWEINTRCT_bIsVerbose() {
	return sSerCmd_P3.bverbose;
}
