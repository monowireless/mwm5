/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

#if defined(_MSC_VER)

#include "conio.h"
#include "windows.h"
#include <stdio.h>

#include "twecommon.h"
#include "tweutils.h"
#include "tweserial.h"
#include "tweprintf.h"

#include "twesercmd_gen.h"
#include "twesercmd_plus3.h"
#include "tweinputstring.h"
#include "twestring.h"

#include "twesettings.h"
#include "twesettings_std.h"
#include "twesettings_cmd.h"
#include "twesettings_validator.h"

#include "twesettings_std_defsets.h"
#include "tweinteractive.h"
#include "tweinteractive_defmenus.h"
#include "tweinteractive_settings.h"
#include "tweinteractive_nvmutils.h"
#include "twenvm.h"

#include "msc_sys.h"

#include "msc_defs.h" // tables compiled with C

#include "msc_term.hpp"

using namespace TWETERM;

/*********************************************************************************
 * コンソール
 *********************************************************************************/
const uint8_t U8COL = 64;
const uint8_t U8LINE = 24;
GChar screen_buf[U8COL * U8LINE]; // raw screen buffer.
SimpBuf_GChar screen_lines[U8LINE]; // line buffer
TWETerm_WinConsole the_screen(U8COL, U8LINE, screen_lines, screen_buf); // init the screen.

TWE_GetChar_CONIO con_in; // input stream
TWE_PutChar_CONIO con_out; // output stream (may not be used)

/*********************************************************************************
 * システム関連
 *********************************************************************************/
static bool_t bExit = 0; //!終了フラグ（もう使ってないと思う）

/*********************************************************************************
 * インタラクティブモード関連
 *********************************************************************************/
static TWESERCMD_tsSerCmd_Context sSerCmd;	//!< シリアル入力系列のパーサー
static TWE_tsFILE file, * fp = &file;		//! 入出力ストリーム

static TWESTG_tsFinal sFinal;				//! 確定設定リスト
TWESTG_DECLARE_FINAL(MYDAT, STGS_MAX_SETTINGS_COUNT, 16, 4); //! 確定設定リストの配列等の宣言

static uint8 u8AppKind = STGS_KIND_USER_1;    //! 現在選択中の設定種別(Kind)
static uint8 u8AppSlot = TWESTG_SLOT_DEFAULT; //! 現在選択中の設定スロット(Slot)

/*!
 * アプリケーション保持のデータ構造体
 */
static struct _sAppData {
	uint32 u32AppId;
	uint8 u8Id;
	uint8 u8Ch;
	uint32 u32Baud;
	uint8 u8Pow_n_Retry;
	uint32 u32Opt;
} sAppData;

/*!
 * 確定設定リスト(tsFinal)から各設定を読み出す。
 * ※ コード可読性の観点からイテレータ(TWESTG_ITER_tsFinal_*)を用いた読み出しを行う。
 */
static void vQueryAppData() {
	// 設定のクエリ
	TWESTG_ITER_tsFinal sp;

	TWESTG_ITER_tsFinal_BEGIN(sp, &sFinal); // init iterator
	if (!TWESTG_ITER_tsFinal_IS_VALID(sp)) return; //ERROR DATA

	while (!TWESTG_ITER_tsFinal_IS_END(sp)) { // end condition of iter
		switch (TWESTG_ITER_tsFinal_G_ID(sp)) { // get data as UINT32
		case E_TWESTG_DEFSETS_APPID:
			sAppData.u32AppId = TWESTG_ITER_tsFinal_G_U32(sp); break;
		case E_TWESTG_DEFSETS_LOGICALID:
			sAppData.u8Id = (uint8)TWESTG_ITER_tsFinal_G_U8(sp); break;
		case E_TWESTG_DEFSETS_CHANNEL:
			sAppData.u8Ch = (uint8)TWESTG_ITER_tsFinal_G_U8(sp); break;
		case E_TWESTG_DEFSETS_POWER_N_RETRY:
			sAppData.u8Pow_n_Retry = (uint8)TWESTG_ITER_tsFinal_G_U8(sp); break;
		case E_TWESTG_DEFSETS_UARTBAUD:
			sAppData.u32Baud = TWESTG_ITER_tsFinal_G_U16(sp); break;
		case E_TWESTG_DEFSETS_OPTBITS:
			sAppData.u32Opt = TWESTG_ITER_tsFinal_G_U32(sp); break;
		}
		TWESTG_ITER_tsFinal_INCR(sp); // incrment
	}
}

/*!
 * sAppDataにコピー済みの設定値を出力する
 */
