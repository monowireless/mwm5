/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <string.h>

#include "twecommon.h"
#include "twesettings0.h"
#include "twesettings_validator.h"

#include "tweutils.h"
#include "twestring.h"
#include "tweinputstring.h"
#include "tweprintf.h"


/*!
 * 数値型を変換したうえ、最大最小のチェックを行う
 * 
 * \param pMe     単一設定定義
 * \param pDatum  書き出し先のデータ構造
 * \param u16OpId 処理内容
 * \param pBuf    読み込み・書き出し用のバッファ
 * \return        成功・失敗
 */
TWE_APIRET TWESTGS_VLD_u32MinMax(struct _TWESTG_sElement* pMe, TWESTG_tsDatum* psDatum, uint16 u16OpId, TWE_tsBuffer* pBuf) {
	TWE_APIRET ret = TWE_APIRET_FAIL;
	TWESTG_tuDatum* pDatum = &psDatum->uDatum;

	if (u16OpId == TWESTGS_VLD_OP_VALIDATE && pBuf->u8bufflen == 0) {
		// 未入力の場合はデフォルトに戻す
		TWESTG_vSetUDatumFrUDatum(pMe->sDatum.u8Type, pDatum, &pMe->sDatum.uDatum);
		ret = TWE_APIRET_SUCCESS;
	} else 	if (u16OpId == TWESTGS_VLD_OP_VALIDATE) {
		uint32 u32num = 0;
		switch (pMe->sFormat.u8Format) {
		case E_TWEINPUTSTRING_DATATYPE_DEC_DECA:
			u32num = TWESTR_i32DecstrToNum(pBuf->pu8buff, pBuf->u8bufflen) / 10;
			ret = TWE_APIRET_SUCCESS;
		case E_TWEINPUTSTRING_DATATYPE_DEC_HECTO:
			u32num = TWESTR_i32DecstrToNum(pBuf->pu8buff, pBuf->u8bufflen) / 100;
			ret = TWE_APIRET_SUCCESS;
		case E_TWEINPUTSTRING_DATATYPE_DEC:
			u32num = TWESTR_i32DecstrToNum(pBuf->pu8buff, pBuf->u8bufflen);
			ret = TWE_APIRET_SUCCESS;
			break;
		case E_TWEINPUTSTRING_DATATYPE_HEX:
			u32num = TWESTR_u32HexstrToNum(pBuf->pu8buff, pBuf->u8bufflen);
			ret = TWE_APIRET_SUCCESS;
			break;
		default:
			break;
		}

		// 値の変換に成功したので値の範囲のチェックを行う
		if (TWE_APIRET_IS_SUCCESS(ret)) {
			ret = TWE_APIRET_FAIL; // 一旦FAILにして、チェックが成功したら TRUE に戻す
			if (pMe->sDatum.u8Type & 0x1) { // 符号なし
				if (u32num >= pMe->sValidate.d1.u32 && u32num <= pMe->sValidate.d2.u32) {
					TWESTG_vSetUDatumFrU32(pMe->sDatum.u8Type, pDatum, u32num);
					ret = TWE_APIRET_SUCCESS;
				}
			}
			else { // 符号あり
				if ((int32)u32num >= pMe->sValidate.d1.i32 && (int32)u32num <= pMe->sValidate.d2.i32) {
					TWESTG_vSetUDatumFrU32(pMe->sDatum.u8Type, pDatum, u32num);
					ret = TWE_APIRET_SUCCESS;
				}
			}
		}
	}

	return ret;
}

/*!
 * 文字列の場合、長さチェックのみ
 * 
 * \param pMe     単一設定定義
 * \param pDatum  書き出し先のデータ構造
 * \param u16OpId 処理内容
 * \param pBuf    読み込み・書き出し用のバッファ
 * \return        成功・失敗
 */
