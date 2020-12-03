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

// if debugging, ...
#define DEBUGMSG
// #define DEBUGMSG(...) TWE_fprintf(psIntr->pStream, __VA_ARGS__) 

static int i_defct = 0;
static int i_modct = 0;
static int i_savct = 0;

/*!
 * インタラクティブモードで画面クリアと設定の全表示を行う
 */
static void s_TWEINTCT_vSerUpdateScreen_settings(TWEINTRCT_tsContext *psIntr, int idx) {
	if (idx == -1) {
		TWEINTRCT_vSerHeadLine(psIntr, 0x01UL);
		TWE_fputs(_TWELB, psIntr->pStream);
	}

	int i, i_end, i_sel;
	if (idx == -1) {
		i = 0;
		i_end = psIntr->psFinal->u8DataCount;
		i_sel = psIntr->i16SelectedIndex;

		i_defct = 0; // items to be set as DEFAULT
		i_modct = 0; // items to be modified
		i_savct = 0; // items which has save data
	} else {
		if (idx < psIntr->psFinal->u8DataCount) {
			i = idx;
			i_end = idx + 1;
			i_sel = idx;
		} else {
			// error
			TWE_fputs("...ERROR...",psIntr->pStream);
			return;
		}
	}

	for (; i < i_end; i++) {
		uint8 u8mark = ' ';
		if (psIntr->psFinal->u8Opt & TWESTG_FINAL_OPT_REVERTED_TO_DEFAULT) { u8mark = '*'; }
		else if (psIntr->psFinal->asDatum[i].u8Stat & TWESTG_DATASTAT_MODIFIED_MASK) { u8mark = '*'; }
		else if (psIntr->psFinal->asDatum[i].u8Stat & TWESTG_DATASTAT_SAVED_MASK) { u8mark = '$'; }
		else if (psIntr->psFinal->asDatum[i].u8Stat & TWESTG_DATASTAT_SAVED_HIGHER_MASK) u8mark = '^';

		if (idx == -1) {
			if (psIntr->psFinal->u8Opt & TWESTG_FINAL_OPT_REVERTED_TO_DEFAULT) { i_defct++;  }
			else if (psIntr->psFinal->asDatum[i].u8Stat & TWESTG_DATASTAT_MODIFIED_MASK) { i_modct++; }
			else if (psIntr->psFinal->asDatum[i].u8Stat & TWESTG_DATASTAT_SAVED_MASK) { i_savct++; }
			else if (psIntr->psFinal->asDatum[i].u8Stat & TWESTG_DATASTAT_SAVED_HIGHER_MASK) { ; }
		} 
		
		TWE_fprintf(psIntr->pStream, _TWET_INV "%c" _TWET_RST ":%c%s(" // " %c:%c%s(%3s=",
			, psIntr->psFinal->apEle[i]->sFormat.u8ShortcutKey
			, u8mark
			, (i == i_sel) ? _TWET_INV : ""
			//, psIntr->psFinal->apEle[i]->sStr.strLabel // 3letters label
		);

		if (psIntr->psFinal->apEle[i]->sFormat.u8Format & E_TWEINPUTSTRING_DATATYPE_CUSTOM_DISP_MASK) {
			uint8 au8buf[TWESTGS_VLD_MAX_CUSTOMSTR_BUFF];
			TWE_tsBuffer sBuf;
			TWE_vInit_tsBuffer(&sBuf, au8buf, 0, TWESTGS_VLD_MAX_CUSTOMSTR_BUFF);
			
			psIntr->psFinal->apEle[i]->sValidate.fpOp(psIntr->psFinal->apEle[i], &psIntr->psFinal->asDatum[i], TWESTGS_VLD_OP_CUSTDISP, &sBuf);
			TWE_fprintf(psIntr->pStream, "%-10s", sBuf.pu8buff);
		} else {
			uint32 u32Val = TWESTG_u32GetU32FrUDatum(psIntr->psFinal->asDatum[i].u8Type, &psIntr->psFinal->asDatum[i].uDatum);
			

			switch (psIntr->psFinal->apEle[i]->sFormat.u8Format) {
			case E_TWEINPUTSTRING_DATATYPE_DEC:
				TWE_fprintf(psIntr->pStream, "%10d", u32Val);
				break;
			case E_TWEINPUTSTRING_DATATYPE_DEC_DECA:
				TWE_fprintf(psIntr->pStream, "%%9d0", u32Val);
				break;
			case E_TWEINPUTSTRING_DATATYPE_DEC_HECTO:
				TWE_fprintf(psIntr->pStream, "%8d00", u32Val);
				break;
			case E_TWEINPUTSTRING_DATATYPE_HEX: 
				_TWESC(TRUE) {
					uint8 fmt[16];
					uint8 len = psIntr->psFinal->asDatum[i].u8Len * 2;
					if (len > 8) len = 8;
					uint8 pad = 8 - len;

					TWE_snprintf((char*)fmt, 16, "%%%ds0x%%0%dX", pad, len);
					TWE_fprintf(psIntr->pStream, (const char*)fmt, "", u32Val);
				}
				break;
			case E_TWEINPUTSTRING_DATATYPE_STRING:
				_TWESC(TRUE) {
					int ct = 0;
					uint8 *p = psIntr->psFinal->asDatum[i].uDatum.pu8;

					while(*p != 0 && ct < psIntr->psFinal->asDatum[i].u8Len) {
						TWE_fputc(*p, psIntr->pStream);
						p++; ct++;
					}
					if (ct < 10) {
						while (ct < 10) {
							TWE_fputc(' ', psIntr->pStream);
							ct++;
						}
					}
				}
				break;

			default:
				// エラー処理は省略
				break;
			}
		}
#if 0 // DEBUGMSG // TODO DEBUG
		TWE_fprintf(psIntr->pStream, "<%02X>",
			psIntr->psFinal->asDatum[i].u8Stat,
			psIntr->psFinal->apEle[i]->sStr.strLabel
		); // デバッグ表示
#endif

		{
			// 設定名（入れ替えテーブルの探索）
			const char *name = psIntr->psFinal->apEle[i]->sStr.strName;
			const TWESTG_tsMsgReplace *p = psIntr->msgReplace;
			if(p) {
				while(p->u8Id != 0xFF) { 
					if (p->u8Id == psIntr->psFinal->apEle[i]->u16Id) {
						name = p->strName;
						break;
					}
					p++;
				} 
			}
			TWE_fprintf(psIntr->pStream, ") %s"_TWELB, name);
		}

		// invert close
		if (i == i_sel) {
			TWE_fputs(_TWET_RST,psIntr->pStream);
		}

		TWE_fflush(psIntr->pStream);
	}


	if (idx == -1) {
		//TWE_fputs("   */modified $/saved ^/slot0 saved"_TWELB, psIntr->pStream);
		
		if (psIntr->psFinal->u8Opt & TWESTG_FINAL_OPT_SAVED) {
			TWE_fputs(_TWET_INV "!!! Saved, Please RESET to take effect."_TWET_RST _TWELB, psIntr->pStream);
		}
		if (psIntr->psFinal->u8Opt & TWESTG_FINAL_OPT_REVERTED_TO_DEFAULT) {
			TWE_fputs(_TWET_INV"!!! Reverted, Please SAVE and RESET to take effect."_TWET_RST _TWELB, psIntr->pStream);
		}

		TWE_fputs(_TWELB, psIntr->pStream);

		if (i_modct > 0 || i_defct > 0)
			TWE_fputs("- [S]:Save [R]:Reload settings"_TWELB, psIntr->pStream);
		else if (i_modct == 0 && i_savct > 0)
			TWE_fputs("- [S]:Save [R]:DEFAULT settings"_TWELB, psIntr->pStream);


		TWEINTRCT_vSerFootLine(psIntr, 0); // フッター行の表示
	}

	// move cursor to the selected
	if (idx == -1 && i_sel >= 0) {
		TWE_fprintf(psIntr->pStream, "\033[%d;1H", i_sel + 2);
	}

	TWE_fflush(psIntr->pStream);
}