static void vPrintAppData() {
	TWE_fprintf(fp, _TWELB"*** KIND=%d SLOT=%d ***", u8AppKind, u8AppSlot);
	TWE_fprintf(fp, _TWELB"APPID=%08X", sAppData.u32AppId);
	TWE_fprintf(fp, _TWELB"LID=%d", sAppData.u8Id);
	TWE_fprintf(fp, _TWELB"CH=%d", sAppData.u8Ch);
	TWE_fprintf(fp, _TWELB"BAUD=%d", sAppData.u32Baud);
	TWE_fprintf(fp, _TWELB"POWRT=%02X", sAppData.u8Pow_n_Retry);
	TWE_fprintf(fp, _TWELB"OPT=%0b", sAppData.u32Opt);
	TWE_fputs(_TWELB, fp);
}

#if 0
/* 通常のシリアル入力を処理するパーサー */
vProcessInputByte_Chat(TWESERCMD_tsSerCmd_Context* pSerCmd, int16 u16Byte) {
	uint8 u8res;

	u8res = pSerCmd->u8Parse(pSerCmd, (uint8)u16Byte);
	TWE_fputc(u16Byte, fp);

	// 完了!
	if (u8res == E_TWESERCMD_COMPLETE) {
		if (pSerCmd->au8data[0] == 'q') {
			vQueryAppData();
			vPrintAppData();
		}
		else {
			TWE_fprintf(fp, _TWELB"INPUT:");
			pSerCmd->vOutput(pSerCmd, fp);
			TWE_fprintf(fp, _TWELB);
		}
	}
}
#endif

/*!
 * ASCII 入出力を管理するパーサーを動作させる。
 * ※本関数はTWEINTRCT_vHandleSerialInput()より呼び出される。
 *
 * \param pSerCmd パーサー定義構造体
 * \param u16Byte 入力バイト (0x00-0xFF, それ以外はエラー)
 */
void vProcessInputByte_Ascii(TWESERCMD_tsSerCmd_Context* pSerCmd, int16 u16Byte) {
	// 完了!
	if (pSerCmd->u8state == E_TWESERCMD_COMPLETE) {
		uint8* p = pSerCmd->au8data;
		uint8* e = pSerCmd->au8data + pSerCmd->u16len;

		uint8 u8Dst = TWE_G_OCTET(p);
		uint8 u8Op = TWE_G_OCTET(p);

		// 入力メッセージ（これは pSerCmd のバッファを参照する）
		TWE_tsBuffer sBufIn;
		TWE_vInit_tsBuffer(&sBufIn, p, (uint8)(e - p), (uint8)(e - p));

		// 出力メッセージ（応答電文用として）
		uint8 au8Out[128 + 16];
		TWE_tsBuffer sBufOut;
		TWE_vInit_tsBuffer(&sBufOut, au8Out + 16, 0, sizeof(au8Out) - 16);

		if (u8Dst == 0xDB) { // DB コマンドである
			TWE_APIRET apiRet = TWESTG_CMD_u32CmdOp(u8Op, &sBufIn, &sBufOut, &sFinal);

			// 設定データのシリアル化			
			const uint8 U8HEADER_LEN = 2; // ヘッダに２バイト

			TWE_tsBuffer sBufRes;
			sBufRes.pu8buff = au8Out + 16 - U8HEADER_LEN; //ヘッダバイト数分、先頭を巻き戻す
			sBufRes.u8bufflen = sBufOut.u8bufflen + U8HEADER_LEN; //ヘッダバイト数分、全長を長くする
			sBufRes.u8bufflen_max = sizeof(au8Out);

			uint8 u8Code = TWE_APIRET_VALUE(apiRet) & 0xFF;

			// ヘッダの生成
			sBufRes.pu8buff[0] = 0xDB;   // 応答電文もDBから始まる
			sBufRes.pu8buff[1] = u8Code; // CMDはTWESTG_CMD_u32CmdOp()の戻り値から

			// ASCII 形式で出力する
			if (TWE_APIRET_IS_FAIL(apiRet)) {
				TWE_fputs("(ERROR)", fp);
			}
			if (sBufOut.u8bufflen > 0) {
				TWESERCMD_Ascii_vOutput(fp, sBufRes.pu8buff, sBufRes.u8bufflen);
			}
		}
	}
}

/*!
 * データセーブを行う。twesettings ライブラリから呼び出されるコールバック関数。
 *
 * 本関数ではテストを目的として au8SaveRegion[][] にデータを格納する。
 * 格納時の書式は [LEN] [pBuf の内容 ...] である。
 *
 * \param pBuf   データ領域 pBuf->pu8buff[-16..-1] を利用することができる。
 *               NULL の場合は、当該領域の初期化（少なくとも先頭の４バイトをクリア）
 * \param u8kind 種別
 * \param u8slot スロット
 * \param u32Opt オプション
 * \param
 * \return
 */
