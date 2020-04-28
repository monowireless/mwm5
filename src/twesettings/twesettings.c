/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <string.h>

#include "twecommon.h"
#include "twesettings0.h"
#include "tweutils.h"

#include "twesettings_std_defsets.h"

#include "twecrc8.h"

#define DATATYPE_GET_LEN(c) ((c) & 0x0F) //! データ型バイトからデータ長（バイト数）を得る
#define DATATYPE_GET_TYPE(c) ((c) & 0x80 ? 0x80 : ((c) >> 4)) //! データ型バイトから型番号を得る

/*!
 * バイト列から TWESTG_tsDatum 構造体への変換（シリアル化）
 *   [0]  : MSB=0 H:データタイプ, L: データ長,
 *          MSB=1 LSBから4bit:データ長(最大16バイト)
 *   [1..]: データ長に応じたデータが格納、ビッグエンディアンの並び。
 * 
 * 文字列型の場合は、au8Bytes 中の領域へのポインタを格納する
 * 
 * \param au8Bytes 入力データバイト
 * \param pDat     出力先のデータ構造体(u8Stat, u8Opt は変更しない)
 * \return         0
 */
static int s_iConvBytesToDatum(TWESTG_tsDatum *pDat, const uint8 *au8Bytes) {
	const uint8 *p = au8Bytes;
	uint8 c;
    
    uint8 u8DataType = 0;
    uint8 u8DataLen = 0;
    
    // H: DataType, L: DataLen
    c = TWE_G_OCTET(p);
	u8DataLen = DATATYPE_GET_LEN(c);
	u8DataType = DATATYPE_GET_TYPE(c);
    
    pDat->u8Type = u8DataType;
    pDat->u8Len = u8DataLen;

    switch (u8DataType) {
        case TWESTG_DATATYPE_INT8:
            pDat->uDatum.i8 = (int8)TWE_G_OCTET(p);
            break;
        case TWESTG_DATATYPE_UINT8:
            pDat->uDatum.u8 = (uint8)TWE_G_OCTET(p);
            break;
        case TWESTG_DATATYPE_INT16:
            pDat->uDatum.i16 = (int16)TWE_G_WORD(p);
            break;
        case TWESTG_DATATYPE_UINT16:
            pDat->uDatum.u16 = (uint16)TWE_G_WORD(p);
            break;
        case TWESTG_DATATYPE_INT32:
            pDat->uDatum.i32 = (int32)TWE_G_DWORD(p);
            break;
        case TWESTG_DATATYPE_UINT32:
            pDat->uDatum.u32 = (uint32)TWE_G_DWORD(p);
            break;
        case TWESTG_DATATYPE_STRING:
            pDat->uDatum.pu8 = (uint8*)p; // const 対策
            break;
        default:
            break;
    }

    return 0;
}

/*!
 * TWESTG_tsDatum からバイト列への変換（シリアル化）
 * 
 * \param pDat     入力データ構造
 * \param au8Bytes 出力先のデータ構造体
 * \param pBufEnd  入力データの末尾＋１バイト
 * \return         書き出されたバッファ長
 */
static int s_iConvDatumToBytes(uint8 *au8Bytes, TWESTG_tsDatum *pDat,  uint8 *pBufEnd) {
    uint8 *q = au8Bytes;

	// バッファー不足で終了
	if (pDat->u8Len + q > pBufEnd) return 0;
	
    switch (pDat->u8Type) {
        case TWESTG_DATATYPE_INT8:
            TWE_S_OCTET(q, ((TWESTG_DATATYPE_INT8<<4) | 1));
            TWE_S_OCTET(q, pDat->uDatum.i8);
            break;
        case TWESTG_DATATYPE_UINT8:
            TWE_S_OCTET(q, ((TWESTG_DATATYPE_UINT8<<4) | 1));
            TWE_S_OCTET(q, pDat->uDatum.u8);
            break;
        case TWESTG_DATATYPE_INT16:
            TWE_S_OCTET(q, ((TWESTG_DATATYPE_INT16<<4) | 2));
            TWE_S_WORD(q, pDat->uDatum.i16);
            break;
        case TWESTG_DATATYPE_UINT16:
            TWE_S_OCTET(q, ((TWESTG_DATATYPE_UINT16<<4) | 2));
            TWE_S_WORD(q, pDat->uDatum.u16);
            break;
        case TWESTG_DATATYPE_INT32:
            TWE_S_OCTET(q, ((TWESTG_DATATYPE_INT32<<4) | 4));
            TWE_S_DWORD(q, pDat->uDatum.i32);
            break;
        case TWESTG_DATATYPE_UINT32:
            TWE_S_OCTET(q, ((TWESTG_DATATYPE_UINT32<<4) | 4));
            TWE_S_DWORD(q, pDat->uDatum.u32);
            break;
        case TWESTG_DATATYPE_STRING: // 可変長バイト
            TWE_S_OCTET(q, (TWESTG_DATATYPE_STRING | pDat->u8Len));
            TWE_S_STRING(q, pDat->uDatum.pu8, pDat->u8Len);
            break;
        default:
            break;
    }

    return (uint8)(q - au8Bytes);
}

