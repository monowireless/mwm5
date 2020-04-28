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

/*!
 * インタラクティブモードで画面クリアと設定の全表示を行う
 */
void TWEINTCT_vSerUpdateScreen_defmenus(TWEINTRCT_tsContext *psIntr) {
	TWEINTRCT_vSerHeadLine(psIntr, 0UL);
	TWE_fputs(_TWELB, psIntr->pStream);

	// u8DefMenusSlots>0 設定時は psIntr->pFuncs[1] が設定メニューである前提とする
	if (psIntr->config.u8DefMenusSlots) {
		TWE_APIRET apiRet = TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINTCT_OP_GET_MAX_SLOT, 0, 0, NULL);
		psIntr->config.u8DefMenusSlots = apiRet & 0x00FF;

		int i;
		for (i = 0; i <= psIntr->config.u8DefMenusSlots; i++) {
			uint8 strSlotName[32] = "", * pstrSlotName = strSlotName;

			// acquire slot name
			TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_GET_SLOTNAME, psIntr->psFinal->u8Kind, i, &pstrSlotName);

			// print slot
			TWE_fprintf(psIntr->pStream, _TWET_INV "%d" _TWET_RST ": %s%s%s"_TWELB
				, i
				, (i == 0) ? "" : " "
				, (i == psIntr->i16SelectedIndex) ? _TWET_INV : "" // selected index, display inverted.
				, pstrSlotName);

			if (i == psIntr->i16SelectedIndex) {
				// reset invert.
				TWE_fputs(_TWET_RST, psIntr->pStream);
			}
		}
	}
	{
		int i = (psIntr->config.u8DefMenusSlots ? psIntr->config.u8DefMenusSlots + 1 : 0);
		int j = (psIntr->config.u8DefMenusSlots ? 2 : 1);
		while (psIntr->pFuncs[j].u8MenuId != 0xFF) {
			TWE_fprintf(psIntr->pStream, _TWET_INV "%d" _TWET_RST ": %s%s"_TWELB, i,
				(i == psIntr->i16SelectedIndex) ? _TWET_INV : "", // selected index, display inverted.
				psIntr->pFuncs[j].pu8MenuString);

			if (i == psIntr->i16SelectedIndex) {
				// reset invert.
				TWE_fputs(_TWET_RST, psIntr->pStream);
			}

			TWE_fflush(psIntr->pStream);
			i++;
			j++;
		}
	}

	TWE_fputs(_TWELB, psIntr->pStream);
	TWEINTRCT_vSerFootLine(psIntr, 0); // フッター行の表示
	TWE_fflush(psIntr->pStream);
}

/*!
 * インタラクティブモードでの１バイトコマンド入力を処理する。
 * 
 * \param u8Byte 入力バイト
 */
void TWEINTCT_vProcessInputByte_defmenus(TWEINTRCT_tsContext *psIntr, TWEINTRCT_tkeycode keycode) {
	bool_t bInhibitUpdate = TRUE;
	bool_t bHandled = FALSE;
	int16_t iCommitSelection = -1;

	psIntr->u16HoldUpdateScreen = 0;

	TWE_APIRET apiRet;

	if (keycode >= '0' && keycode < psIntr->u8screen_max + psIntr->config.u8DefMenusSlots + '0') {
		iCommitSelection = keycode - '0';
		keycode = '1';
	}

	switch (keycode) {
	case _TWECR: // CR
		if (psIntr->i16SelectedIndex == -1) {
			bHandled = TRUE; // clear screen
		} else {
			iCommitSelection = psIntr->i16SelectedIndex;
		}
		break;

	case '1': // Direct Selection '1' .. N.
		break;
	
	case TWEINTRCT_KEY_UP:
		if (psIntr->i16SelectedIndex > 0) {
			psIntr->i16SelectedIndex--;
		}
		bHandled = TRUE; // redraw screen
		break;

	case TWEINTRCT_KEY_DOWN:
		if (psIntr->i16SelectedIndex < psIntr->u8screen_max + psIntr->config.u8DefMenusSlots - 2) {
			psIntr->i16SelectedIndex++;
		}
		bHandled = TRUE; // redraw screen
		break;

	default:
		apiRet = TWEINTRCT_u32MenuOpKey(psIntr, keycode);
		if (TWE_APIRET_IS_SUCCESS(apiRet)) {
			bHandled = TRUE;
		}
		break;
	}

	if (iCommitSelection >= 0) {
		uint8_t u8NextPage = 0;
		psIntr->i16SelectedIndex = iCommitSelection;

		if (psIntr->config.u8DefMenusSlots > 0) {
			if (iCommitSelection <= psIntr->config.u8DefMenusSlots) {
				TWE_APIRET apiRet = TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_CHANGE_KIND_SLOT, 0xFF, iCommitSelection, NULL);
				u8NextPage = 1;
			}
			else {
				u8NextPage = (uint8_t)(iCommitSelection - psIntr->config.u8DefMenusSlots + 1);
			}
		} else {
			u8NextPage = (uint8_t)iCommitSelection + 1;
		}

		TWEINTCT_vSerUpdateScreen_defmenus(psIntr);
		psIntr->u16HoldUpdateScreen = 8;
		TWEINTRCT_u32MenuChange(psIntr, u8NextPage);
		bHandled = TRUE;
	}

	// 画面を更新する
	if (bHandled) {
		if (psIntr->u16HoldUpdateScreen == 0) psIntr->u16HoldUpdateScreen = 1;
		bInhibitUpdate = FALSE;
	}

	// 処理されない場合
	if (!bHandled) {
		psIntr->u16HoldUpdateScreen = 1;
		bInhibitUpdate = FALSE;
	}

	// 画面更新しない
	if (bInhibitUpdate) {
		psIntr->u16HoldUpdateScreen = 0;
	}
}

/*!
 * 入力処理(TWEINPSTR)完了時の処理
 * 
 * 入力された文字列をバリデータ(TWESTGS_VLD_*)に渡し、要素データ(tuDatum)への変換と
 * データのチェックを行う。
 * 
 * \param pContext 入力処理(TWEINPSTR)
 */
void TWEINTCT_vProcessInputString_defmenus(TWEINTRCT_tsContext *psIntr, TWEINPSTR_tsInpStr_Context *pContext) {
	// 結果出力
	TWE_fputs(_TWELB, psIntr->pStream);

	// 一定時間待って画面を再描画
	psIntr->u16HoldUpdateScreen = 96; // 約1.5sec
}

/**
 * @brief ルートメニュー
 * 
 * @param pContext 
 * @param u32Op 
 * @param u32Arg1 
 * @param u32Arg2 
 * @param vpArg 
 * @return TWE_APIRET 
 */
TWE_APIRET TWEINTCT_u32ProcessMenuEvent_defmenus(TWEINTRCT_tsContext *psIntr, uint32 u32Op, uint32 u32Arg1, uint32 u32Arg2, void *vpArg) {
	TWE_APIRET ret = TWE_APIRET_FAIL;

	switch (u32Op) {
		case E_TWEINTCT_MENU_EV_LOAD:
			TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINTCT_MENU_EV_LOAD, psIntr->u8screen, 0, NULL); // メニューへの遷移（初期化）をアプリケーションに伝える
			ret = TWE_APIRET_SUCCESS;
			
			psIntr->i16SelectedIndex = -1;
		break;

		default:
		break;

	}

	return ret;
}