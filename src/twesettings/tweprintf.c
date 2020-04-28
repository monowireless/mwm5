/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

 /****************************************************************************/
 /***        printfmt Implemenation                                          ***/
 /****************************************************************************/


#include "twecommon.h"
#include "tweprintf.h"

#include <stdarg.h>

// output to stream.
static void vOutputStream(char out, void* vp) {
	TWE_tsFILE* fp = (TWE_tsFILE*)vp;
	fp->fp_putc(out, fp);
}

/*!
 * printf の実装（シリアル等へ出力）
 * TWE_tsFILE::fp_putc が必要
 * 
 * \param fp      出力ストリーム
 * \param format  出力書式
 * \param         書式に応じたパラメータ
 * \return        -1:失敗 以外:成功（書き出した文字数）
 */
int TWE_fprintf(TWE_tsFILE *fp, const char *format, ...)
{
	if(IS_TWETERM_FILE_INITIALIZED(fp)) {
		va_list va;
		va_start(va, format);
		int ret = fctvprintf(vOutputStream, (void*)fp, format, va);
		va_end(va);
		return ret;
	}
	return -1;
}

/*!
 * printf の実装（シリアル等へ出力） va_list 版
 * TWE_tsFILE::fp_putc が必要
 * 
 * \param fp      出力ストリーム
 * \param format  出力書式
 * \param va       書式に応じたパラメータ
 * \return        -1:失敗 以外:成功（書き出した文字数）
 */
int TWE_vfprintf(TWE_tsFILE *fp, const char *format, va_list va) {
	int ret = -1;
	if(IS_TWETERM_FILE_INITIALIZED(fp)) {
		ret = fctvprintf(vOutputStream, (void*)fp, format, va);
		return ret;
	}
	return ret;
}

/*!
 * sprintf の実装
 * 
 * \param out    出力バッファ
 * \param format 出力書式
 * \param        書式に応じたパラメータ
 * \return 書き出した文字数
 */
#if 0
int TWE_snprintf(char * buffer, size_t count, const char *format, ...)
{
	va_list va;
	va_start(va, format);
	int ret = vsnprintf_(buffer, count, format, va);
	va_end(va);
	return ret;
}
#endif