/*!
 * tsDatum の値を得る (UINT32キャスト)
 * \param pDat tsDatum
 */
uint32 TWESTG_u32GetU32FrUDatum(uint8 u8Type, TWESTG_tuDatum *pDat) {
	switch (u8Type) {
        case TWESTG_DATATYPE_INT8:		return (uint32)pDat->i8;
        case TWESTG_DATATYPE_UINT8:		return (uint32)pDat->u8;
        case TWESTG_DATATYPE_INT16:		return (uint32)pDat->i16;
        case TWESTG_DATATYPE_UINT16:	return (uint32)pDat->u16;
        case TWESTG_DATATYPE_INT32:		return (uint32)pDat->i32;
        case TWESTG_DATATYPE_UINT32:	return (uint32)pDat->u32;
        default:
            return 0;
    }
}

/*!
 * tsDatum の値を入れる
 * \param pDat tsDatum
 */
void TWESTG_vSetUDatumFrU32(uint8 u8Type, TWESTG_tuDatum *pDat, uint32 u32val) {
	pDat->u32 = 0; // clear zero (just in case)

	switch (u8Type) {
        case TWESTG_DATATYPE_INT8:		pDat->i8 = (int8)u32val; break;
        case TWESTG_DATATYPE_UINT8:		pDat->u8 = (uint8)u32val; break;
        case TWESTG_DATATYPE_INT16:		pDat->i16 = (int16)u32val; break;
        case TWESTG_DATATYPE_UINT16:	pDat->u16 = (uint16)u32val; break;
        case TWESTG_DATATYPE_INT32:		pDat->i32 = (int32)u32val; break;
        case TWESTG_DATATYPE_UINT32:	pDat->u32 = (uint32)u32val; break;
        default:
			break;
    }
}

/*!
 * tsDatum の値を入れる
 * \param pDat tsDatum
 */
void TWESTG_vSetUDatumFrUDatum(uint8 u8Type, TWESTG_tuDatum* pDat, const TWESTG_tuDatum* pRef) {
	switch (u8Type) {
	case TWESTG_DATATYPE_INT8:		pDat->i8 = pRef->i8; break;
	case TWESTG_DATATYPE_UINT8:		pDat->u8 = pRef->u8; break;
	case TWESTG_DATATYPE_INT16:		pDat->i16 = pRef->i16; break;
	case TWESTG_DATATYPE_UINT16:	pDat->u16 = pRef->u16; break;
	case TWESTG_DATATYPE_INT32:		pDat->i32 = pRef->i32; break;
	case TWESTG_DATATYPE_UINT32:	pDat->u32 = pRef->u32; break;
	default:
		break;
	}
}

/*!
 * 設定データ集合からIDが一致するデータ(tsDatum)を検索する。
 * 
 * \param au8Data 設定データ集合
 * \param u16Id 探したいID
 * \return データ列へのポインタ（IDの次のバイト,DATATYPEから）
 */
static const uint8 *s_pu8FindId(const uint8 *au8Data, uint16 u16Id) {
    const uint8 *p = au8Data, *e = NULL;

    // すでに設定項目が記録されているかチェックする
    uint8 u8Len = TWE_G_OCTET(p);

    e = p + u8Len; // 末尾＋１ポインタ

    while(p < e) {
        uint8 u8Id = TWE_G_OCTET(p);
		uint8 c = TWE_G_OCTET(p);

		uint8 u8Type = DATATYPE_GET_TYPE(c); (void)u8Type;
		uint8 u8DataLen = DATATYPE_GET_LEN(c);

        if (u8Id == (u16Id & 0xFF)) {
			p--; // ポインタを TYPE, LEN のバイトの位置に戻す
            break;
        }

        p += u8DataLen;
    }

    return (p < e) ? p : NULL;
}