void TWEINTCT_vSerUpdateScreen_settings(TWEINTRCT_tsContext *psIntr) {
	s_TWEINTCT_vSerUpdateScreen_settings(psIntr, -1);
}

/*!
 * インタラクティブモードでの１バイトコマンド入力を処理する。
 * 
 * \param keycode 入力バイト
 */
void TWEINTCT_vProcessInputByte_settings(TWEINTRCT_tsContext *psIntr, TWEINTRCT_tkeycode keycode) {
	//static uint8 u8lastbyte = 0xFF;
	bool_t bInhibitUpdate = TRUE;
	bool_t bHandled = FALSE;
    TWE_APIRET apiRet;
	bool_t bChangeKindSlot = FALSE;
	bool_t bCommitSelection = FALSE;

	if (keycode == _TWELF) return; // LF は無視
	
	psIntr->u16HoldUpdateScreen = 0;

	switch (keycode) {
	//case _TWELF: // LF
    case _TWECR: // CR
		if (psIntr->i16SelectedIndex == -1) {
			bHandled = TRUE; // clear screen
		} else {
			bCommitSelection = TRUE;
		}
		break;
	case _TWECLR: // Ctrl+L
		// 画面の書き換え
		bHandled = TRUE;
		break;

	case 'S': // Save
		if (i_modct > 0 || i_defct > 0) {
			if (psIntr->config.u8NoSysReset) {
				TWE_fprintf(psIntr->pStream, _TWELB _TWET_INV"!!! Settings SAVED. !!!"_TWET_RST);
			} else {
				TWE_fprintf(psIntr->pStream, _TWELB _TWET_INV"!!! Settings SAVED. !!!"_TWET_RST 
										     _TWELB _TWET_INV"Note: Please RESET to take effects."_TWET_RST);
			}
			TWE_fflush(psIntr->pStream);

			TWESTG_u32SaveDataToAppStrg(psIntr->psFinal, psIntr->psFinal->u8Kind, psIntr->psFinal->u8Slot, psIntr->psFinal->u32AppId, psIntr->psFinal->sVer.u8Set, 0UL);

			psIntr->psFinal->u8Opt |= TWESTG_FINAL_OPT_SAVED; // SAVE フラグ
			if (psIntr->psFinal->u8Opt & TWESTG_FINAL_OPT_REVERTED_TO_DEFAULT) {
				psIntr->psFinal->u8Opt &= ~TWESTG_FINAL_OPT_REVERTED_TO_DEFAULT;
			}

			if (!psIntr->config.u8NoSysReset) {
				// do reset
				TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_RESET, 0, 0, NULL);
			} else {
				// reload
				TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_REVERT, FALSE, 0, NULL);
			}
			
		} else {
			TWE_fprintf(psIntr->pStream, _TWELB"!!! not saved.");
		}
		psIntr->u16HoldUpdateScreen = 96;	
		bHandled = TRUE;
		break;

	case 'R': // Revert Settings
		_TWESC(TRUE) {
			if (i_modct > 0 || i_defct > 0) {
				// modified some, to RELOAD
				TWE_fprintf(psIntr->pStream, _TWELB _TWET_INV"!!! Reload settings !!!"_TWET_RST);
				apiRet = TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_REVERT, FALSE, 0, NULL);
			} else if (i_modct == 0 && i_savct > 0) {
				// revert to DEFAULT
				TWE_fprintf(psIntr->pStream, _TWELB _TWET_INV"!!! Revert to DEFAULT !!!"_TWET_RST
				     						 _TWELB _TWET_INV"Note: [S]:Save to take effect."_TWET_RST);
				apiRet = TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_REVERT, TRUE, 0, NULL);
				psIntr->psFinal->u8Opt |= TWESTG_FINAL_OPT_REVERTED_TO_DEFAULT; // REVERT フラグ
			}
			psIntr->u16HoldUpdateScreen = 96;
			bHandled = TRUE;
		}
		break;

	case TWEINTRCT_KEY_RIGHT:
	case '>': // NEXT SLOT (TODO)
		apiRet = TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_CHANGE_KIND_SLOT, 0xFF, 0x0100, NULL);
		if (TWE_APIRET_IS_SUCCESS(apiRet)) bChangeKindSlot = TRUE;
		bHandled = TRUE;
		break;

	case TWEINTRCT_KEY_LEFT:
	case '<': // PREV SLOT (TODO)
		apiRet = TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_CHANGE_KIND_SLOT, 0xFF, 0xFF00, NULL);
		if (TWE_APIRET_IS_SUCCESS(apiRet)) bChangeKindSlot = TRUE;
		bHandled = TRUE;
		break;

	case ')': // NEXT KIND (TODO)
		apiRet = TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_CHANGE_KIND_SLOT, 0x0100, 0xFF, NULL);
		if (TWE_APIRET_IS_SUCCESS(apiRet)) bChangeKindSlot = TRUE;
		bHandled = TRUE;
		break;

	case '(': // PREV KIND (TODO)
		apiRet = TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_CHANGE_KIND_SLOT, 0xFF00, 0xFF, NULL);
		if (TWE_APIRET_IS_SUCCESS(apiRet)) bChangeKindSlot = TRUE;
		bHandled = TRUE;
		break;

	case TWEINTRCT_KEY_UP:
		if (psIntr->i16SelectedIndex > 0) {
			psIntr->i16SelectedIndex--;
		}
		bHandled = TRUE; // redraw screen
		break;

	case TWEINTRCT_KEY_DOWN:
		if (psIntr->i16SelectedIndex < psIntr->psFinal->u8DataCount - 1) {
			psIntr->i16SelectedIndex++;
		}
		bHandled = TRUE; // redraw screen
		break;

	case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': // DIRECT SLOT SELECTION
		apiRet = TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_CHANGE_KIND_SLOT, 0xFF, keycode - '0', NULL);
		if (TWE_APIRET_IS_SUCCESS(apiRet)) bChangeKindSlot = TRUE;
		psIntr->u16HoldUpdateScreen = 5;
		bHandled = TRUE;
		break;
	
	default:
        apiRet = TWEINTRCT_u32MenuOpKey(psIntr, keycode);
		if (TWE_APIRET_IS_SUCCESS(apiRet)) {
			bHandled = TRUE;
		}
		break;
	}

	// KIND/SLOT に変更があった(戻り値のkind/slotを保存)
	if (bChangeKindSlot) {
		psIntr->psFinal->u8Slot = TWE_B(apiRet, 0);
		psIntr->psFinal->u8Kind = TWE_B(apiRet, 8);
		//DEBUGMSG("<CHANGE %d:%d>", psIntr->psFinal->u8Kind, psIntr->psFinal->u8Slot);
	}

	// 画面を更新する
	if (bHandled) {
		if (psIntr->u16HoldUpdateScreen == 0) psIntr->u16HoldUpdateScreen = 5;
		bInhibitUpdate = FALSE;
	}

	// 設定項目かどうかを確認する
	if (!bHandled) {
		int i;

		if (bCommitSelection) {
			i = psIntr->i16SelectedIndex;
		} else {
			for (i = 0; i < psIntr->psFinal->u8DataCount && psIntr->psFinal->apEle[i] != NULL; i++) {
				if (keycode == psIntr->psFinal->au8ShortcutKeys[i]) {
					bCommitSelection = TRUE;
					break;
				}
			}
		}

		// check the index `i' again.
		if (bCommitSelection && i < psIntr->psFinal->u8DataCount && psIntr->psFinal->apEle[i] != NULL) {
			// 設定入力に入る
			TWESTG_tsElement *pE = psIntr->psFinal->apEle[i];

			// clear screen
			TWE_fputs(_TWET_CLS _TWELB, psIntr->pStream);
			
			s_TWEINTCT_vSerUpdateScreen_settings(psIntr, i);
			
			// 設定名（入れ替えテーブルの探索）
			const char *desc = psIntr->psFinal->apEle[i]->sStr.strDesc;
			const TWESTG_tsMsgReplace *p = psIntr->msgReplace;
			if(p) {
				while(p->u8Id != 0xFF) { 
					if (p->u8Id == psIntr->psFinal->apEle[i]->u16Id) {
						desc = p->strDesc;
						break;
					}
					p++;
				}
			}
			if (desc[0] != '\0') TWE_fprintf(psIntr->pStream, _TWELB"%s", desc);
		
			TWE_fprintf(psIntr->pStream, _TWELB "Input: ", pE->sStr.strLabel);
			TWEINPSTR_vStart(psIntr->pSerInpStr, pE->sFormat.u8Format & E_TWEINPUTSTRING_DATATYPE_STANDARD_MASK, pE->sFormat.u8InputMaxLength, i);

			bHandled = TRUE;
		}
	}

	// 処理されなかったキーはコールバックで処理
	// int i = TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_UNHANDLED_CHAR, u8Byte, u8lastbyte, NULL);
	
	// 処理されない場合
	if (!bHandled) {
		bInhibitUpdate = FALSE;
	}

	// 最後の処理
	//u8lastbyte = u8Byte; // 2キー入力を行うための直前のストロークの保存

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
void TWEINTCT_vProcessInputString_settings(TWEINTRCT_tsContext *psIntr, TWEINPSTR_tsInpStr_Context *pContext) {
	//uint8 *pu8str = pContext->au8Data;
	//uint8 u8idx = pContext->u8Idx;

	int i = pContext->u32Opt;
	if (i < psIntr->psFinal->u8DataCount) { // } && pContext->u8Idx > 0) { // ０バイト入力も許容する
		TWESTG_tsElement *pE = psIntr->psFinal->apEle[i];
		TWESTG_tsDatum *pDat = &(psIntr->psFinal->asDatum[i]);

		// 文字列からデータ型への変換
		TWE_tsBuffer sBuf;
		TWE_vInit_tsBuffer(&sBuf, pContext->au8Data, pContext->u8Idx, pContext->u8MaxLen);
		
		// validator の実行
		TWE_APIRET apiRet = TWE_APIRET_SUCCESS;		
		if (pE->sValidate.fpOp != NULL) { // validator を実行する
			apiRet = pE->sValidate.fpOp(pE, pDat, TWESTGS_VLD_OP_VALIDATE, &sBuf); // データ変換とチェック
		}

		// validator の実行結果の評価
		if (TWE_APIRET_IS_SUCCESS(apiRet)) { // データがチェックされた＝pDtに書き込まれている
			pDat->u8Stat |= (TWESTG_DATASTAT_SAVED_MASK | TWESTG_DATASTAT_MODIFIED_MASK); // セーブデータがある場合のマスクを設定
		}
		else {
			TWE_fputs("(invalid data)", psIntr->pStream);
		}
	}
	else {
		TWE_fputs("(canceled)", psIntr->pStream);
	}

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
TWE_APIRET TWEINTCT_u32ProcessMenuEvent_settings(TWEINTRCT_tsContext *psIntr, uint32 u32Op, uint32 u32Arg1, uint32 u32Arg2, void *vpArg) {
	TWE_APIRET ret = TWE_APIRET_FAIL;
	TWE_APIRET apiRet = TWE_APIRET_FAIL;

	switch (u32Op) {
		case E_TWEINTCT_MENU_EV_LOAD:
			/// ロード処理
			// ロード処理の戻り値は、設定された Kind と Id 値
			// アプリケーションは、最初にロードすべき Kind/Slot を設定の上、ロード処理をすます。
			// ロード完了後に psIntr->psFinal をロード済みの構造体にセットする。
			// 戻り値の FF00 マスク：KIND、00FF マスク：SLOT
			apiRet = TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINTCT_MENU_EV_LOAD, psIntr->u8screen, 0, NULL); // メニューへの遷移（初期化）をアプリケーションに伝える
			if (TWE_APIRET_IS_SUCCESS(apiRet)) {
				psIntr->psFinal->u8Slot = TWE_B(apiRet, 0);
				psIntr->psFinal->u8Kind = TWE_B(apiRet, 8);

				ret = TWE_APIRET_SUCCESS;
			}

			psIntr->i16SelectedIndex = -1;

		break;

		default:
		break;
	}

	return ret;
}