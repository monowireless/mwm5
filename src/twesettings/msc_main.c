/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/*! @file
 * 
 * twesettings 開発・デバッグ用実行ファイル
 * 
 * 本ファイルは、twesettings の動作を確認するため Windows 上の Visual Studio 上で動作する
 * コンソールアプリケーションとして作成しています。
 * 
 * TWENET のシリアル入力の動作をシミュレーションするため、C標準ライブラリ外の機能として、
 * 非ブロッキングキー入力と、スリープ機能を用い、cbToCoNet_vMain() での処理を記述します。
 * 
 * twesettings での設定は、以下のように３つの設定定義を合成します。
 *   - BASE 基本定義（共通設定、APPIDなど）
 *   - BOARD 定義 (種別単位の設定、Kindとも表記, 接続ハードなどで設定を切り替える)
 *   - SLOT 定義 (設定のバリエーション, DIP SW などで切り替えることを想定）
 *   ここで BASE が最上位(LV0)、SLOTが最下位(LV2)の設定と呼びます。
 * 
 * これら設定には、カスタムデフォルト設定を与えることができ、以下を行えます。
 *   - デフォルト値の書き換え
 *   - 上位で定義された設定を無効化する
 * 
 * セーブデータは、スロット単位で保有し、８つのスロット(SLOT0-7)を利用します。
 * SLOT0 に行われた設定は、他のスロットの設定にも影響を与えます。(APPID をSLOT0 に設定し
 * た場合は SLOT1..7 で別途設定しない限り SLOT0 のセーブ値が用いられます。例外としてプリ
 * セット定義のカスタムデフォルト設定があるLIDについては、SLOT0の設定を参照しません。）
 *  
 * ライブラリの組み込みとして、主要な要素を列挙します。
 * 1. 事前定義
 *   - const TWESTG_tsElement SetBrd[]
 *   単一設定定義のリスト、特定ハードに準じたもの
 *         (e.g. PAL の接続基板の種別に応じた設定を行うための追加設定リスト）
 *   - uint8 au8CustomDefault_Base[]
 *   設定のカスタム化
 *        （プリセット定義のデフォルト値を書き換える設定など）
 *   - au8CustomDefault_Brd_Chan3[] au8CustomDefault_Brd_Def[]
 *   設定のカスタム化
 *        （プリセット定義で不要な設定を無効化する設定など）
 *   - const TWESTG_tsSettingsListItem SetList[]
 *   スロット単位の設定の組み合わせをリスト化したもの
 *         (e.g. PAL の接続基板ごと、DIP SWごとに独自の定義を行う）
 * 
 * 2. セーブデータ
 *   - au8MySetting_0[] など
 *   シリアル化セーブデータの例です。本来は、このバイト列を EEPROM 等に読み書きします。
 * 
 *   OTA といった、他のデバイス向けの設定に対応する場合は、
 *     - SetList[] : 設定定義
 *     - KIND      : 種別
 *     - SLOT ID   : SLOT ID
 *   により管理する。
 * 
 * 3. シリアル入出力
 *   - TWE_tsFILE シリアル入出力ストリーム
 *   本テストプログラムではコンソールへの入出力向けに以下の関数を実装しています。
 *      TWETERM_vInitVSCON() - tsFILE 初期化
 *      TWETERM_iPutC()      - １文字出力
 *      TWETERM_iGetC()      - １文字入力（ノンブロッキング）
 *      TWETERM_iPutS()      - 文字列出力（実装は必須ではない）
 * 
 * 4. 必須コールバック
 *   twesettings ライブラリ外部で実装すべき機種依存性が高い手続きです。
 * 
 *   - TWESTG_cbu32LoadSetting()
 *   アプリケーションが管理する不揮発性メモリより設定をロードする。
 *   Kind, Slot 番号を従った格納場所から、設定データを読み出しそのバイト列を渡します。
 * 
 *   - TWESTG_cbu32SaveSetting()
 *   アプリケーションが管理する不揮発性メモリに設定をセーブする。
 *   Kind, Slot 番号に従った格納場所に、渡された設定データを書き込みます。
 * 
 *   - TWEINTRCT_cbu32GenericHandler()
 *   インタラクティブモード実行に必要な機能を実装します。
 *     モジュールリセット・設定の再ロード、初期化・別のKind/Slotの設定を読み出す
 *     メッセージ表示のためにしばらく待ち処理を行う・アプリケーション名を得る
 *     Kind種別名を得る・シリアル番号を得る
 * 
 * 5. 宣言
 *   - 確定設定リストの宣言
 *     static TWESTG_tsFinal sFinal; //リスト構造体
 *     TWESTG_DECLARE_FINAL(...);    //リスト用の配列等の宣言
 *     
 * 6. 初期化
 *   - 入出力ストリーム(tsFILE)の初期化
 *     TWETERM_vInitVSCON(&file); 
 *   - シリアルパーサーの初期化
 *     TWESERCMD_Ascii_vInit(); // ASCIIモード
 *   - インタラクティブモードの初期化
 *     TWEINTRCT_vInit();
 *     asFuncs はメニュー構成の定義ファイル。tweinteractive_???.c を参照。
 *     static const TWEINTRCT_tsFuncs asFuncs[]; 
 *   - 確定設定リスト(tsFinal)の初期化とロード vAppLoadData()
 *     TWESTG_INIT_FINAL();                 // 初期化
 *     TWESTG_u32SetBaseInfoToFinal();      // 必須情報の登録
 *     TWESTG_u32SetSettingsToFinal();      // Kind, Slot に対応した設定の初期化
 *     TWESTG_u32LoadDataFrAppstrg();       // セーブ設定の読み込み
 *  
 * 7. 動作、シリアル入力の流れ
 *   -main()
 *     アプリケーション無限ループ（20ms スリープを入れて cbToCoNet_vMain()呼び出し)
 * 
 *     -cbToCoNet_vMain()
 *      メインループ
 * 
 *       -TWEINTRCT_vHandleSerialInput()
 *        シリアルの入力をチェックし、入力バイトをディスパッチする。
 * 
 *         -vProcessInputByte_Ascii()
 *          通常はASCII書式に基づくコマンド処理を行う
 * 
 *            -TWESTG_CMD_u32CmdOp()
 *             コマンド処理（設定の読み書きなど）
 * 
 *         -インタラクティブモードの処理
 *          + + + 入力にて、このモードに遷移。
 * 
 * 8. 設定の読み出し
 *   - 各種設定の読み出し vQueryAppData()
 *     確定設定リスト(tsFinal)の読み出しを簡便化する TWESTG_ITER_tsFinal* マクロを
 *     用いて各種設定をアプリケーションの設定領域(sAppData 構造体)にコピーしている。
 */