#if 0
/*!
 * バイト列のデータ定義から ID に適合したデータを抽出
 *
 * \param pSet
 * \param psSettings
 * \param u16Id
 * \return 
 */
const TWESTG_tsElement *s_FindSettingElement(TWESTG_tsSlice *pSet, TWESTG_tsSettings *psSettings, uint16 u16Id) {
    const TWESTG_tsElement *pEle = NULL;
	const TWESTG_tsElement* apEle[4] = { psSettings->pSlot, psSettings->pBoard, psSettings->pBase, NULL };

	int i;
	for (i = 0; i < 3; i++) {
		pEle = apEle[i];
		if (pEle != NULL) {
			do {
				if (pEle->u16Id == u16Id) {
					return pEle;
				}
				pEle++;
			} while (pEle->u16Id != E_TWESTG_DEFSETS_VOID);
		}
	}
    
    return NULL;
}
#endif

/*!
 * TWE_tsBufferを初期化する
 * 
 * \param pBuf 初期化する汎用バッファ
 * \param pu8Buf 確保済みのバイト列へのポインタ
 * \param u8Len バッファ長（使用済みの領域）
 * \param u8MaxLen バッファ最大長
 */
void TWE_vInit_tsBuffer0(TWE_tsBuffer *pBuf, uint8 *pu8Buf, uint8 u8Len, uint8 u8MaxLen) {
    pBuf->pu8buff = pu8Buf;
    pBuf->u8bufflen = 0;
    pBuf->u8bufflen_max = u8MaxLen;
}

/*!
 * 確定設定リスト(TWESTG_tsFinal 構造体)を初期化する。
 * TWESTG_INIT_FINAL()マクロ参照。
 * 
 * \param psFinal           初期化対象
 * \param pDef              設定リスト（読み出し）
 * \param aDatum            要素データ配列（設定保存用）
 * \param pBuf              STRING型データ格納バッファ
 * \param apEle             単一設定定義へのリスト（設定保存用）
 * \param pShortcutKeys     インタラクティブモードでのショートカットキー
 * \param auDatumCust       カスタムデフォルトの定義最大数（デフォルト値の保存用）
 * \param u8CustDefCountMax auDatumCust[]配列の要素数
 * \param u8DataCountMax    設定最大数＝配列要素数
 */
void TWESTG_vInit_tsFinal(TWESTG_tsFinal *psFinal, TWESTG_tsSettings *pDef, TWESTG_tsDatum *aDatum, TWE_tsBuffer *pBuf, TWESTG_tsElement **apEle, uint8 *pShortcutKeys, uint8 u8DataCountMax, TWESTG_tuDatum *auDatumCust, uint8 u8CustDefCountMax) {
	memset(psFinal, 0, sizeof(TWESTG_tsFinal));
    psFinal->psSettings = pDef;

    psFinal->u8DataCountMax = u8DataCountMax;
    psFinal->u8DataCount = 0;
	psFinal->asDatum = aDatum;
	psFinal->apEle = apEle;
	psFinal->auDatumCustDef = auDatumCust;
	psFinal->u8CustDefCountMax = u8CustDefCountMax;
	psFinal->au8ShortcutKeys = pShortcutKeys;
	// memset(psFinal->apEle, 0, sizeof(TWESTG_tsElement *) * u8DataCountMax);

    psFinal->psBuf = pBuf;
}

#if 0
/*!
 * 設定定義（TWESTG_tsSettings）の初期化を行う
 * 
 * \param pStg  初期化大賞
 * \param pBase
 * \param pBrd
 * \param pSet
 */
void TWESTG_vInit_tsSettings(TWESTG_tsSettings *pStg, TWESTG_tsElement *pBase, TWESTG_tsElement *pBrd, TWESTG_tsElement *pSet) {
	pStg->pBase = pBase;
	pStg->pBoard = pBrd;
	pStg->pSlot = pSet;
}
#endif

