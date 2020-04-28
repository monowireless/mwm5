/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <string.h>

#include "twecommon.h"
#include "twecrc8.h"
#include "tweutils.h"
#include "twesettings0.h"
#include "twesettings_std.h"

#include "twesettings_weak.h"

 /*!
  * アプリケーション管理の不揮発性メモリからデータをロードする。
  * - psFinal は TWESTG_u32SetBaseInfoToFinal() TWESTG_u32SetSettingsToFinal() による初期化は終わっていること。
  * 
  * \param psFinal     設定データを保管する確定設定リスト(tsFinal)、事前に初期化が終わっていること。
  * \param u8kind      種別
  * \param u8slot      スロット
  * \param u32AppId    APPID
  * \param u8VerCompat 互換性のあるバージョン
  * \param u32Opt      オプション
  * \return 成功・失敗
  */
TWE_APIRET TWESTG_u32LoadDataFrAppstrgSingle(TWESTG_tsFinal *psFinal, uint8 u8kind, uint8 u8slot, uint32 u32AppId, uint8 u8VerCompat, uint32 u32Opt) {
	TWE_APIRET bRet = TWE_APIRET_FAIL;
	TWE_APIRET apiRet = TWE_APIRET_FAIL;

	// ロード用のローカルバッファ
	uint8 u8Buff[TWESTG_STD_SAVEBUFF_SIZE];
	TWE_tsBuffer sBuff;
	TWE_vInit_tsBuffer(&sBuff, u8Buff, 0, TWESTG_STD_SAVEBUFF_SIZE); // セーブデータ保存先のバッファ

	// ロードデータ（デフォルト 0)
	TWESTG_tsSlice sCst;
	TWESTG_vInit_tsSlice(&sCst);

	// SAVE HIGHER FLAGを設定する
	int i;
	for (i = 0; i < psFinal->u8DataCount; i++) {
		TWESTG_tsDatum *pD = &psFinal->asDatum[i];

		// セーブマスクをクリアする
		if (pD->u8Stat & TWESTG_DATASTAT_SAVED_MASK) {
			pD->u8Stat &= ~TWESTG_DATASTAT_SAVED_MASK; // セーブマスクのクリア
			pD->u8Stat |= TWESTG_DATASTAT_SAVED_HIGHER_MASK; // 替わりに上位でロードされたマスク
		}
	}
	
	// データロードせずに終了
	if (u32Opt & TWESTG_STD_OPT_NOLOAD) {
		return TWE_APIRET_SUCCESS;
	}

	// データロード
	apiRet = TWESTG_cbu32LoadSetting(&sBuff, u8kind, u8slot, 0, psFinal); // 外部定義のロード関数を呼び出す
	if (TWE_APIRET_IS_SUCCESS(apiRet)) {
		// バッファからのデータ解釈を行う (sCst に sBuff から解釈済みの必要領域を切り出す）
		apiRet = TWESTG_u32SliceLoad(&sCst, &sBuff);

		if (TWE_APIRET_IS_SUCCESS(apiRet)) {
			uint8 u8KeyAppId;

			// アプリケーションIDやバージョンのチェック
			u8KeyAppId = TWE_CRC8_u8CalcU32(u32AppId);

			if (sCst.u8KeyAppId == u8KeyAppId 
				&& sCst.u8SettingsVer >= u8VerCompat
				&& sCst.u8SettingsKind == u8kind
				&& sCst.u8SettingsSlot == u8slot
				) {
				// 設定種別のデータを設定する
				apiRet = TWESTG_u32FinalLoad(psFinal, &sCst, u32Opt);

				if (TWE_APIRET_IS_SUCCESS(apiRet)) {
					bRet = TWE_APIRET_SUCCESS;
				}
			}
		}
	}

	// Returns
	return bRet;
}

/**
 * データセーブを行う。
 * - セーブデータ用のローカルバッファを確保(128+16バイト)し、17バイト目からローカルバッファにデータを書き込む。
 * - TWESTG_cbu32SaveSetting()関数を呼び出す。
 *   先頭から16バイトは上記関数での利用（更にヘッダを追加したり、チェックサムを書き込む、など）を想定している。
 */
