/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/** @file
 *
 * @defgroup TWESTGS ヘッダファイル
 *
 */

/*
 セーブフォーマットについて

 セーブデータの形式は、以下のような３層構造となっている。
   [c. セーブ形式 [b. 要素データ集合 [a. 要素データ ]xN ]

 a. シリアル化要素データ (TWESTG_tsElement, TWESTG_tsDatum, TWESTG_tuDatum)
   [OCTET]: データ型データ長
     0x00 の場合
	   設定を無効にする
     MSB=0 の場合
	   H:MSBから4bit TWESTG_DATATYPE_* に示すデータ型
	   L:LSBから4bit データ長
     MSB=1 の場合(TWESTG_DATATYPE_STRING)
	   LSBから4bit  -> データ長
	   その他ビット -> reserved

   [OCTET x N]: データ
     データは big endian で格納する。
	 文字列型は 0 終端不要で、データ長分固定長で格納する。

 b. シリアル化要素データ集合 (TWESTG_tsSlice::psBuff)
   [OCTET]: 総データ長さ（次のバイトから数えるので0ならデータなし)
   [[OCTET x N: a.要素データ] x N]: 以後 a.単一設定データが並ぶ（終端や単一データ間にはバイトは挿入しない）

 c. シリアル化セーブ形式
 　[OCTET]: 書式バージョン (MSB=0, MSB=1は拡張用のリザーブ)
   
   VER 0x01 / EEP等の記録形式
   [OCTET]: アプリケーションID (32bitのIDをビッグエンディアン形式のバイト列にし CRC8 にて計算)
   [OCTET]: バージョン定義 (MSB=0, MSB=1は拡張用のリザーブ)
   [OCTET]: 種別、同一アプリ内で別のふるまいをさせる時の識別子 (MSB=0, MSB=1は拡張用のリザーブ)
   [OCTET]: MSB:拡張用ビット 
      H=MSBから4bit: MSB(拡張用ビット)
      L=LSBから4bit: スロットID
   [b. 要素データ集合]

   VER 0x11 / コマンド等 --- TODO --- NOT IMPLEMENTED
   [OCTET]: アプリケーションID (32bitのIDをビッグエンディアン形式のバイト列にし CRC8 にて計算)
   [OCTET]: バージョン定義 (MSB=0, MSB=1は拡張用のリザーブ)
   [OCTET]: 種別、同一アプリ内で別のふるまいをさせる時の識別子 (MSB=0, MSB=1は拡張用のリザーブ)
   [OCTET]: MSB:拡張用ビット
	  H=MSBから4bit: MSB(拡張用ビット)
	  L=LSBから4bit: スロットID
   [OCTET]: 設定オプション
   [b. 要素データ集合]

   参考: EEPROM
     a5 01 ef 5a 10 d9 01 37 01 01 00 0a 02 ...
	             ^1 ^2 ^3 ^4 ^5 ^6 ^7 ^8 ^9 ...
	1: length
	2: CRC
	3: 書式バージョン
	4: APPID
	5: バージョン定義
	6: 種別(KIND)
	7: スロット(SLOT)

	8: b. シリアル化要素データ集合の総データ長

    b. シリアル化要素データ
    9: ID
 */

#ifndef TWESETTINGS0_H_
#define TWESETTINGS0_H_

#include "twecommon.h"