/*!
 * セーブ形式(tsSlice)の初期化
 * 
 * \param pSet 初期化対象
 */
void TWESTG_vInit_tsSlice(TWESTG_tsSlice *pSet) {
    memset(pSet, 0, sizeof(TWESTG_tsSlice));
	pSet->psBuff = &pSet->sBuff;
}

/*!
 * セーブ形式(tsSlice)に設定データをロードする
 * 
 * 本関数ではヘッダ情報の読み出しとバッファへのポインタなどの情報を格納するのみで、
 * データの妥当性や解釈は行わない。
 * 
 * Note: 本関数終了後 TWESTG_u32FinalLoad() よりバッファを解釈する
 * 
 * \param pSet 読み出し先
 * \param pBuf バイト列[シリアル化設定データ]
 * \return 
 */
TWE_APIRET TWESTG_u32SliceLoad(TWESTG_tsSlice *pSet, TWE_tsBuffer *pBuf) {
	uint8 *p = pBuf->pu8buff;
    //uint8 u8datalen;

    // ヘッダの読み出し
    uint8 u8Dataformat = TWE_G_OCTET(p); // 先頭バイトはデータ形式を示すID
    if (u8Dataformat != TWESTG_FORMAT_VER_0x01) return TWE_APIRET_FAIL;

    pSet->u8KeyAppId = TWE_G_OCTET(p);
    pSet->u8SettingsVer = TWE_G_OCTET(p);
    pSet->u8SettingsKind = TWE_G_OCTET(p);
    pSet->u8SettingsSlot = TWE_G_OCTET(p);

	// pBuf の必要部分のポインタとサイズを保存する。
	pSet->psBuff->pu8buff = p;
	pSet->psBuff->u8bufflen = pBuf->u8bufflen - 5;
	pSet->psBuff->u8bufflen_max = pBuf->u8bufflen_max - 5;

	return TWE_APIRET_SUCCESS;
}

/*!
 * [セーブ形式]のヘッダやバッファの準備を行う。
 * - 事前確保されたバッファ(pBuf)にヘッダ情報を書き込む
 * - セーブ形式(pSet)のバッファ情報に pBuf の部分列を指定する
 *   本関数終了後、TWESTG_u32FinalSave() にて [シリアル化要素データ集合] を追記する。
 * 
 * \param pBuf 書き出し先バッファ
 * \param pSet pBuf の部分列が指定される
 * \param u8FormatVer セーブ形式のフォーマット書式
 * \return
 */
TWE_APIRET TWESTG_u32SliceSavePrep(TWE_tsBuffer *pBuf, TWESTG_tsSlice *pSet, uint8 u8FormatVer) {
    uint8 *q = pBuf->pu8buff;
    //uint8 *e_buff = pBuf->pu8buff + pBuf->u8bufflen_max;

    // ヘッダの書き出し
	if (u8FormatVer == TWESTG_FORMAT_VER_0x01) {
		TWE_S_OCTET(q, TWESTG_FORMAT_VER_0x01);
		TWE_S_OCTET(q, pSet->u8KeyAppId);
		TWE_S_OCTET(q, pSet->u8SettingsVer);
		TWE_S_OCTET(q, pSet->u8SettingsKind);
		TWE_S_OCTET(q, pSet->u8SettingsSlot);

		pSet->psBuff->pu8buff = q;
		TWE_S_OCTET(q, 0); // ペイロードサイズ

		pSet->psBuff->u8bufflen = 0;
		pSet->psBuff->u8bufflen_max = pBuf->u8bufflen_max - 5;
		return TWE_APIRET_SUCCESS;
	}

	return TWE_APIRET_FAIL;
}