TWE_APIRET TWESTGS_VLD_u32String(struct _TWESTG_sElement* pMe, TWESTG_tsDatum* psDatum, uint16 u16OpId, TWE_tsBuffer* pBuf) {
	TWE_APIRET ret = TWE_APIRET_FAIL;
	TWESTG_tuDatum* pDatum = &psDatum->uDatum;

	if (u16OpId == TWESTGS_VLD_OP_VALIDATE) {
		if (pMe->sDatum.u8Len >= pBuf->u8bufflen) {
			// pDatum->pu8には事前にバッファが割り当てられている
			memcpy(pDatum->pu8, pBuf->pu8buff, pBuf->u8bufflen);
			uint8* p = pDatum->pu8 + pBuf->u8bufflen;   // memcpyの末尾ポインタ＋１
			uint8* e = pDatum->pu8 + pMe->sDatum.u8Len; // 末尾ポインタ＋１

			// 末尾に余裕がある場合は 0 終端しておく
			if (p < e) {
				*p = '\0';
			}

			// STRING のバッファ長の保存
			psDatum->u8Len = pBuf->u8bufflen;

			ret = TWE_APIRET_SUCCESS;
		}
	}

	return ret;
}

/*!
 * アプリケーションＩＤ。
 * - 制約：上位WORD/下位WORDともに 0x0000/0xFFFF になってはいけない。
 * 
 * \param pMe     単一設定定義
 * \param pDatum  書き出し先のデータ構造
 * \param u16OpId 処理内容
 * \param pBuf    読み込み・書き出し用のバッファ
 * \return        成功・失敗
 */
TWE_APIRET TWESTGS_VLD_u32AppId(struct _TWESTG_sElement* pMe, TWESTG_tsDatum* psDatum, uint16 u16OpId, TWE_tsBuffer* pBuf) {
	TWE_APIRET ret = TWE_APIRET_FAIL;
	TWESTG_tuDatum* pDatum = &psDatum->uDatum;

	if (u16OpId == TWESTGS_VLD_OP_VALIDATE && pBuf->u8bufflen == 0) {
		// 未入力の場合はデフォルトに戻す
		TWESTG_vSetUDatumFrUDatum(pMe->sDatum.u8Type, pDatum, &pMe->sDatum.uDatum);
		ret = TWE_APIRET_SUCCESS;
	} else 
	if (u16OpId == TWESTGS_VLD_OP_VALIDATE) {
		if (pMe->sDatum.u8Type == TWESTG_DATATYPE_UINT32) {
			uint32 u32num = TWESTR_u32HexstrToNum(pBuf->pu8buff, pBuf->u8bufflen);
			uint16 l = u32num & 0xFFFF;
			uint16 h = (u32num >> 16) & 0xFFFF;

			if (l == 0 || l == 0xFFFF || h == 0 || h == 0xFFFF) {
				; // FAIL
			}
			else {
				pDatum->u32 = u32num;
				ret = TWE_APIRET_SUCCESS;
			}
		}
	}

	return ret;
}



/*!
 *  チャネルマスク（リスト）を入力解釈する。
 *  11,15,19 のように１０進数を区切って入力する。
 *  以下では区切り文字は任意で MAX_CHANNELS 分処理したら終了する。
 * 
 * \param pMe     単一設定定義
 * \param pDatum  書き出し先のデータ構造
 * \param u16OpId 処理内容
 * \param pBuf    読み込み・書き出し用のバッファ
 * \return        成功・失敗
 */
TWE_APIRET TWESTGS_VLD_u32ChList(struct _TWESTG_sElement* pMe, TWESTG_tsDatum* psDatum, uint16 u16OpId, TWE_tsBuffer* pBuf) {
	TWE_APIRET ret = TWE_APIRET_FAIL;
	TWESTG_tuDatum* pDatum = &psDatum->uDatum;

	if (u16OpId == TWESTGS_VLD_OP_VALIDATE && pBuf->u8bufflen == 0) {
		// 未入力の場合はデフォルトに戻す
		TWESTG_vSetUDatumFrUDatum(pMe->sDatum.u8Type, pDatum, &pMe->sDatum.uDatum);
		ret = TWE_APIRET_SUCCESS;
	} else if (u16OpId == TWESTGS_VLD_OP_VALIDATE) {
		uint32 u32chmask = 0; // 新しいチャネルマスク
		uint8* p_token[3] = { NULL, NULL, NULL };

		uint8 u8num = TWESTR_u8SplitTokens(pBuf->pu8buff, pBuf->u8bufflen_max, p_token, 3);

		int i, j = 0;
		for (i = 0; i < u8num; i++) {
			if (p_token[i] != NULL) {
				uint8 u8ch = TWESTR_i32DecstrToNum(p_token[i], (uint8)strlen((const char*)p_token[i]));

				if (u8ch >= 11
					&& u8ch <= 26
					) {
					uint32 u32bit = (1UL << u8ch);
					if (!(u32bit & u32chmask)) {
						u32chmask |= u32bit;
						j++;
					}
				}
			}
		}

		if (u32chmask != 0) {
			pDatum->u16 = (u32chmask >> 11) & 0xFFFF;
			ret = TWE_APIRET_SUCCESS;
		}
	}
	else if (u16OpId == TWESTGS_VLD_OP_CUSTDISP) {
		uint8* p = pBuf->pu8buff;
		// uint8* e = pBuf->pu8buff + pBuf->u8bufflen_max;
		uint32 u32mask = (uint32)(pDatum->u16) << 11;

		int i;
		int ct = 0;
		for (i = 11; i <= 26; i++) {
			if (u32mask & (1UL << i)) {
				if (ct) *p++ = ',';

				int l = TWE_snprintf((char*)p, 8, "%d", i);

				p += l;
				ct++;
			}
		}
		if (ct) {
			ret = TWE_APIRET_SUCCESS;
		}
	}

	return ret;
}