#ifdef __cplusplus
extern "C" {
#endif


#define TWESTG_FORMAT_VER_0x01 0x01
#define TWESTG_FORMAT_VER_VOID 0xFF

#define TWESTG_DATATYPE_UNSET 0
#define TWESTG_DATATYPE_UINT8 1
#define TWESTG_DATATYPE_INT8 2
#define TWESTG_DATATYPE_UINT16 3
#define TWESTG_DATATYPE_INT16 4
#define TWESTG_DATATYPE_UINT32 5
#define TWESTG_DATATYPE_INT32 6
#define TWESTG_DATATYPE_STRING 0x80
#define TWESTG_DATATYPE_UNUSE 0x00 //! このデータセットの利用を無効化する（上位設定にない設定に限る）

#define TWESTG_DATAFORMAT_DEC 0
#define TWESTG_DATAFORMAT_HEX 1
#define TWESTG_DATAFORMAT_BIN 2
#define TWESTG_DATAFORMAT_STRING 3

#define TWESTG_DATASTAT_SAVED_MASK 0x80 //! このスロット向けにセーブされた
#define TWESTG_DATASTAT_SAVED_HIGHER_MASK 0x40 //! 上位設定にてセーブされた
#define TWESTG_DATASTAT_MODIFIED_MASK 0x10 //! 値の編集が行われた（ただしセーブ前）
#define TWESTG_DATASTAT_ORIGIN_MASK 0x03 //! 設定階層 (0: 最上位, 1: スロット0, 2: スロット向け設定)
#define TWESTG_DATASTAT_CUSTOM_DEFAULT_MASK 0x04 //! カスタムデフォルトにより値が設定された

#define TWESTG_DATAID_START 0x1   // 各設定ID(8bit)

#define TWESTG_SLOT_DEFAULT 0
#define TWESTG_SLOT_1 1
#define TWESTG_SLOT_2 2
#define TWESTG_SLOT_3 3
#define TWESTG_SLOT_4 4
#define TWESTG_SLOT_5 5
#define TWESTG_SLOT_6 6
#define TWESTG_SLOT_7 7
#define TWESTG_SLOT_8 8
#define TWESTG_SLOT_9 9
#define TWESTG_SLOT_A 0xA
#define TWESTG_SLOT_B 0xB
#define TWESTG_SLOT_C 0xC
#define TWESTG_SLOT_D 0xD
#define TWESTG_SLOT_E 0xE
#define TWESTG_SLOT_F 0xF
#define TWESTD_SLOT_VOID 0xFF

#define TWESTD_KIND_GENERAL 0
#define TWESTG_KIND_VOID 0xFF

#define TWESTG_LOAD_OPT_NOLOAD 0x00000001 //! セーブデータをロードしない
#define TWESTG_LOAD_OPT_DONTAPPLY_LV2 0x00000002 //! LV2 設定に対してはロードした値を適用しない
#define TWESTG_LOAD_OPT_LOAD_AS_UNSAVED 0x00000004 //! ロード時に TWESTG_DATASTAT_MODIFIED_MASK フラグを立てる

#define TWESTG_SAVE_OPT_INCLUDE_HIGHER_SETTINGS 0x00010000 //! 上位のセーブデータを同一スロットのセーブデータとして保存する

#define TWESTG_DATA_COUNT_MAX 32 //! 設定数の最大値

#define TWESTG_FINAL_OPT_SAVED 0x01 //! セーブをしたがリセットしていない（リセットを促す）
#define TWESTG_FINAL_OPT_REVERTED_TO_DEFAULT 0x02 //! デフォルト設定にリバートした（セーブを促す）

/*!
 * TWESTG_tsFinal構造体で必要とするデータ列を宣言する。
 * \param NAME 識別名（変数名の一部に使われる）
 * \param COUNT 構造体中の配列長（設定エントリの最大数）
 * \param STRBUF 文字列型の格納領域（ヒープ）のバイト数
 * \param CUSTDEF カスタムデフォルト設定の最大数
 */
#define TWESTG_DECLARE_FINAL(NAME,COUNT,STRBUF,CUSTDEF) \
	static TWESTG_tsDatum __TWESTG_##NAME##_sData[COUNT]; \
	static TWESTG_tuDatum __TWESTG_##NAME##_uData[CUSTDEF]; \
	static TWESTG_tsElement* __TWESTG_##NAME##_apEle[COUNT]; \
	static TWE_tsBuffer __TWESTG_ ##NAME##_sBufStrPool; \
	static uint8 __TWESTG_ ##NAME##_auShortcutKeys[COUNT]; \
	static uint8 __TWESTG_ ##NAME##_u8StrPool[STRBUF];

/*!
 * TWESTG_tsFinal構造体の初期化処理を行う。
 * 事前にTWESTG_DECLARE_FINAL()マクロにて宣言しておくこと。
 * \param NAME 識別名（変数名の一部に使われる。TWESTG_DECLARE_FINALで使用したNAME）
 * \param SFINAL TWESTG_tsFinal構造体変数へのポインタ
 */
#define TWESTG_INIT_FINAL(NAME,PSFINAL) \
	TWE_vInit_tsBuffer(&__TWESTG_##NAME##_sBufStrPool, __TWESTG_##NAME##_u8StrPool, 0, sizeof(__TWESTG_##NAME##_u8StrPool)); \
	TWESTG_vInit_tsFinal((PSFINAL), NULL, __TWESTG_##NAME##_sData, &__TWESTG_##NAME##_sBufStrPool, __TWESTG_##NAME##_apEle, \
            __TWESTG_##NAME##_auShortcutKeys, sizeof(__TWESTG_##NAME##_auShortcutKeys), __TWESTG_##NAME##_uData,  \
			sizeof(__TWESTG_##NAME##_uData) / sizeof(TWESTG_tuDatum));

/*!
 * 要素データ(tuDatum)
 * 32bit の共用体のデータ型として取り扱う
 */
typedef union _uDatum {
    uint8 u8;
    uint16 u16;
    uint32 u32;
    int8 i8;
    int16 i16;
    int32 i32;
    uint8 *pu8;
} TWESTG_tuDatum;

/*!
 * 要素データ定義(tsDatum)
 * - tuDatumのデータ型・データ長
 * - データの利用状況
 */
typedef struct {
    uint8 u8Type; //!データ型, TWESTG_DATATYPE_* 参照
    uint8 u8Len;  //!データ長（バイト数）
    uint8 u8Stat; //!適用されたデータの状況 TWESTG_DATASTAT_??? 参照
    uint8 u8Opt;  //!カスタムデフォルトの設定値配列へのインデックス(MSB=1格納, LSBから4bit:インデックス)
    TWESTG_tuDatum uDatum; // 格納データ (4bytes)
} TWESTG_tsDatum;

/*!
 * メッセージ入れ替えのためのテーブル
 */
typedef struct {
	uint8 u8Id;          //! 項目ID
	const char* strName;  //! 項目名
	const char* strDesc;  //! 詳細
} TWESTG_tsMsgReplace;

/**
 * 単一設定定義(tsElement)
 * 
 * 設定の諸定義・デフォルト値の定義を行う。
 * 本構造体に従った固定データ定義をコード中で行う。
 */
typedef struct _TWESTG_sElement {
    uint16 u16Id; //! 設定ID 0x0000,0x00FF: reserved, 1..63: vendor default, 64...127: application specific, 0xFFFF: terminator  
    TWESTG_tsDatum sDatum; //! データ定義とデフォルト値

    // メッセージなど
	struct {
		const char* strLabel; //! 項目キー名
		const char* strName;  //! 項目名
		const char* strDesc;  //! 詳細
	} sStr;

	// 入力データ
	struct {
		uint8 u8Format; //! 入出力フォーマット
		uint8 u8InputMaxLength; //! 入力時の最大文字数 
		uint8 u8ShortcutKey; //! 選択用のキーバインド
	} sFormat;

	// 入力データのチェック
	struct {
		TWESTG_tuDatum d1;
		TWESTG_tuDatum d2;
		TWE_APIRET(*fpOp)(struct _TWESTG_sElement* pMe, TWESTG_tsDatum* psDatum, uint16 u16OpId, TWE_tsBuffer* pBuf);
		void* pvOpt;
	} sValidate;
} TWESTG_tsElement;

/**
 * 設定定義(tsSettings)
 * 
 * 単一設定定義(tsElement)とデフォルト値の上書きデータの集合により、
 * アプリケーションの設定一覧を形成する。
 * 
 * 設定は３層構造になっている。
 * - 上位から順に読み込まれる。
 * - 下位で同一の設定定義がある場合は、下位の設定が上書きされる。
 * 
 *   Base  : 基本的な設定(原則としてTWESTG_DEFSETS_BASE定義を利用。APPID, LID, CH など基本設定)
 *   Board : アプリケーションや付属ハードウェアなどの違いで設定群の変化に対応する(センサー設定など)
 *   Slot  : 同一設定でもバリエーションを持たせたい場合に利用する(DIP SW により設定を切り替えたい場合などに利用する)
 * 
 * 各層に対応するデフォルト値の上書き(CustomDefault)設定を含めることができる。
 *   - NULL の場合は、デフォルト値の上書きを行わない
 *   - [シリアル化要素データ集合]のデータ列を指定する。
 *   - [シリアル化要素データ]の[データ型データ長]バイトが 0x00 の場合は、この設定を無効にする。
 *   - 上位で無効にした設定であっても、下位で再定義した場合はその設定は有効になる。
 *     例：Base で無効にした場合でも、Boardで再定義した場合は設定は有効になる。
 *   - 読み込まれた設定値はデフォルト値として TWESTG_tsFinal::asDatum[] に格納される。
 * 
 */
typedef struct _TWESTG_sSettings {
    const TWESTG_tsElement *pBase;      //! Base 基本設定
    const TWESTG_tsElement *pBoard;     //! Board ハード由来の設定
    const TWESTG_tsElement *pSlot;      //! Slot DIPスイッチなどで切り替えるセット
	const uint8 *au8BaseCustomDefault;  //! Base カスタムデフォルト 
	const uint8 *au8BoardCustomDefault; //! Board カスタムデフォルト 
	const uint8 *au8SlotCustomDefault;  //! Slot カスタムデフォルト 
} TWESTG_tsSettings;


/**
 * 設定定義 (Kind, Slotごとの設定一覧が生成される）
 */
typedef struct {
	uint8 u8Kind;
	uint8 u8Slot;
	TWESTG_tsSettings sStgs;
} TWESTG_tsSettingsListItem;

/*!
 * セーブ形式(tsSlice)
 * 
 * [シリアル化セーブ形式]を解釈・生成するための中間構造体。
 * データペイロード、付属情報に分離格納する。
 * 
 * sBuff, psBuff は、[シリアル化セーブ形式]の[シリアル化要素データ集合]部分を、
 * 不要なコピーを避けるため、ポインタとデータ長という形で切り出している(Slice)。
 * 
 */
typedef struct _TWESTG_sSlice {
    TWE_tsBuffer sBuff, *psBuff; //! セーブバッファのデータペイロード部のスライス

    uint8 u8KeyAppId;        //! AppIDから生成したハッシュ値 (CRC8)
    uint8 u8SettingsVer;     //! 設定バージョン(互換性のあるバージョンかどうかを判定する)
    uint8 u8SettingsKind;    //! アプリケーションの設定セットの種別
    uint8 u8SettingsSlot;    //! L:設定スロット 0:Default 1...7:セーブ番号
} TWESTG_tsSlice;

/*!
 * 確定設定リスト(tsFinal)
 * 
 * 設定定義(tsSettings)とセーブデータを反映した設定リスト。
 * アプリケーションからのデータの読み書きは、本リストを介して行う。
 * 
 * 宣言、初期化には TWESTG_DECLARE_FINAL() TWESTG_INIT_FINAL() マクロを用いる。
 * 
 * 設定が N 個ある場合は apEle[0...N-1]までにポインタが格納されており、
 * apEle[N...]はNULLとなる。
 */
typedef struct _TWESTG_sFinal {
    uint8 u8DataCount;          //! 格納データ数
    uint8 u8DataCountMax;       //! 最大データ数
	uint8 u8CustDefCountMax;    //! カスタムデフォルトのカウント最大値
	uint8 u8Opt;                //! bit1: Revertフラグ

	uint32 u32AppId;            //! アプリケーションID（デフォルト）
	uint32 u32AppVer;           //! ファームバージョン

	struct {
		uint8 u8Set;            //! 現在のバージョン
		uint8 u8Compat;         //! 互換性のある下位バージョン
	} sVer;

	uint8 u8Kind;               //! 設定種別（同一アプリ内での設定の組み合わせ）
	uint8 u8Slot;               //! スロット（同一種別内でのバリエーション）
	
	const TWESTG_tsSettings *psSettings; //! 設定定義、デフォルト定義
	const TWESTG_tsSettingsListItem *pasSetList; //! 設定リストの定義
    TWE_tsBuffer *psBuf;		//! 文字列などの可変長データを格納するためのデータプール
    TWESTG_tsDatum *asDatum;	//! データ構造体の配列へのポインタ(セーブデータ)
	TWESTG_tsElement* *apEle;	//! データ定義を格納するための配列(psSettingsのデータをリスト化したもの)
	uint8 *au8ShortcutKeys;		//! インタラクティブモードで設定選択するためのキー
	TWESTG_tuDatum *auDatumCustDef; //! カスタムデフォルト値
} TWESTG_tsFinal;

/*!
 * 確定設定リスト(tsFinal)を参照するためのイテレータ。
 */
typedef struct _TWESTG_ITER_tsFinal {
	TWESTG_tsFinal *p;
	uint8 u8idx;
} TWESTG_ITER_tsFinal;

#define TWESTG_ITER_tsFinal_BEGIN(ptr,f) (ptr.p = f, ptr.u8idx = 0) //! イテレータを初期化する
#define TWESTG_ITER_tsFinal_IS_VALID(ptr) (ptr.p != NULL && ptr.p->u8DataCount > 0) //! イテレータが初期化されているか判定する
#define TWESTG_ITER_tsFinal_IS_END(ptr) (ptr.u8idx >= ptr.p->u8DataCount) //! イテレータが末尾かどうか判定する
#define TWESTG_ITER_tsFinal_INCR(ptr) (ptr.u8idx++) //! イテレータを進める
#define TWESTG_ITER_tsFinal_G_ID(ptr) (ptr.p->apEle[ptr.u8idx]->u16Id) //! 設定ID(teTWESTG_STD_DEFSETS)を取得する

#define TWESTG_ITER_tsFinal_G_U32(ptr) (ptr.p->asDatum[ptr.u8idx].uDatum.u32) //! uint32 形式で値を得る
#define TWESTG_ITER_tsFinal_G_U16(ptr) (ptr.p->asDatum[ptr.u8idx].uDatum.u16) //! uint16 形式で値を得る
#define TWESTG_ITER_tsFinal_G_U8(ptr) (ptr.p->asDatum[ptr.u8idx].uDatum.u8) //! uint8 形式で値を得る
#define TWESTG_ITER_tsFinal_G_I32(ptr) (ptr.p->asDatum[ptr.u8idx].uDatum.i32) //! uint32 形式で値を得る
#define TWESTG_ITER_tsFinal_G_I16(ptr) (ptr.p->asDatum[ptr.u8idx].uDatum.i16) //! uint16 形式で値を得る
#define TWESTG_ITER_tsFinal_G_I8(ptr) (ptr.p->asDatum[ptr.u8idx].uDatum.i8) //! uint8 形式で値を得る
#define TWESTG_ITER_tsFinal_G_PU8(ptr) (ptr.p->asDatum[ptr.u8idx].uDatum.pu8) //! uint8 形式で値を得る

/*****************************************************************************************************************
 * 関数プロトタイプ
 *****************************************************************************************************************/
 
void TWESTG_vInit_tsFinal(TWESTG_tsFinal *psFinal, TWESTG_tsSettings *pDef, TWESTG_tsDatum *aDatum, TWE_tsBuffer *pBuf, TWESTG_tsElement **apEle, uint8 *pShortcutKeys, uint8 u8DataCountMax, TWESTG_tuDatum *auDatumCust, uint8 u8CustDefCountMax);
TWE_APIRET TWESTG_u32FinalSetDefaults(TWESTG_tsFinal *psFinal);
TWE_APIRET TWESTG_u32FinalLoad(TWESTG_tsFinal *psFinal, TWESTG_tsSlice *psSet, uint32 u32Opt);
TWE_APIRET TWESTG_u32FinalSave(TWESTG_tsSlice *psSet, TWESTG_tsFinal *psFinal, uint32 u32Opt);
void TWESTG_vInit_tsSlice(TWESTG_tsSlice *pSet);
TWE_APIRET TWESTG_u32SliceLoad(TWESTG_tsSlice *pSet, TWE_tsBuffer *pBuf); // pBuf を解釈し slice を生成
TWE_APIRET TWESTG_u32SliceSavePrep(TWE_tsBuffer *pBuf, TWESTG_tsSlice *pSet, uint8 u8Ver); // pBuf 領域に slice を割り当てたうえで、tsSlice データ構造の準備を行う（ヘッダ、データバッファのポインタ）
//extern TWE_APIRET TWESTG_cbu32SaveSetting(TWE_tsBuffer *pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal *psFinal);
//extern TWE_APIRET TWESTG_cbu32LoadSetting(TWE_tsBuffer *pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal *psFinal);
uint32 TWESTG_u32GetU32FrUDatum(uint8 u8Type, TWESTG_tuDatum *pDat);
void TWESTG_vSetUDatumFrU32(uint8 u8Type, TWESTG_tuDatum *pDat, uint32 u32val);
void TWESTG_vSetUDatumFrUDatum(uint8 u8Type, TWESTG_tuDatum* pDat, const TWESTG_tuDatum* pRef);


#ifdef __cplusplus
}
#endif


#endif /* TWESETTINGS_H_ */