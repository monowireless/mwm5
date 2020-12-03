/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#ifndef TWEINTERACTIVE_H_
#define TWEINTERACTIVE_H_

#include "twecommon.h"
#include "tweutils.h"
#include "tweserial.h"
#include "tweprintf.h"

#include "twesercmd_gen.h"
#include "twesercmd_plus3.h"
#include "tweinputstring.h"
#include "twestring.h"

#include "twesettings0.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief	typedef of struct _TWEINTRCT_sContext (forward decl) */
typedef struct _TWEINTRCT_sContext TWEINTRCT_tsContext;

/** @brief	typedef of struct _TWEINTRCT_sFuncs(forward decl) */
typedef struct _TWEINTRCT_sFuncs TWEINTRCT_tsFuncs;

/** @brief	typedef of keycode (-1: error, 0-0xFF: normal input, others: special key) */
typedef int16 TWEINTRCT_tkeycode;

/*!
 * @brief メニュー構成を切り替えるための関数ポインタ群
 */
typedef struct _TWEINTRCT_sFuncs {
	uint8 u8MenuId;
	uint8 *pu8MenuString;
	void (*pf_vSerUpdateScreen)(TWEINTRCT_tsContext *psIntr);
	void (*pf_vProcessInputByte)(TWEINTRCT_tsContext *psIntr, int16 u16Byte);
	void (*pf_vProcessInputString)(TWEINTRCT_tsContext *psIntr, TWEINPSTR_tsInpStr_Context *pContext);
	TWE_APIRET (*pf_u32ProcessMenuEvent)(TWEINTRCT_tsContext *pContext, uint32 u32Op, uint32 u32Arg1, uint32 u32Arg2, void *vpArg);
} TWEINTRCT_tsFuncs;

/*!
 * インタラクティブモードで管理するデータを保存する構造体
 */
typedef struct _TWEINTRCT_sContext {
	uint8 u8screen;			//! メニュー番号 0:ROOT, 1..N: メニュー
	uint8 u8screen_max;		//! メニューの最大数 (1,2 の2メニューある場合は 3 になる)

	// settings
	struct {
		uint8 u8screen_default; //! デフォルトスクリーン
		bool_t u8OptSerCmd;		//! 通常入力の際にエコーバックを行うか
		bool_t u8Mode;			//! 0:通常 1:インタラクティブモード
		bool_t u8AlwaysKeyReport; //! キーボードコールバックを常に行う
		bool_t u8NoSysReset;	 //! No syetem reset required to take effects.
		uint8 u8DefMenusSlots; //! 0: normal, 1> display slot information on defmenu.
		uint8 u8HeadlineStyle; //! 0: default, 0xFF CUSTOM
	} config;

	uint8 u8keyseq_count;			  //! Key sequence index
	uint16 u16keyseq_tick;		  //! Tick count when get the last key
	TWEINTRCT_tkeycode keyseq[4]; //! Key sequence
	

	int16 i16SelectedIndex;		//! selected index of menu screen (-1: not selected, 0..Items-1: selected)
	uint16 u16HoldUpdateScreen; //! 画面更新までのタイマー値

	TWESTG_tsFinal *psFinal;	//! tsFinal 構造体へのポインタ
	TWE_tsFILE *pStream;		//! 入出力ストリームへのポインタ
	TWE_tsFILE *pStreamTitle;   //! タイトルバーへのポインタ
	TWESERCMD_tsSerCmd_Context *pSerCmd;	//! シリアルパーサへのポインタ (NULL も可)
	TWEINPSTR_tsInpStr_Context *pSerInpStr; //! シリアル入力処理

	const TWEINTRCT_tsFuncs *pFuncs; // メニュー関数
	void *pvWrapObjCpp; // CPP用ラッパオブジェクトへのポインタ

	const TWESTG_tsMsgReplace *msgReplace; //! 設定メッセージの入れ替えテーブル
} TWEINTRCT_tsContext;

/*!
 * TWEINTRCT_cbu32GenericHandler()でのオペレーション引数
 * 
 */