#if 0
/**
 * UART のオプションを表示する
 */
TWE_APIRET TWESTGS_VLD_u32UartOpt(struct _TWESTG_sElement* pMe, TWESTG_tsDatum* psDatum, uint16 u16OpId, TWE_tsBuffer* pBuf) {
	const uint8  APPCONF_UART_CONF_PARITY_MASK = 0x3;
	const uint8  APPCONF_UART_CONF_STOPBIT_MASK = 0x4;
	const uint8  APPCONF_UART_CONF_WORDLEN_MASK = 0x8;
	TWESTG_tuDatum* pDatum = &psDatum->uDatum;

	TWE_APIRET ret = TWE_APIRET_FAIL;

	if (u16OpId == TWESTGS_VLD_OP_VALIDATE) {
		int i;
		uint8 u8parity = 0;
		uint8 u8stop = 0;
		uint8 u8wordlen = 0;
		bool_t bInvalid = FALSE;

		for (i = 0; i < pBuf->u8bufflen; i++) {
			switch (pBuf->pu8buff[i]) {
			case 'N': case 'n': u8parity = 0; break;
			case 'O': case 'o': u8parity = 1; break;
			case 'E': case 'e': u8parity = 2; break;
			case '1': u8stop = 0; break;
			case '2': u8stop = APPCONF_UART_CONF_STOPBIT_MASK; break;
			case '8': u8wordlen = 0; break;
			case '7': u8wordlen = APPCONF_UART_CONF_WORDLEN_MASK; break;
			default:
				bInvalid = TRUE; // 関係ない文字列の場合はエラーにする
				break;
			}
		}

		uint8 u8new = u8parity | u8stop | u8wordlen;
		pDatum->u8 = u8new;

		ret = bInvalid ? TWE_APIRET_FAIL : TWE_APIRET_SUCCESS;
	}
	else if (u16OpId == TWESTGS_VLD_OP_CUSTDISP) {
		uint8 u8conf = pDatum->u8;

		const uint8 au8name_bit[] = { '8', '7' };
		const uint8 au8name_parity[] = { 'N', 'O', 'E', '@' };
		const uint8 au8name_stop[] = { '1', '2' };

		uint8* p = pBuf->pu8buff;
		uint8* e = pBuf->pu8buff + pBuf->u8bufflen_max;
		
		TWE_snprintf(p, 8, "%c%c%c",
			au8name_bit[u8conf & APPCONF_UART_CONF_WORDLEN_MASK ? 1 : 0],
			au8name_parity[u8conf & APPCONF_UART_CONF_PARITY_MASK],
			au8name_stop[u8conf & APPCONF_UART_CONF_STOPBIT_MASK ? 1 : 0]);

		ret = TWE_APIRET_SUCCESS;
	}
	return ret;
}
#endif

/*!
 * UARTとオプションの設定。
 *  16bit の上位４ビットをオプションに、下位１２ビットをボーレート(100bps単位)を保存する。
 * 
 * \param pMe     単一設定定義
 * \param pDatum  書き出し先のデータ構造
 * \param u16OpId 処理内容
 * \param pBuf    読み込み・書き出し用のバッファ
 * \return        成功・失敗
 */