#ifdef _MSC_VER // ONLY ON WINDOWS (JUST TEST PURPOSE)

#include <stdio.h>

#include <windows.h>
#include <conio.h>

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

#define APPID 0x12345678   //! アプリケーションID
#define APPID_CRC8 0x37    //! アプリケーションIDのCRC8(計算した値)
#define APPVER 0x01020300  //! バージョン番号
#define APPVER_CRC8 0x3A   //! バージョン番号のCRC8(計算した値)

#define STGS_SET_VER 0x01         //! 設定バージョン
#define STGS_SET_VER_COMPAT 0x01  //! 互換性のある設定バージョン

#define STGS_MAX_SETTINGS_COUNT 32 //! 設定数の最大(確定設定リスト tsFinal の配列数を決める)

#define STGS_KIND_USER_1 0x01      //! 種別ID

#undef USE_MEM_SAVE //! 本ファイルで定義するセーブ関数を用いる（定義しない場合は msc_eep.h のAPIを用いる）
#ifdef USE_MEM_SAVE
#define STGS_SAVE_BUFF_SIZE 128     //! セーブバッファサイズ
#endif

/* serial functions */
static int TWETERM_iPutC(int c, TWE_tsFILE *fp) { return _putch(c); }
// static int TWETERM_iGetC(TWE_tsFILE *fp) { while (!_kbhit()) Sleep(20); return _getch(); } // blocking
static int TWETERM_iGetC(TWE_tsFILE *fp) { if (_kbhit()) return _getch(); else return -1; }
static int TWETERM_iPutS(const char *s, TWE_tsFILE *fp) { return fputs(s, stdout); }
static void TWETERM_vFlush(TWE_tsFILE *fp) { ; }
void TWETERM_vInitVSCON(TWE_tsFILE *fp) {
	fp->fp_getc = TWETERM_iGetC;
	fp->fp_putc = TWETERM_iPutC;
	fp->fp_puts = TWETERM_iPutS;
	fp->fp_flush = TWETERM_vFlush;
}

