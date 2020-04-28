/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/** @file
 *
 * @defgroup TWESER ヘッダファイル
 *
 * シリアル等の入出力の抽象定義を行う。
 */

#ifndef TWEUTILS_H_
#define TWEUTILS_H_

#ifdef __cplusplus
extern "C" {
#endif


/*!
 * ポインタpより１バイト取得しpをインクリメントする 
 */
#define TWE_G_OCTET(p) (*(p)++)

/*!
 * ポインタpより２バイト(WORD)取得しpを２バイトインクリメントする（ビッグエンディアン形式）
 */
#define TWE_G_WORD(p) ((p)+=2, ((uint16)*((p)-1) | ((uint16)*((p)-2) << 8)))

/*!
 * ポインタpより４バイト(DWORD)取得しpを４バイトインクリメントする（ビッグエンディアン形式）
 */
#define TWE_G_DWORD(p) ((p)+=4, ((uint32)*((p)-1) | ((uint32)*((p)-2) << 8) | ((uint32)*((p)-3) << 16) | ((uint32)*((p)-4) << 24)))

/*!
 * ポインタqの位置にバイトcを格納する。qはインクリメントする。 
 */
#define TWE_S_OCTET(q, c) ( *(q) = c, (q)++ )

/*!
 * ポインタqの位置にWORD cを格納する。qは２バイト分インクリメントする。（ビッグエンディアン形式）
 */
#define TWE_S_WORD(q, c) ( \
	*(q) = ((c)>>8)&0xff, (q)++, \
	*(q) = ((c)&0xff), (q)++ )

/*!
 * ポインタqの位置にDWORD cを格納する。qは２バイト分インクリメントする。（ビッグエンディアン形式）
 */
#define TWE_S_DWORD(q, c) ( \
	*(q) = ((c)>>24)&0xff, (q)++, \
	*(q) = ((c)>>16)&0xff, (q)++, \
	*(q) = ((c)>>8)&0xff, (q)++, \
	*(q) = ((c)&0xff), (q)++ )

/*!
 * ポインタ q の位置に src から len バイト分コピーする。
 */
#define TWE_S_STRING(q, src, len) { memcpy((q), (src), (len)); (q) += (len); }

/*!
 * a,b,c,d ４バイトよりDWORDのデータを作る。
 */
#define TWE_MAKE_U32_BY_FOURBYTES(a,b,c,d) ((a) << 24 | (b) << 16 | (c) << 8 | (d))

/*!
 * s だけ右にシフトしたバイトを得る。
 */
#define TWE_B(c, s) ((c >> s) & 0xFF)

/*!
 * スコープ作成のためのダミーマクロ
 */
#define _TWESC(f) if(f) 

/*!
 * 改行文字列 (CRLF)
 */
#define _TWELB "\r\n"

#define _TWECR 0x0d //! CR
#define _TWELF 0x0a //! LF
#define _TWECLR 0x0c //! Ctrl-L
#define _TWEBS 0x08 //! BS
#define _TWETAB 0x09 //! TAB
#define _TWEESC 0x1b //! ESC
#define _TWEEOT 0x04 //! EOT

#define _TWET_INV "\033[7m" //! 反転
#define _TWET_BLD "\033[1m" //! BOLD
#define _TWET_RST "\033[0m" //! 表示リセット
#define _TWET_CLS "\033[2J\033[H" //! クリアスクリーン

#ifdef __cplusplus
}
#endif


#endif