TWE_APIRET TWESTGS_VLD_u32UartBaudOpt(struct _TWESTG_sElement* pMe, TWESTG_tsDatum* psDatum, uint16 u16OpId, TWE_tsBuffer* pBuf) {
	const uint8  APPCONF_UART_CONF_PARITY_MASK = 0x3;
	const uint8  APPCONF_UART_CONF_STOPBIT_MASK = 0x4;
	const uint8  APPCONF_UART_CONF_WORDLEN_MASK = 0x8;
	TWESTG_tuDatum* pDatum = &psDatum->uDatum;

	TWE_APIRET ret = TWE_APIRET_FAIL;

	if (u16OpId == TWESTGS_VLD_OP_VALIDATE && pBuf->u8bufflen == 0) {
		// 未入力の場合はデフォルトに戻す
		TWESTG_vSetUDatumFrUDatum(pMe->sDatum.u8Type, pDatum, &pMe->sDatum.uDatum);
		ret = TWE_APIRET_SUCCESS;
	} else 
	if (u16OpId == TWESTGS_VLD_OP_VALIDATE) {
		//uint32 u32chmask = 0; // 新しいチャネルマスク
		uint8* p_token[2] = { NULL, NULL };

		uint8 u8num = TWESTR_u8SplitTokens(pBuf->pu8buff, pBuf->u8bufflen_max, p_token, 2);

		uint16 u16set = 0x0000;

		// BAUD を検出する
		bool_t bOk = TRUE;
		if (u8num && p_token[0] != NULL) {
			// ボーレート
			uint32 u32Baud = TWESTR_i32DecstrToNum(p_token[0], (uint8)strlen((const char*)p_token[0]));
			u32Baud /= 100; // 1/100 で保存する
			if (!(u32Baud >= 96 && u32Baud < 2500)) { // 250kbps までは入力範囲とする
				bOk = FALSE;
			}

			u16set = u32Baud & 0xFFF;
		}

		// オプションの設定
		if (bOk && u8num >= 2 && p_token[1] != NULL) {
			// OPTION
			uint8 u8parity = 0;
			uint8 u8stop = 0;
			uint8 u8wordlen = 0;

			uint8* p = p_token[1];
			uint8* e = p_token[1] + 3;

			while (p < e) {
				switch (*p) {
				case 'N': case 'n': u8parity = 0; break;
				case 'O': case 'o': u8parity = 1; break;
				case 'E': case 'e': u8parity = 2; break;
				case '1': u8stop = 0; break;
				case '2': u8stop = APPCONF_UART_CONF_STOPBIT_MASK; break;
				case '8': u8wordlen = 0; break;
				case '7': u8wordlen = APPCONF_UART_CONF_WORDLEN_MASK; break;
				default:
					bOk = FALSE; // 関係ない文字列の場合はエラーにする
					break;
				}
				p++;
			}

			if (bOk) {
				uint8 u8Opt = u8parity | u8stop | u8wordlen;
				u16set |= ((uint16)u8Opt) << 12; // 上位4bitに設定を書き込む
			}
		}

		if (bOk) {
			pDatum->u16 = u16set;
			ret = TWE_APIRET_SUCCESS;
		}
	}
	else if (u16OpId == TWESTGS_VLD_OP_CUSTDISP) {
		uint8 u8conf = (pDatum->u16 & 0xF000) >> 12;
		uint16 u16hbaud = (pDatum->u16 & 0x0FFF);

		const uint8 au8name_bit[] = { '8', '7' };
		const uint8 au8name_parity[] = { 'N', 'O', 'E', '@' };
		const uint8 au8name_stop[] = { '1', '2' };

		uint8* p = pBuf->pu8buff;
		//uint8* e = pBuf->pu8buff + pBuf->u8bufflen_max;

		TWE_snprintf((char*)p, 16, "%d00,%c%c%c",
			u16hbaud,
			au8name_bit[u8conf & APPCONF_UART_CONF_WORDLEN_MASK ? 1 : 0],
			au8name_parity[u8conf & APPCONF_UART_CONF_PARITY_MASK],
			au8name_stop[u8conf & APPCONF_UART_CONF_STOPBIT_MASK ? 1 : 0]);

		ret = TWE_APIRET_SUCCESS;
	}

	return ret;
}