static bool_t bExit = 0; //!終了フラグ（もう使ってないと思う）

/*!
 * ボード設定データ例（独自の設定を追加している）
 */
const TWESTG_tsElement SetBrd[] = {
	{ 0x82,
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = 38400 }},
		{ "UBA", "UART Baud [9600-230400]", "" },
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 6, 'b' },
		{ {.u32 = 0}, {.u32 = 115200}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ 0x81,
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = (1UL << 13 | 1UL << 18) }},
		{ "TE1", "test for ChList", "" },
		{ E_TWEINPUTSTRING_DATATYPE_CUSTOM_DISP_MASK | E_TWEINPUTSTRING_DATATYPE_STRING, 8, 'A' },
		{ {.u32 = 0}, {.u32 = 0xFFFFFFFF}, TWESTGS_VLD_u32ChList, NULL },
	},
	{E_TWESTG_DEFSETS_VOID} // FINAL DATA
};

/*!
 * カスタムデフォルト(BASE)
 *   APPIDのデフォルト値を書き換えている
 */
uint8 au8CustomDefault_Base[] = {
	6,   // 総バイト数
	E_TWESTG_DEFSETS_APPID, (TWESTG_DATATYPE_UINT32 << 4) | 4, 0xa1, 0xb2, 0x12, 0x34, // 6bytes
};

/*!
 * カスタムデフォルト(BOARD) - [シリアル化要素データ集合] 
 *   E_TWESTG_DEFSETS_CHANNELS_3を未使用にしている
 */
uint8 au8CustomDefault_Brd_Chan3[] = {
	2,   // 総バイト数(このバイトは含まない。手計算で間違えないように入力！)
	E_TWESTG_DEFSETS_CHANNELS_3, TWESTG_DATATYPE_UNUSE
};

/*!
 * カスタムデフォルト(BOARD) - [シリアル化要素データ集合]
 *   E_TWESTG_DEFSETS_CHANNELを未使用にしている
 */
uint8 au8CustomDefault_Brd_Def[] = {
	2,   // 総バイト数(このバイトは含まない。手計算で間違えないように入力！)
	E_TWESTG_DEFSETS_CHANNEL, TWESTG_DATATYPE_UNUSE
};

#ifdef USE_MEM_SAVE
/*!
 * 設定データ例 - [シリアル化セーブ形式]
 */
uint8 au8MySetting_0[] = {
	0x01, // データ形式ID (0x01 固定)
	APPID_CRC8, // APP ID のハッシュ値
	STGS_SET_VER, // 保存された設定のバージョン番号
	STGS_KIND_USER_1, // 設定ID
	TWESTG_SLOT_DEFAULT, // スロット

	// 以降[シリアル化要素データ集合]
	12,   // 総バイト数(このバイトは含まない。手計算で間違えないように入力！)
	E_TWESTG_DEFSETS_OPTBITS,   (TWESTG_DATATYPE_UINT32 << 4) | 4, 0xa1, 0xb2, 0xc3, 0xd4, // 6bytes
	E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8  << 4) | 1, 33, // 3bytes
	E_TWESTG_DEFSETS_CHANNEL,   (TWESTG_DATATYPE_UINT8  << 4) | 1, 25, // 3bytes
};