TWE_APIRET TWESTG_cbu32SaveSetting(TWE_tsBuffer* pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal* psFinal) {
	if (pBuf != NULL) {
		bool_t bRes = TWENVM_bWrite(pBuf, u8slot * 2 + 1); //先頭セクターはコントロールブロックとして残し、2セクター単位で保存
		return bRes ? TWE_APIRET_SUCCESS : TWE_APIRET_FAIL;
	}
	else {
		// pBuf が NULL の時は、該当の EEPROM 領域を初期化する。
		TWENVM_bErase(u8slot * 2 + 1);
		return TWE_APIRET_SUCCESS;
	}
}

// データロード
// データセーブを行う。twesettings ライブラリから呼び出されるコールバック関数。
TWE_APIRET TWESTG_cbu32LoadSetting(TWE_tsBuffer* pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal* psFinal) {
	bool_t bRes = TWENVM_bRead(pBuf, u8slot * 2 + 1); //先頭セクターはコントロールブロックとして残し、2セクター単位で保存
	return bRes ? TWE_APIRET_SUCCESS : TWE_APIRET_FAIL;
}

/*!
 * 確定設定データを再構築する。
 *
 * \param u8kind  種別
 * \param u8slot  スロット
 * \param bNoLoad TRUEならslotに対応するセーブデータを読み込まない。
 */
void vAppLoadData(uint8 u8kind, uint8 u8slot, bool_t bNoLoad) {
	/// tsFinal 構造体の初期化とロード
	// tsFinal 構造体の初期化
	TWESTG_INIT_FINAL(MYDAT, &sFinal);
	// tsFinal 構造体に基本情報を適用する
	TWESTG_u32SetBaseInfoToFinal(&sFinal, APPID, APPVER, STGS_SET_VER, STGS_SET_VER_COMPAT);
	// tsFinal 構造体に kind, slot より、デフォルト設定リストを構築する
	TWESTG_u32SetSettingsToFinal(&sFinal, u8kind, u8slot, SetList);
	// セーブデータがあればロードする
	TWESTG_u32LoadDataFrAppstrg(&sFinal, u8kind, u8slot, APPID, STGS_SET_VER_COMPAT, bNoLoad ? TWESTG_LOAD_OPT_NOLOAD : 0);

	// 設定データをアプリケーション内のデータ領域に反映
	vQueryAppData();
}

/*!
 * 諸処理を行うコールバック。
 * 主としてインタラクティブモードから呼び出されるが、一部は他より呼び出される。
 *
 * \param pContext インタラクティブモードのコンテキスト(NULLの場合はインタラクティブモード以外からの呼び出し)
 * \param u32Op    コマンド番号
 * \param u32Arg1  引数１（役割はコマンド依存）
 * \param u32Arg2  引数２（役割はコマンド依存）
 * \param vpArg    引数３（役割はコマンド依存、主としてデータを戻す目的で利用する）
 * \return コマンド依存の定義。TWE_APIRET_FAILの時は何らかの失敗。
 */