/**
 * 単一設定定義(pEle)から、確定設定リスト(apEle, pDatum)を作成する。
 * 
 *  - 同じ設定IDのアイテムは上書きする
 *  - カスタムデフォルト(pCustomDefault)があるデータについてはデフォルト値を変更する
 *    未使用指定(TWESTG_DATATYPE_UNUSE)の場合は、そのIDの設定は確定設定リストから除外する。
 *  - pDatum[].u8Stat に状態ビットを設定する（設定階層、カスタムデフォルト）
 *
 * @param apEle    生成される設定リスト
 * @param pDatum   格納データのリスト（デフォルト値）
 * @param u8OrgLv  設定レベル(0-2)。設定リストは親子関係があり親元から順に設定が追加上書きされる 0:大本の親 1:アプリケーションやハード特有 2:スロットによる設定選択
 * @param u8maxele apEle, pDatum の最大格納数
 * @param pCustomDefault カスタム設定（プリセットの設定リストを用いる場合に ApplicationID のデフォルト値の書き換えに使う
 * @param puDatumCust  カスタムデフォルトのデフォルト値を保存
 * @return 
 */
TWE_APIRET s_u32ElementsListAdd(const TWESTG_tsElement** apEle, TWESTG_tsDatum *pDatum, uint8 u8OrgLv, const TWESTG_tsElement *pEle, uint8 u8maxele, const uint8 *pCustomDefault, TWESTG_tuDatum *puDatumCust) {
	if (pEle != NULL) {
		const TWESTG_tsElement *p = pEle;

		while (p->u16Id != E_TWESTG_DEFSETS_VOID) {
			int i = 0;

			// 配列 apEle[] 中でエントリの格納場所を探索し i にインデックスを保存する。
			// 既に登録されていれば、このインデックス
			// 未登録なら末尾
			while (i < u8maxele && apEle[i] != NULL) {
				// 同じエントリが見つかったので、この i の位置に上書きする
				if (apEle[i]->u16Id == p->u16Id) {
					break;
				}
				i++;
			}

			if (i < u8maxele) {
				// 設定リスト(pEle)のポインタをコピーする
				apEle[i] = p;

				// デフォルト値を pDatum に格納する
				pDatum[i] = p->sDatum;
				puDatumCust[i].u32 = 0; // カスタムデフォルトはクリアする

				// 下位２ビットは設定データの大本を示し、u8SetOrigin を書き込む
				pDatum[i].u8Stat &= (0xFF - TWESTG_DATASTAT_ORIGIN_MASK);
				pDatum[i].u8Stat |= (u8OrgLv & TWESTG_DATASTAT_ORIGIN_MASK);

				// カスタムデフォルトの設定ビットは抹消
				pDatum[i].u8Stat &= ~TWESTG_DATASTAT_CUSTOM_DEFAULT_MASK;
				pDatum[i].u8Opt = 0;
			}

			p++;
		}
	}

	// カスタムデフォルトの反映
	if(pCustomDefault != NULL) {
		const uint8 *p = pCustomDefault + 1;
		const uint8 *e = p + *pCustomDefault; // 末尾

		while (p < e) {
			uint8 u8ld = TWE_G_OCTET(p);
			uint8 u8datty = TWE_G_OCTET(p);

			// 未使用フラグを発見
			int i = 0;
			while (i < u8maxele && apEle[i] != NULL) {
				if (apEle[i]->u16Id == u8ld) {
					if (u8datty == TWESTG_DATATYPE_UNUSE) {
						// このエントリを抹消する

						// i の位置のデータを未使用にする
						//
						//  - 一つ先に有効データがあれば、手前に上書きする。
						//  - 一つ先に有効データがなくなれば、最後の有効データまで処理が終わっている
						//    （同じ末尾データが２つ並んでいる）ので、末尾データを削除する。
						int j = i + 1;
						while (j < u8maxele && apEle[j] != NULL) {
							apEle[j - 1] = apEle[j];
							pDatum[j - 1] = pDatum[j];
							puDatumCust[j - 1] = puDatumCust[j];
							j++; 
						}
						apEle[j - 1] = NULL;
						pDatum[j - 1].u8Type = TWESTG_DATATYPE_UNSET;
						puDatumCust[j - 1].u32 = 0;

						break;
					}
					else { // カスタムデフォルトがある場合はその値を採用
						// データの有無をチェック
						uint8 u8len = DATATYPE_GET_LEN(u8datty);
						if (u8len == 0) return TWE_APIRET_FAIL; // これはエラー

						s_iConvBytesToDatum(&pDatum[i], p - 1);
						puDatumCust[i] = pDatum[i].uDatum;

						// 下位２ビットは設定データの大本を示し、u8SetOrigin を書き込む
						pDatum[i].u8Stat &= (0xFF - TWESTG_DATASTAT_ORIGIN_MASK);
						pDatum[i].u8Stat |= (u8OrgLv & TWESTG_DATASTAT_ORIGIN_MASK);
						pDatum[i].u8Stat |= TWESTG_DATASTAT_CUSTOM_DEFAULT_MASK;
						pDatum[i].u8Opt = 0x80; //カスタムデフォルトありますビット。

						p += u8len;
					}
				}
	
				i++; // while
			}
		}
	}

    return TWE_APIRET_SUCCESS;
}