/*!
 * 設定データ例 - [シリアル化セーブ形式]
 */
uint8 au8MySetting_1[] = {
	0x01, // データ形式ID (0x01 固定)
	APPID_CRC8, // APP ID のハッシュ値
	STGS_SET_VER, // 保存された設定のバージョン番号
	STGS_KIND_USER_1, // 設定ID
	TWESTG_SLOT_1, // スロット

	// 以降[シリアル化要素データ集合]
	3,   // ペイロードのバイト数
	E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8 << 4) | 1, 20 // 3bytes
};
#endif

/*!
 * 設定定義(tsSettings)
 *   スロット0..7までの定義を記述
 */
const TWESTG_tsSettingsListItem SetList[] = {
	{ STGS_KIND_USER_1, TWESTG_SLOT_DEFAULT, 
		{ TWESTG_DEFSETS_BASE, NULL, NULL, 
		  au8CustomDefault_Base, au8CustomDefault_Brd_Def, NULL } },
	{ STGS_KIND_USER_1, TWESTG_SLOT_1, 
		{ TWESTG_DEFSETS_BASE, NULL, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  au8CustomDefault_Base, au8CustomDefault_Brd_Def, TWESTG_DEFCUST_SLOT[1] }}, // slot1 固有定義
	{ STGS_KIND_USER_1, TWESTG_SLOT_2,
		{ TWESTG_DEFSETS_BASE, NULL, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  au8CustomDefault_Base, au8CustomDefault_Brd_Def, TWESTG_DEFCUST_SLOT[2] }}, // slot2 固有定義
	{ STGS_KIND_USER_1, TWESTG_SLOT_3,
		{ TWESTG_DEFSETS_BASE, SetBrd, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  au8CustomDefault_Base, au8CustomDefault_Brd_Chan3, TWESTG_DEFCUST_SLOT[3] }}, // slot3 固有定義
	{ STGS_KIND_USER_1, TWESTG_SLOT_4,
		{ TWESTG_DEFSETS_BASE, SetBrd, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  au8CustomDefault_Base, au8CustomDefault_Brd_Def, TWESTG_DEFCUST_SLOT[4] }}, // slot4 固有定義
	{ STGS_KIND_USER_1, TWESTG_SLOT_5,
		{ TWESTG_DEFSETS_BASE, NULL, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  au8CustomDefault_Base, au8CustomDefault_Brd_Def, TWESTG_DEFCUST_SLOT[5] }}, // slot5 固有定義
	{ STGS_KIND_USER_1, TWESTG_SLOT_6,
		{ TWESTG_DEFSETS_BASE, NULL, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  au8CustomDefault_Base, au8CustomDefault_Brd_Def, TWESTG_DEFCUST_SLOT[6] }}, // slot6 固有定義
	{ STGS_KIND_USER_1, TWESTG_SLOT_7,
		{ TWESTG_DEFSETS_BASE, NULL, NULL /* TWESTG_DEFSETS_SLOT[TWESTG_SLOT_1] */,
		  au8CustomDefault_Base, au8CustomDefault_Brd_Def, TWESTG_DEFCUST_SLOT[7] }}, // slot7 固有定義
	{ TWESTG_KIND_VOID, TWESTD_SLOT_VOID, { NULL }} // TERMINATE
};

#ifdef USE_MEM_SAVE
/*!
 * セーブ領域(テストアプリではメモリー上でセーブ・ロードを行う)
 */
static uint8 au8SaveRegion[8][STGS_SAVE_BUFF_SIZE]; 
#endif

/**
 * インタラクティブモードのメニュー構成
 */
