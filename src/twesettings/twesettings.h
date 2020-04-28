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
	   (TWESTG_DATATYPE_STRINGはMSB=1の場合参照)
     MSB=1 の場合
	   LSBから4bit  -> データ長
	   その他ビット -> reserved
   [OCTET x N]: データ
     データは big endian で格納する。
	 文字列型は 0 終端をしなくてよい。

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
 */

#ifndef TWESETTINGS_H_
#define TWESETTINGS_H_

#include "twecommon.h"
#include "twesettings0.h"
#include "twesettings_callbacks.h"

#endif