/*!
 * 設定定義(psFinal->psSettings)より確定設定リストを作成する。
 * 
 * 階層化された設定を配列化することで、確定設定リストとなる。
 * (TODO: ソートしなきゃならんか？）
 * 
 * このリストの要素は psFinal->apEle[] 配列で、配列末尾か NULL ポインタが格納された場所で終了である。
 * このリストにはデフォルト値(psFinal->asDatum)が格納される。
 * 
 * \param psFinal 初期化・諸情報設定済みの確定設定リスト(tsFinal)
 * \return 
 */
TWE_APIRET TWESTG_u32FinalSetDefaults(TWESTG_tsFinal *psFinal) {
	const TWESTG_tsElement** apEle = (const TWESTG_tsElement**)psFinal->apEle;
	int i;

	// エラーチェック
	if (psFinal->psSettings == NULL)
		return TWE_APIRET_FAIL;

	// カスタムデフォルトの値保存用の配列
	TWESTG_tuDatum puCustDef[TWESTG_DATA_COUNT_MAX];
	memset(puCustDef, 0, sizeof(TWESTG_tuDatum *) * TWESTG_DATA_COUNT_MAX);

	// 領域クリア
	memset(psFinal->apEle, 0, sizeof(TWESTG_tsElement*) *  psFinal->u8DataCountMax);

	s_u32ElementsListAdd(apEle, psFinal->asDatum, 0, psFinal->psSettings->pBase, psFinal->u8DataCountMax, psFinal->psSettings->au8BaseCustomDefault, puCustDef); // LV0 設定(BASE)
	s_u32ElementsListAdd(apEle, psFinal->asDatum, 1, psFinal->psSettings->pBoard, psFinal->u8DataCountMax, psFinal->psSettings->au8BoardCustomDefault, puCustDef); // LV1 設定(BOARD)
	s_u32ElementsListAdd(apEle, psFinal->asDatum, 2, psFinal->psSettings->pSlot, psFinal->u8DataCountMax, psFinal->psSettings->au8SlotCustomDefault, puCustDef); // LV2 設定(SLOT)

	// その他後処理
	uint8 u8custidx = 0;
	for (i = 0; i < psFinal->u8DataCountMax; i++) {
		if (apEle[i] == NULL) break; // 常に末尾は NULL (または配列の末端）
		psFinal->au8ShortcutKeys[i] = apEle[i]->sFormat.u8ShortcutKey; // ショートカットキーを配列に格納する
		psFinal->u8DataCount++; // データカウント

		// カスタムデフォルト
		if (psFinal->asDatum[i].u8Stat & TWESTG_DATASTAT_CUSTOM_DEFAULT_MASK) {
			if (u8custidx < psFinal->u8CustDefCountMax) {
				psFinal->auDatumCustDef[u8custidx] = puCustDef[i]; // 管理領域にデータをコピーする
				psFinal->asDatum[i].u8Opt = 0x80 + u8custidx; // インデックス情報を保存する

				u8custidx++;
			}
			else {
				// 配列オーバー
				psFinal->asDatum[i].u8Opt = 0;
			}
		}
	}

	// STRING バッファの割当
	{
		uint8_t *p; uint8_t *e;
		p = psFinal->psBuf->pu8buff;
		e = psFinal->psBuf->pu8buff + psFinal->psBuf->u8bufflen_max;
		memset(p, 0, psFinal->psBuf->u8bufflen_max); // clear buffer 
		for (i = 0; i < psFinal->u8DataCount; i++) {
			if (psFinal->asDatum[i].u8Type == TWESTG_DATATYPE_STRING) {
				int len = psFinal->apEle[i]->sDatum.u8Len;
				if (len > 0) {
					const uint8_t *p_def = psFinal->asDatum[i].uDatum.pu8;
					if (p + len < e) {
						psFinal->asDatum[i].uDatum.pu8 = p;
						p += len;
						if (p_def != NULL) {
							// copy default setting
							memcpy(psFinal->asDatum[i].uDatum.pu8, p_def, len);
						}
					} else {
						psFinal->asDatum[i].uDatum.pu8 = NULL; // error: insufficient pre-allocated buffer	
					}
				} else {
					psFinal->asDatum[i].uDatum.pu8 = NULL; // error settings
				}
			}
		}

		// update the length
		psFinal->psBuf->u8bufflen = (uint8_t)(p - psFinal->psBuf->pu8buff);
	}

	return TWE_APIRET_SUCCESS;
}