static const TWEINTRCT_tsFuncs asFuncs[] = {
	{ 0, (uint8*)"ROOT MENU", TWEINTCT_vSerUpdateScreen_defmenus, TWEINTCT_vProcessInputByte_defmenus, TWEINTCT_vProcessInputString_defmenus, TWEINTCT_u32ProcessMenuEvent_defmenus }, // standard settings
	{ 1, (uint8*)"CONFIG", TWEINTCT_vSerUpdateScreen_settings, TWEINTCT_vProcessInputByte_settings, TWEINTCT_vProcessInputString_settings, TWEINTCT_u32ProcessMenuEvent_settings  }, // standard settings
	{ 2, (uint8*)"EEPROM UTIL", TWEINTCT_vSerUpdateScreen_nvmutils, TWEINTCT_vProcessInputByte_nvmutils, TWEINTCT_vProcessInputString_nvmutils, TWEINTCT_u32ProcessMenuEvent_nvmutils }, // standard settings
	{ 0xFF, NULL, NULL, NULL }
};

/*********************************************************************************
 * インタラクティブモード関連
 *********************************************************************************/
static TWESERCMD_tsSerCmd_Context sSerCmd;	//!< シリアル入力系列のパーサー
static TWE_tsFILE file, *fp = &file;		//! 入出力ストリーム

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
			sAppData.u8Id = (uint8)TWESTG_ITER_tsFinal_G_U32(sp); break;
		case E_TWESTG_DEFSETS_CHANNEL:
			sAppData.u8Ch = (uint8)TWESTG_ITER_tsFinal_G_U32(sp); break;
		case E_TWESTG_DEFSETS_POWER_N_RETRY:
			sAppData.u8Pow_n_Retry = (uint8)TWESTG_ITER_tsFinal_G_U32(sp); break;
		case E_TWESTG_DEFSETS_UARTBAUD:
			sAppData.u32Baud = TWESTG_ITER_tsFinal_G_U32(sp); break;
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
vProcessInputByte_Chat(TWESERCMD_tsSerCmd_Context *pSerCmd, int16 u16Byte) {
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
void vProcessInputByte_Ascii(TWESERCMD_tsSerCmd_Context *pSerCmd, int16 u16Byte) {
	// 完了!
	if (pSerCmd->u8state == E_TWESERCMD_COMPLETE) {
		uint8 *p = pSerCmd->au8data;
		uint8 *e = pSerCmd->au8data + pSerCmd->u16len;

		uint8 u8Dst = TWE_G_OCTET(p);
		uint8 u8Op = TWE_G_OCTET(p);

		// 入力メッセージ（これは pSerCmd のバッファを参照する）
		TWE_tsBuffer sBufIn;
		TWE_vInit_tsBuffer(&sBufIn, p, (uint8)(e - p), (uint8)(e - p));

		// 出力メッセージ（応答電文用として）
		uint8 au8Out[128+16];
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
TWE_APIRET TWESTG_cbu32SaveSetting(TWE_tsBuffer *pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal *psFinal) {
#ifdef USE_MEM_SAVE
	if (u8slot < 8) {
		uint8 *q = au8SaveRegion[u8slot];

		if (pBuf == NULL) {
			*q = 0; // セーブ領域を潰す
		}
		else {
			TWE_S_OCTET(q, pBuf->u8bufflen); // 1 バイト目はデータ長
			TWE_S_STRING(q, pBuf->pu8buff, pBuf->u8bufflen);
		}
	}

	return TWE_APIRET_SUCCESS;
#else
	if (pBuf != NULL) {
		bool_t bRes = TWENVM_bWrite(pBuf, u8slot * 2 + 1); //先頭セクターはコントロールブロックとして残し、2セクター単位で保存
		return bRes ? TWE_APIRET_SUCCESS : TWE_APIRET_FAIL;
	} else {
		// pBuf が NULL の時は、該当の EEPROM 領域を初期化する。
		TWENVM_bErase(u8slot * 2 + 1);
		return TWE_APIRET_SUCCESS;
	}
#endif
}

// データロード
// データセーブを行う。twesettings ライブラリから呼び出されるコールバック関数。
TWE_APIRET TWESTG_cbu32LoadSetting(TWE_tsBuffer *pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal *psFinal) {
#ifdef USE_MEM_SAVE
	uint8 *p = NULL, l = 0;

	if (u8slot < 8) {
		if (au8SaveRegion[u8slot][0] != 0) {
			p = au8SaveRegion[u8slot] + 1;
			l = au8SaveRegion[u8slot][0];
		}
	}

	if (p != NULL) {
		pBuf->pu8buff = p;
		pBuf->u8bufflen = l;
		pBuf->u8bufflen_max = l;

		return TWE_APIRET_SUCCESS;
	}
	return TWE_APIRET_FAIL;
#else
	bool_t bRes = TWENVM_bRead(pBuf, u8slot * 2 + 1); //先頭セクターはコントロールブロックとして残し、2セクター単位で保存
	return bRes ? TWE_APIRET_SUCCESS : TWE_APIRET_FAIL;
#endif
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
TWE_APIRET TWEINTRCT_cbu32GenericHandler(TWEINTRCT_tsContext *pContext, uint32 u32Op, uint32 u32Arg1, uint32 u32Arg2, void *vpArg) {
	uint32 u32ApiRet = TWE_APIRET_SUCCESS;

	switch (u32Op) {
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
			*((uint8**)vpArg) = "APP_SAMPLE";
		}
		break;

	case E_TWEINRCT_OP_GET_KINDNAME: // CONFIG行, KIND種別名
		if (vpArg != NULL) {
			// &(char *)vpArg: には、バッファ16bytesのアドレスが格納されているため strcpy() でバッファをコピーしてもよいし、
			// 別の固定文字列へのポインタに書き換えてもかまわない。
			*((uint8**)vpArg) = "DEF";
		}
		break;

	case E_TWEINTCT_OP_GET_OPTMSG:
		if (vpArg != NULL) {
			// &(char *)vpArg: には、バッファ32bytesのアドレスが格納されているため strcpy() でバッファをコピーしてもよいし、
			// 別の固定文字列へのポインタに書き換えてもかまわない。
			uint8 *q = *((uint8**)vpArg);
			TWE_snprintf(q, 32, "<tick=%d>", u32TickCount_ms);
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
}

/*!
 * メイン関数。
 * - 各初期化
 * - メインループの動作
 * 
 * \return 
 */
int main() {
	// シリアルの初期化(Windowsコンソール用)
	TWETERM_vInitVSCON(&file);

	// セーブ領域の初期化
#ifdef USE_MEM_SAVE
	memcpy(au8SaveRegion[0] + 1, au8MySetting_0, sizeof(au8MySetting_0)); // SLOT0
	au8SaveRegion[0][0] = sizeof(au8MySetting_0);
	memcpy(au8SaveRegion[1] + 1, au8MySetting_1, sizeof(au8MySetting_1)); // SLOT1
	au8SaveRegion[1][0] = sizeof(au8MySetting_1);
#endif

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
		TWEINTRCT_tsContext *psIntr = TWEINTRCT_pscInit(&sFinal, &sSerCmd, fp, vProcessInputByte_Ascii, asFuncs);
		psIntr->config.u8Mode = 1; //初期モードはインタラクティブ
		psIntr->u16HoldUpdateScreen = 96; //画面リフレッシュを行う
		psIntr->config.u8OptSerCmd = 0x01; // エコーバックを行う
		TWEINTRCT_vReConf(psIntr); // 設定を反映させる
	}

	// データロード
	vAppLoadData(u8AppKind, u8AppSlot, FALSE);
	vPrintAppData();

	// キー入力
	while (TRUE) {
		cbToCoNet_vMain();
		if (bExit) break;

		Sleep(20);
		u32TickCount_ms += 20;
	}

	return 0;
}

#endif