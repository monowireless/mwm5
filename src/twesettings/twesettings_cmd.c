/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twecommon.h"
#include "twesettings0.h"
#include "twesettings_std.h"
#include "twesettings_cmd.h"

#include "tweinteractive.h"

#include "twesettings_weak.h"

/*!
 * コマンド処理の実装
 * 
 * \param u8Op    コマンド番号
 * \param pBufIn  入力コマンドのペイロード
 * \param pBufOut 出力データのペイロード
 * \param psFinal 確定設定リスト
 * \return 成功時：LSBから8bit戻りコマンドのID
 */
TWE_APIRET TWESTG_CMD_u32CmdOp(uint8 u8Op, TWE_tsBuffer *pBufIn, TWE_tsBuffer *pBufOut, TWESTG_tsFinal *psFinal) {
	TWE_APIRET ret = TWE_APIRET_FAIL;
	TWE_APIRET apiRet = TWE_APIRET_FAIL;
	TWESTG_teCMD_OP eOp = (TWESTG_teCMD_OP)u8Op;

	uint8 *p = pBufIn->pu8buff;
	uint8 u8firstbyte = *p;

	switch (eOp) {
	case E_TWESTG_CMD_OP_ACK:
		if (pBufIn->u8bufflen > 0) {
			// 入力文字をそのまま返す
			int i;
			for (i = 0; i < pBufIn->u8bufflen && i < pBufOut->u8bufflen_max; i++) {
				pBufOut->pu8buff[i] = pBufIn->pu8buff[i];
			}
			pBufOut->u8bufflen = pBufIn->u8bufflen;
		}
		else {
			// 引数ナシなら 0x01 を戻す
			pBufOut->pu8buff[0] = 0x01;
			pBufOut->u8bufflen = 1;
		}
		ret = TWE_APIRET_SUCCESS_W_VALUE(E_TWESTG_CMD_OP_ACK);
		break;

	case E_TWESTG_CMD_OP_QUERY_MODULE_INFO:
		if (u8firstbyte == 0x01) { // get module address
			uint8 *q = pBufOut->pu8buff;
			uint32 u32adr = 0;

			apiRet = TWEINTRCT_cbu32GenericHandler(NULL, E_TWEINTCT_OP_GET_SID, 0, 0, &u32adr);

			TWE_S_OCTET(q, 0x01);
			TWE_S_DWORD(q, u32adr);
			pBufOut->u8bufflen = (uint8)(q - pBufOut->pu8buff);

			ret = TWE_APIRET_SUCCESS_W_VALUE(E_TWESTG_CMD_OP_QUERY_MODULE_INFO);
		}
		break;

	case E_TWESTG_CMD_OP_QUERY_SETTINGS: // pBuffOut に設定一覧を出力する
		if (1) {
			uint8 u8Opt = u8firstbyte; // 最初のバイトはオプション
			if (pBufIn->u8bufflen == 0) { // オプションバイトの省略
				u8Opt = 0x00;
			}

			TWE_tsBuffer bSave;
			bSave.pu8buff = pBufOut->pu8buff + 1;
			bSave.u8bufflen = 0;
			bSave.u8bufflen_max = pBufOut->u8bufflen_max - 1;

			apiRet = TWESTG_STD_u32FinalToSerializedBuff(&bSave, psFinal, psFinal->sVer.u8Set, TWESTG_SAVE_OPT_INCLUDE_HIGHER_SETTINGS);
			if (TWE_APIRET_IS_SUCCESS(apiRet)) {
				pBufOut->pu8buff[0] = u8Opt; // オプションはオウム返し
				pBufOut->u8bufflen = bSave.u8bufflen + 1;
				ret = TWE_APIRET_SUCCESS_W_VALUE(E_TWESTG_CMD_OP_QUERY_SETTINGS);
			}
			else {
				pBufOut->pu8buff[0] = u8Opt; // オプションはオウム返し
				pBufOut->pu8buff[1] = TWESTG_FORMAT_VER_VOID;
				pBufOut->u8bufflen = 2;
				ret = TWE_APIRET_FAIL_W_VALUE(E_TWESTG_CMD_OP_QUERY_SETTINGS);
			}
			break;
		}

	case E_TWESTG_CMD_OP_APPLY_SETTINGS: // pBuffIn からの設定データを適用する
		if (1) {
			uint8 u8Opt = u8firstbyte; // 最初のバイトはオプション
			TWE_tsBuffer bLoad;
			bLoad.pu8buff = pBufIn->pu8buff + 1;
			bLoad.u8bufflen = pBufIn->u8bufflen - 1;
			bLoad.u8bufflen_max = pBufIn->u8bufflen_max - 1;
			bool_t bFail = TRUE;

			if (bLoad.u8bufflen > 0) {
				apiRet = TWESTG_CMD_u32SerializedBuffToFinal(psFinal, &bLoad, TWESTG_LOAD_OPT_LOAD_AS_UNSAVED); // セーブはしない

				if (TWE_APIRET_IS_SUCCESS(apiRet)) {
					TWE_tsBuffer bSave;
					bSave.pu8buff = pBufOut->pu8buff + 1;
					bSave.u8bufflen = 0;
					bSave.u8bufflen_max = pBufOut->u8bufflen_max - 1;

					// 現在の設定情報を返す
					apiRet = TWESTG_STD_u32FinalToSerializedBuff(&bSave, psFinal, psFinal->sVer.u8Set, 0 /* TWESTG_SAVE_OPT_INCLUDE_HIGHER_SETTINGS */);
					if (TWE_APIRET_IS_SUCCESS(apiRet)) {
						pBufOut->pu8buff[0] = u8Opt; // オプションはオウム返し
						pBufOut->u8bufflen = bSave.u8bufflen + 1;
						ret = TWE_APIRET_SUCCESS_W_VALUE(E_TWESTG_CMD_OP_APPLY_SETTINGS);
						bFail = FALSE;
					}
				}
			}
			
			if(bFail) {
				pBufOut->pu8buff[0] = u8Opt;
				pBufOut->pu8buff[1] = TWESTG_FORMAT_VER_VOID;
				pBufOut->u8bufflen = 2;
				ret = TWE_APIRET_FAIL_W_VALUE(E_TWESTG_CMD_OP_APPLY_SETTINGS);
			}
		}
		
		break;

	case E_TWESTG_CMD_OP_MODULE_CONTROL:
		if(TRUE) {
			uint8 u8CtlCmd = TWE_G_OCTET(p);

			if (u8CtlCmd == 0x20) { // change kind and slot
				uint8 u8kind = TWE_G_OCTET(p);
				uint8 u8slot = TWE_G_OCTET(p);
				apiRet = TWEINTRCT_cbu32GenericHandler(NULL, E_TWEINRCT_OP_CHANGE_KIND_SLOT, u8kind, u8slot, NULL);

				uint8 *q = pBufOut->pu8buff;

				TWE_S_OCTET(q, u8CtlCmd); // 0x20

				if (TWE_APIRET_IS_SUCCESS(apiRet)) {
					uint8 u8kindnew = (TWE_APIRET_VALUE(apiRet) & 0xFF00) >> 8;
					uint8 u8slotnew = (TWE_APIRET_VALUE(apiRet) & 0xFF);

					TWE_S_OCTET(q, u8kindnew);
					TWE_S_OCTET(q, u8slotnew);

					pBufOut->u8bufflen = (uint8)(q - pBufOut->pu8buff);
					ret = TWE_APIRET_SUCCESS_W_VALUE(E_TWESTG_CMD_OP_MODULE_CONTROL);
				}
				else {
					TWE_S_OCTET(q, 0xFF);

					pBufOut->u8bufflen = (uint8)(q - pBufOut->pu8buff);
					ret = TWE_APIRET_FAIL_W_VALUE(E_TWESTG_CMD_OP_MODULE_CONTROL);
				}
			}
		}
		break;

	case E_TWESTG_CMD_OP_SAVE_AND_RESET:
		TWESTG_u32SaveDataToAppStrg(psFinal, psFinal->u8Kind, psFinal->u8Slot, psFinal->u32AppId, psFinal->sVer.u8Set, 0UL);
		TWEINTRCT_cbu32GenericHandler(NULL, E_TWEINRCT_OP_RESET, FALSE, 0, NULL);

		pBufOut->pu8buff[0] = 0x00;
		pBufOut->u8bufflen = 1;
		ret = TWE_APIRET_SUCCESS_W_VALUE(E_TWESTG_CMD_OP_SAVE_AND_RESET); // may not reach this line...
		break;

	case E_TWESTG_CMD_OP_DO_MDDULE_RESET:
		TWEINTRCT_cbu32GenericHandler(NULL, E_TWEINRCT_OP_RESET, FALSE, 0, NULL);

		pBufOut->pu8buff[0] = 0x00;
		pBufOut->u8bufflen = 1;
		ret = TWE_APIRET_SUCCESS_W_VALUE(E_TWESTG_CMD_OP_DO_MDDULE_RESET); // may not reach this line...
		break;

	default:
		// 未対応メッセージ
		pBufOut->pu8buff[0] = 0xde;
		pBufOut->pu8buff[1] = 0xad;
		pBufOut->pu8buff[2] = 0xbe;
		pBufOut->pu8buff[3] = 0xef;
		pBufOut->u8bufflen = 4;

		ret = TWE_APIRET_SUCCESS_W_VALUE(E_TWESTG_CMD_OP_ACK);
		break;
	}

	return ret;
}