/*!
 * セーブ形式(psSlice)から確定設定リスト(psFinal)を構築する（セーブデータのロード処理）。
 * 
 * - psSlice は事前に TWESTG_u32SliceLoad() 手続きが終わっていること。
 * - psFinal は事前に初期化手続きが終わっておりデフォルトのリストが構築されてること。
 * - SLOT0 のデータも反映させる場合は、まず SLOT0 のデータにて本関数を呼び出し、続けて
 *   対象SLOTのデータにて本関数を呼び出すこと。
 *   参照: TWESTG_u32LoadDataFrAppstrg()
 *
 * \param psFinal 初期化処理の終わった(デフォルトのリストが構築されている)確定設定リスト(psFinal)
 * \param psSlice セーブ形式
 * \param u32Opt  諸オプション (TWESTG_LOAD_OPT_DONTAPPLY_LV2: SLOT0で設定適用しない条件向け)
 * \return 
 */
TWE_APIRET TWESTG_u32FinalLoad(TWESTG_tsFinal *psFinal, TWESTG_tsSlice *psSlice, uint32 u32Opt) {
	int i;

	TWESTG_tsElement** apEle = psFinal->apEle;
	TWESTG_tsElement *pE;

	// エラーチェック
	if (psFinal->psSettings == NULL || apEle[0] == NULL)
		return TWE_APIRET_FAIL;

	// 設定値を psFinal にコピーしていく
	for (i = 0; i < psFinal->u8DataCount; i++) {
		pE = apEle[i];

		if (pE != NULL) { // 念のため
			// データを探索
			const uint8 *pu8dat;

			// 値を設定 (pu8dat がある場合は、そちらを、ない場合はデフォルト値を採用する)
			TWESTG_tsDatum *pD = &psFinal->asDatum[i];

			// LV2 設定の場合
			uint8 u8OrgLv = pD->u8Stat & 0x3;
			if (u32Opt & TWESTG_LOAD_OPT_DONTAPPLY_LV2 && u8OrgLv == 2) {
				// LV2 設定に対しては LV0, LV1 設定はロードしない
				//  (PAL の DIP での ID 設定対応)
				continue;
			}

			// ロードデータに対象のデータが含まれるか検索する
			pu8dat = s_pu8FindId(psSlice->psBuff->pu8buff, pE->u16Id);

			if (pu8dat) { // セーブ済みのデータが見つかった
				// バイト列からの変換
				if (*pu8dat & TWESTG_DATATYPE_STRING) { // 文字型
					// バイト列
					pD->u8Len = DATATYPE_GET_LEN(*pu8dat);
					pD->u8Type = TWESTG_DATATYPE_STRING;

					memcpy(pD->uDatum.pu8, pu8dat + 1, pD->u8Len);
				}
				else { // 数値型
					s_iConvBytesToDatum(pD, pu8dat);
				}

				// 設定データが見つかった場合は、セーブマスクを設定（末端の場合はセーブ対象）
				pD->u8Stat &= ~TWESTG_DATASTAT_SAVED_HIGHER_MASK; // HIGHER マスクは消しておく
				if (u32Opt & TWESTG_LOAD_OPT_LOAD_AS_UNSAVED) {
					pD->u8Stat |= TWESTG_DATASTAT_MODIFIED_MASK;
				}
				else {
					pD->u8Stat |= TWESTG_DATASTAT_SAVED_MASK;
				}
			}	
		}
	}

	return TWE_APIRET_SUCCESS;
}