typedef enum {
	E_TWEINRCT_OP_UNHANDLED_CHAR = 0x00,	//! インタラクティブモードで処理されなかったキー入力 (u32Arg1: 入力バイト u32Arg2: n/a vpArg: n/a retrun: n/a)
	E_TWEINRCT_OP_RESET,					//! モジュールリセットを実行する。(u32Arg1: リセットまでの遅延[ms] u32Arg2: n/a vpArg: n/a return: n/a)
	E_TWEINRCT_OP_REVERT,					//! セーブはせずに設定を元に戻す。(u32Arg1: TRUE=>セーブデータなしに戻す FALSE=>セーブの再ロード u32Arg2: n/a vpArg: n/a return: n/a)
	E_TWEINRCT_OP_CHANGE_KIND_SLOT, 		//! KINDとSLOTを変更する。(u32Arg1: 0x0000-0x00FE=>KIND番号 0x0100=>次のKIND 0xFF00=>前のKIND番号 u32Arg2: KINDと同様 n/a vpArg: n/a retrun: TWE_APIRET_SUCCESS/TWE_APIRET_FAIL KIND->0xFF00 SLOT->0x00FF)
	E_TWEINRCT_OP_WAIT,						//! ディレー待ちを行う。(u32Arg1: 待ち時間[ms] u32Arg2: n/a vpArg: n/a return: n/a)
	E_TWEINRCT_OP_GET_APPNAME,				//! アプリケーション名文字列を返す。 (u32Arg1: n/a u32Arg2: n/a vpArg: uint8 **/確保済みバッファ16バイトまたは文字列へのポインタ retrun: n/a)
	E_TWEINRCT_OP_GET_KINDNAME,				//! 種別KIND名文字列を返す。 (u32Arg1: KindID u32Arg2: SlotID vpArg: uint8 **/確保済みバッファ16バイトまたは文字列へのポインタ retrun: n/a)
	E_TWEINRCT_OP_GET_SLOTNAME,				//! 種別SLOT名文字列を返す。 (u32Arg1: KindID u32Arg2: SlotID vpArg: uint8 **/確保済みバッファ16バイトまたは文字列へのポインタ retrun: n/a)
	E_TWEINTCT_OP_GET_SID,					//! SID の取得を返す。 (u32Arg1: n/a u32Arg2: n/a vpArg: uint32 */アドレス格納のためのuint32変数 retrun: n/a)
	E_TWEINTCT_OP_GET_OPTMSG,				//! インタラクティブモードの追記メッセージを返す。(u32Arg1: n/a u32Arg2: n/a vpArg: uint8 **/確保済みバッファ32バイトまたは文字列へのポインタ retrun: n/a)
	E_TWEINTCT_OP_GET_MAX_SLOT,				//! スロット番号の最大値を得る (0:default, slot1..N の場合は Nを返す)
	E_TWEINTCT_OP_ENTER,					//! インタラクティブモードに入る
	E_TWEINTCT_OP_EXIT,						//! インタラクティブモードから離脱する
} E_TWEINRCT_OP;

/**
 * @brief メニューの各種イベントを処理する
 */
typedef enum {
	E_TWEINTCT_MENU_EV_LOAD = 0x20,			//! メニューがロードされた時に呼び出される初期化関数
} E_TWEINTCT_MENU_EV;

/*
 * 以下関数プロトタイプ
 */
TWEINTRCT_tsContext* TWEINTRCT_pscInit(TWESTG_tsFinal *psFinal, TWESERCMD_tsSerCmd_Context *pSerCmd, TWE_tsFILE *fp, void *pfProcessInputByte, const TWEINTRCT_tsFuncs *pFuncs);
#define TWEINTRCT_vInit(...) TWEINTRCT_pscInit(__VA_ARGS__)
void TWEINTRCT_vReConf(TWEINTRCT_tsContext* _psIntr);

void TWEINTRCT_vHandleSerialInput();

// extern TWE_APIRET TWEINTRCT_cbu32GenericHandler(TWEINTRCT_tsContext *pContext, uint32 u32Op, uint32 u32Arg1, uint32 u32Arg2, void *vpArg);

void TWEINTRCT_vSerHeadLine(TWEINTRCT_tsContext *pContext, uint32 u32Op);
void TWEINTRCT_vSerFootLine(TWEINTRCT_tsContext *pContext, uint32 u32Op);
TWE_APIRET TWEINTRCT_u32MenuOpKey(TWEINTRCT_tsContext *pContext, TWEINTRCT_tkeycode keycode);
TWE_APIRET TWEINTRCT_u32MenuChange(TWEINTRCT_tsContext *pContext, uint8 u8NewScreen);

bool_t TWEINTRCT_bIsVerbose(); // NOTE: DEPRECATED!
bool_t TWEINTRCT_bGetInteractive(TWEINTRCT_tsContext* psIntr);
bool_t TWEINTRCT_bSetInteractive(TWEINTRCT_tsContext* psIntr, bool_t bEnter);

#ifdef __cplusplus
}
#endif


#endif