TWE_APIRET TWEINTRCT_cbu32GenericHandler(TWEINTRCT_tsContext* pContext, uint32 u32Op, uint32 u32Arg1, uint32 u32Arg2, void* vpArg) {
	uint32 u32ApiRet = TWE_APIRET_SUCCESS;

	switch (u32Op) {
	case E_TWEINTCT_MENU_EV_LOAD:
		u32ApiRet = TWE_APIRET_SUCCESS_W_VALUE(((uint32)u8AppKind << 8) | u8AppSlot);
		break;

	case E_TWEINRCT_OP_UNHANDLED_CHAR: // 未処理文字列があった場合、呼び出される。
		break;

	case E_TWEINRCT_OP_RESET: // モジュールリセットを行う
		Sleep(3000);
		vAppLoadData(u8AppKind, u8AppSlot, FALSE);
		break;

	case E_TWEINRCT_OP_REVERT: // 設定をもとに戻す。ただしセーブはしない。
		vAppLoadData(u8AppKind, u8AppSlot, u32Arg1);
		Sleep(3000);
		break;

	case E_TWEINRCT_OP_CHANGE_KIND_SLOT:
		// KIND/SLOT の切り替えを行う。切り替え後 pContext->psFinal は、再ロードされること。

		// u32Arg1,2 0xFF: no set, 0xFF00: -1, 0x0100: +1, 0x00?? Direct Set

		// KIND の設定
		if (u32Arg1 != 0xFF) {
			if ((u32Arg1 & 0xff00) == 0x0000) {
				u8AppSlot = u32Arg1 & 0xFF;
			}
			else {
				if ((u32Arg1 & 0xff00) == 0x0100) {
					u8AppKind++;
				}
				else {
					u8AppKind--;
				}
			}
		}

		// SLOT の設定
		if (u32Arg2 != 0xFF) {
			if ((u32Arg2 & 0xff00) == 0x0000) {
				u8AppSlot = u32Arg2 & 0x7;
			}
			else {
				if ((u32Arg2 & 0xff00) == 0x0100) {
					u8AppSlot++;
				}
				else {
					u8AppSlot--;
				}
			}
			u8AppSlot &= 0x07;
		}

		vAppLoadData(u8AppKind, u8AppSlot, FALSE); // 設定を行う

		// 値を戻す。
		// ここでは設定の失敗は実装せず、SUCCESS としている。
		// VALUE は現在の KIND と SLOT。
		u32ApiRet = TWE_APIRET_SUCCESS_W_VALUE((uint16)u8AppKind << 8 | u8AppSlot);
		break;

	case E_TWEINRCT_OP_WAIT: // 一定時間待つ
		Sleep(u32Arg1);
		break;

	case E_TWEINRCT_OP_GET_APPNAME: // CONFIG行, アプリ名
		if (vpArg != NULL) {
			// &(char *)vpArg: には、バッファ16bytesのアドレスが格納されているため strcpy() でバッファをコピーしてもよいし、
			// 別の固定文字列へのポインタに書き換えてもかまわない。
			*((const char**)vpArg) = "APP_SAMPLE";
		}
		break;

	case E_TWEINRCT_OP_GET_KINDNAME: // CONFIG行, KIND種別名
		if (vpArg != NULL) {
			// &(char *)vpArg: には、バッファ16bytesのアドレスが格納されているため strcpy() でバッファをコピーしてもよいし、
			// 別の固定文字列へのポインタに書き換えてもかまわない。
			*((const char**)vpArg) = "DEF";
		}
		break;

	case E_TWEINTCT_OP_GET_OPTMSG:
		if (vpArg != NULL) {
			// &(char *)vpArg: には、バッファ32bytesのアドレスが格納されているため strcpy() でバッファをコピーしてもよいし、
			// 別の固定文字列へのポインタに書き換えてもかまわない。
			char* q = *((char**)vpArg);
			TWE_snprintf(q, 16, "<tick=%d>", u32TickCount_ms);
		}
		break;

	case E_TWEINTCT_OP_GET_SID: // シリアル番号
		if (vpArg != NULL) {
			// シリアル値を書き込む
			*((uint32*)vpArg) = 0x81234567;
		}
		break;

	default:
		break;
	}

	return u32ApiRet;
}

/*!
 * TWENET のコールバック関数（デバッグ用）
 */
void cbToCoNet_vMain() {
	TWEINTRCT_vHandleSerialInput();
	the_screen.refresh();
}

/*!
 * メイン関数。
 * - 各初期化
 * - メインループの動作
 *
 * \return
 */
int main_app() {
	// シリアルの初期化(Windowsコンソール用)
	TWETERM_vInitVSCON(&file, &the_screen, &con_in);

	// シリアルパーサーの初期化
	{
		static uint8 u8buf[256];

		// チャット入力モードの設定
		// TWESERCMD_Chat_vInit(&sSerCmd, u8buf, sizeof(u8buf));
		// TWEINTRCT_vInit(&sFinal, &sSerCmd, fp, vProcessInputByte_Chat);

		// ASCII 入力モードの設定
		TWESERCMD_Ascii_vInit(&sSerCmd, u8buf, sizeof(u8buf));
		sSerCmd.u16timeout = 0; // コマンド入力のタイムアウトはなし

		// インタラクティブモードの設定
		TWEINTRCT_tsContext* psIntr = TWEINTRCT_pscInit(&sFinal, &sSerCmd, fp, vProcessInputByte_Ascii, asFuncs);
		psIntr->config.u8Mode = 1; //初期モードはインタラクティブ
		psIntr->u16HoldUpdateScreen = 96; //画面リフレッシュを行う
		psIntr->config.u8OptSerCmd = 0x01; // エコーバックを行う
		TWEINTRCT_vReConf(psIntr); // 設定を反映させる

		// データロード
		vAppLoadData(u8AppKind, u8AppSlot, FALSE); // 設定を行う
	}

	// キー入力
	while (TRUE) {
		cbToCoNet_vMain();
		if (bExit) break;

		Sleep(20);
		u32TickCount_ms += 20;
	}

	return 0;
}


#endif // _MSC_VER