/**
 * 確定設定リスト(psFinal）を シリアル化要素データ集合として書き出す(psSlice->psBuff) 。(セーブ処理)
 * 
 * - u32Opt & TWESTG_SAVE_OPT_INCLUDE_HIGHER_SETTINGS: 
 *   書き出し時に SLOT0 で設定済みの設定を自スロットの設定として書き出す。
 * - 
 * 
 * @param psSet 格納先
 * @param psFinal 読み出し元
 * @param u32Opt 書き出しオプション
 */
TWE_APIRET TWESTG_u32FinalSave(TWESTG_tsSlice *psSet, TWESTG_tsFinal *psFinal, uint32 u32Opt) {
	int i;
	bool_t bStat = TRUE;

	uint8 u8SaveCount = 0;
	uint8 *q = psSet->psBuff->pu8buff + 1; // 先頭バイトはペイロード長なので飛ばしておく
	uint8 *qs = q;
	uint8 *qe = psSet->psBuff->pu8buff + psSet->psBuff->u8bufflen_max; // ポインタ（末尾バイトの次）

	/// 設定値を psFinal にコピーしていく
	// コピー対象のエントリ抽出条件
	uint32 u32MaskStat = TWESTG_DATASTAT_SAVED_MASK | TWESTG_DATASTAT_MODIFIED_MASK;
	// TWESTG_SAVE_OPT_INCLUDE_HIGHER_SETTINGSが指定された場合は、上位セーブデータの内容も適用する
	if (u32Opt & TWESTG_SAVE_OPT_INCLUDE_HIGHER_SETTINGS) u32MaskStat |= TWESTG_DATASTAT_SAVED_HIGHER_MASK;

	// 各エントリについて書き出し処理を行う
	for (i = 0; i < psFinal->u8DataCount; i++) {
		TWESTG_tsDatum *pD = &psFinal->asDatum[i];
		TWESTG_tsElement *pE = psFinal->apEle[i];

		if (pD->u8Stat & u32MaskStat) {
			// デフォルトと同じなら書き出しは不要
			if (!(pD->u8Stat & TWESTG_DATASTAT_SAVED_HIGHER_MASK)) { // 上位のセーブデータがある場合は書き出しの省略はしない
				if (pE->sDatum.u8Type != TWESTG_DATATYPE_STRING) { // 文字列型は対象としない
					TWESTG_tuDatum *pd0 = 0;

					// 数値型
					if (pD->u8Stat & TWESTG_DATASTAT_CUSTOM_DEFAULT_MASK) {
						// カスタムデフォルト
						if (pD->u8Opt & 0x80) {
							// デフォルトの設定値が保存されている場合はチェックを行う
							pd0 = &psFinal->auDatumCustDef[pD->u8Opt & 0x0F];
						}
					}
					else {
						// 通常（カスタムデフォルトの設定なし）
						pd0 = &pE->sDatum.uDatum;
					}

					if (pd0 != NULL) {
						uint32 u32New = TWESTG_u32GetU32FrUDatum(pD->u8Type, pd0);
						uint32 u32Def = TWESTG_u32GetU32FrUDatum(pD->u8Type, &pD->uDatum);

						// 設定値とデフォルト値が一致する場合は、セーブデータとしての出力はしない
						if (u32New == u32Def) {
							continue;
						}
					}
				}
			}

			// IDを格納
			uint8 u8Id = (uint8)pE->u16Id;
			uint8 *q_id = q++; // ID用のバイト位置を保存
			
			// データを格納
			uint8 len = (uint8)s_iConvDatumToBytes(q, pD, qe);
			if (len > 0) { // len=0ならバッファ不足等でエラー、len>0ならバッファ境界内にデータが保管された
				*q_id = u8Id;  // ID値を書き込む
				q += len;      // ポインタを進める
				u8SaveCount++; // 保存データ数をカウントアップする
			}
			else {
				bStat = FALSE;
				break;
			}
		}
	}

	uint8 u8paylen = (uint8)(q - qs);
	psSet->psBuff->pu8buff[0] = u8paylen; // ペイロード長を格納する
	psSet->psBuff->u8bufflen = u8paylen + 1; // バッファサイズは１バイトだけ大きい

	return bStat ? TWE_APIRET_SUCCESS_W_VALUE(u8SaveCount) : TWE_APIRET_FAIL_W_VALUE(u8SaveCount);
}