TWE_APIRET TWESTG_u32SaveDataToAppStrg(TWESTG_tsFinal *pFinal, uint8 u8kind, uint8 u8slot, uint32 u32AppId, uint8 u8Ver, uint32 u32Opt) {
	TWE_APIRET apiRet = TWE_APIRET_FAIL;
	TWE_APIRET bRet = TWE_APIRET_FAIL;

	// セーブ用のローカルバッファ
	uint8 u8Buff[TWESTG_STD_SAVEBUFF_SIZE + 16];
	
	// セーブ用構造体
	TWESTG_tsSlice sCst;
	TWESTG_vInit_tsSlice(&sCst);

	// 必要なデータの生成
	sCst.u8KeyAppId = TWE_CRC8_u8CalcU32(u32AppId);
	sCst.u8SettingsVer = u8Ver;
	sCst.u8SettingsKind = u8kind;
	sCst.u8SettingsSlot = u8slot;

	// セーブ領域の準備
	uint8 *p = u8Buff + 16; // 16バイト分後ろを利用できるようにバッファを確保しておく（セーブ時のヘッダ等を追加する場合）
	TWE_tsBuffer sBuff;
	TWE_vInit_tsBuffer(&sBuff, p, 0, TWESTG_STD_SAVEBUFF_SIZE); // セーブデータ保存先のバッファ (16バイト進めたところ)

	TWESTG_u32SliceSavePrep(&sBuff, &sCst, TWESTG_FORMAT_VER_0x01);
	apiRet = TWESTG_u32FinalSave(&sCst, pFinal, u32Opt); // ペイロードの準備

	if(TWE_APIRET_IS_FAIL(apiRet)) {
		// デフォルトに戻す
		TWESTG_cbu32SaveSetting(NULL, u8kind, u8slot, u32Opt, pFinal);
	}
	else {
		// セーブ用のコールバック関数の呼び出し
		sBuff.u8bufflen = (uint8)((sCst.psBuff->pu8buff + sCst.psBuff->u8bufflen) - sBuff.pu8buff);
		TWE_APIRET apiRet = TWESTG_cbu32SaveSetting(&sBuff, u8kind, u8slot, u32Opt, pFinal);

		bRet = TWE_APIRET_IS_SUCCESS(apiRet) ? TWE_APIRET_SUCCESS : TWE_APIRET_FAIL;
	}

	// Returns
	return bRet;
}

/*!
 * tsFinal の基本情報を設定する
 * 
 * \param psFinal
 * \param u32AppId
 * \param u32AppVer
 * \param u8SetVer
 * \param u8SetCompat
 * \return 
 */
TWE_APIRET TWESTG_u32SetBaseInfoToFinal(TWESTG_tsFinal * psFinal, uint32 u32AppId, uint32 u32AppVer, uint8 u8SetVer, uint8 u8SetCompat) {
	psFinal->u32AppId = u32AppId;
	psFinal->u32AppVer = u32AppVer;
	psFinal->sVer.u8Set = u8SetVer;
	psFinal->sVer.u8Compat = u8SetCompat;

	return TWE_APIRET_SUCCESS;
}

/**
 * u8kind と u8slot から設定定義情報を探索する。
 */
TWE_APIRET TWESTG_u32SetSettingsToFinal(TWESTG_tsFinal * psFinal, uint8 u8kind, uint8 u8slot, const TWESTG_tsSettingsListItem * pList) {
	// u8kind と u8slot から適切な設定構造体を探索する
	const TWESTG_tsSettings *pSetDef = NULL, *pSet = NULL;
	const TWESTG_tsSettingsListItem *p = pList;

	while (p->u8Kind != TWESTG_KIND_VOID) {
		if (p->u8Kind == u8kind && p->u8Slot == 0) { //default
			pSetDef = &p->sStgs;
		}
		if (p->u8Kind == u8kind && p->u8Slot == u8slot) { //bingo!
			pSet = &p->sStgs;
			break;
		}
		p++;
	}

	// kind と slot が一致しない場合は、デフォルトを適用
	if (pSet == NULL) pSet = pSetDef;
	if (pSet == NULL) return TWE_APIRET_FAIL;

	// psFinal に諸データを書き込む
	psFinal->pasSetList = pList;
	psFinal->psSettings = pSet;
	psFinal->u8Kind = u8kind;
	psFinal->u8Slot = u8slot;

	// デフォルトセーブ設定を適用する
	TWESTG_u32FinalSetDefaults(psFinal);
	
	return TWE_APIRET_SUCCESS;
}

/**
 * デフォルトのスロット (0) を含めてロードする
 */
