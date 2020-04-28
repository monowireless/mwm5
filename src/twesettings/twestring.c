/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twecommon.h"
#include "twestring.h"

/*!
 * 文字列を区切り文字で分割する。pstr は NUL 文字で分割される。
 * 
 * \param pstr
 * \param u8max_len
 * \param auptr
 * \param u8max_entry
 * \return 
 */
uint8 TWESTR_u8SplitTokens(uint8 *pstr, uint8 u8max_len, uint8 **auptr, uint8 u8max_entry) {
	uint8 u8num = 0;

	uint8 *p = pstr;
	uint8 *e = pstr + u8max_len - 1; // 終端は一文字余裕を設けておく

	if (pstr == NULL || *p == 0) {
		return 0;
	} else {

		auptr[0] = pstr;
		u8num = 1;

		while (*p && p < e) {
			if (*p == ',' || *p == '.') {
				*p = 0;
				auptr[u8num] = p + 1;
				u8num++;
				if (u8num >= u8max_entry) {
					break;
				}
			}

			p++;
		}
	}

	return u8num;
}

/*!
 * HEX文字列を数値に変換
 * 
 * \param p 文字列へのポインタ
 * \param u8len 文字列の長さ
 * \return 数値
 */
uint32 TWESTR_u32HexstrToNum(uint8 *p, uint8 u8len) {
	uint32 u32res = 0;

    while(u8len--) {
        if (*p == 0) break; // NUL 文字が来たらここでおしまい

		u32res <<= 4; // 16倍する
		if (*p >= '0' && *p <= '9') {
			u32res += *p -'0';
		} else if (*p >= 'A' && *p <= 'F') {
			u32res += *p - 'A' + 10;
		} else if (*p >= 'a' && *p <= 'f') {
			u32res += *p - 'a' + 10;
		} else {
            // 関係ない文字が来たらここでおしまい
            break;
        }
		p++;
	}

	return u32res;
}

 /*!
  * DEC文字列を数値に変換
  * 
  * \param p 文字列へのポインタ
  * \param u8len 文字列の長さ
  * \return 変換された数値
  */
int32 TWESTR_i32DecstrToNum(uint8 *p, uint8 u8len) {
	int32 i32res = 0;
    bool_t bNagaive = FALSE;

    // 先頭の文字が - の場合のチェック
    if (*p == '-') {
        bNagaive = TRUE;
        p++;
        u8len--;
    }

	while(u8len--) { 
        if (*p == 0) break; // NUL 文字が来たらここでおしまい

		i32res *= 10;
		if (*p >= '0' && *p <= '9') {
			i32res += *p -'0';
		} else {
            // 関係ない文字が来たらここでおしまい
            break;
        }
		p++;
	}

	return bNagaive ? -i32res : i32res;
}