TWE_APIRET TWESTG_u32LoadDataFrAppstrg(TWESTG_tsFinal *psFinal, uint8 u8kind, uint8 u8slot, uint32 u32AppId, uint8 u8VerCompat, uint32 u32Op) {
	bool_t bRet = TRUE;

	// slot セーブ設定を適用する
	if (u8slot != TWESTG_SLOT_DEFAULT) {
		// slot 特有の定義がある場合は、セーブ値は適用せず slot 特有定義のデフォルト値を採用する (TWESTG_LOAD_OPT_DONTAPPLY_LV2)
		bRet &= TWESTG_u32LoadDataFrAppstrgSingle(psFinal, u8kind, TWESTG_SLOT_DEFAULT, u32AppId, u8VerCompat, TWESTG_LOAD_OPT_DONTAPPLY_LV2);
		bRet &= TWESTG_u32LoadDataFrAppstrgSingle(psFinal, u8kind, u8slot, u32AppId, u8VerCompat, u32Op);
	}
	else {
		// SLOT 0 の時
		bRet &= TWESTG_u32LoadDataFrAppstrgSingle(psFinal, u8kind, TWESTG_SLOT_DEFAULT, u32AppId, u8VerCompat, TWESTG_LOAD_OPT_DONTAPPLY_LV2 | u32Op);
	}

	return bRet;
}

/*!
 * 設定をバッファに書き出す。
 * psFinal 格納済みのデータを解釈し、pBuff へ書き出す。
 *
 * \param pBuff   書き出し先のバッファ
 * \param psFinal 書き出し元のデータ
 * \param u8FormatVer   フォーマット形式
 * \param u32Opt  セーブ時のオプション
 * \return
 */
TWE_APIRET TWESTG_STD_u32FinalToSerializedBuff(TWE_tsBuffer *pBuff, TWESTG_tsFinal *psFinal, uint8 u8FormatVer, uint32 u32Opt) {
	TWE_APIRET apiRet;

	// セーブ用構造体
	TWESTG_tsSlice sCst;
	TWESTG_vInit_tsSlice(&sCst);

	// 必要なデータの生成
	sCst.u8KeyAppId = TWE_CRC8_u8CalcU32(psFinal->u32AppId);
	sCst.u8SettingsVer = psFinal->sVer.u8Set;
	sCst.u8SettingsKind = psFinal->u8Kind;
	sCst.u8SettingsSlot = psFinal->u8Slot;

	// セーブ領域の準備
	TWESTG_u32SliceSavePrep(pBuff, &sCst, u8FormatVer);
	apiRet = TWESTG_u32FinalSave(&sCst, psFinal, u32Opt); // ペイロードの準備

	// pBuff のバッファ長を変更する。
	pBuff->u8bufflen = (uint8)((sCst.psBuff->pu8buff + sCst.psBuff->u8bufflen) - pBuff->pu8buff);

	// Returns
	return apiRet;
}

/*
 * 現在の psFinal の設定に、pBuff のデータを反映させる。
 */
TWE_APIRET TWESTG_CMD_u32SerializedBuffToFinal(TWESTG_tsFinal* psFinal, TWE_tsBuffer* pBuff, uint32 u32Opt) {
	TWE_APIRET apiRet, ret = TWE_APIRET_FAIL;

	// バッファからのデータ解釈を行う (sCst に sBuff から解釈済みの必要領域を切り出す）
	TWESTG_tsSlice sCst;
	TWESTG_vInit_tsSlice(&sCst);
	apiRet = TWESTG_u32SliceLoad(&sCst, pBuff);

	if (TWE_APIRET_IS_SUCCESS(apiRet)) {
		uint8 u8KeyAppId;

		// アプリケーションIDやバージョンのチェック
		u8KeyAppId = TWE_CRC8_u8CalcU32(psFinal->u32AppId);

		if (sCst.u8KeyAppId == u8KeyAppId
			&& sCst.u8SettingsVer >= psFinal->sVer.u8Compat
			&& sCst.u8SettingsKind == psFinal->u8Kind
			// && sCst.u8SettingsSlot == psFinal->u8Slot // スロット番号は無視して、今選択されているスロットに記録する
			) {
			// 設定種別のデータを設定する
			apiRet = TWESTG_u32FinalLoad(psFinal, &sCst, u32Opt);

			if (TWE_APIRET_IS_SUCCESS(apiRet)) {
				ret = TWE_APIRET_SUCCESS;
			}
		}
	}

	return ret;
}