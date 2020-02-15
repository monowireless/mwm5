/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */
 
 /****************************************************************************/
 /***        printfmt Implemenation                                          ***/
 /****************************************************************************/

#include <stdarg.h>

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_printf.hpp"
#include "printf/printf.h"

using namespace TWE;

// output to stream. (for printf.h)
static void vOutputStream(char out, void* vp) {
	IStreamOut* os = reinterpret_cast<IStreamOut*>(vp);
	*os << (char_t)out;
}

/*!
 * printf の実装（シリアル等へ出力）
 * 
 * \param os      出力ストリーム
 * \param format  出力書式
 * \param         書式に応じたパラメータ
 * \return        以外:成功（書き出した文字数）
 */
int TWE::fPrintf(IStreamOut& os, const char *format, ...)
{
	va_list va;
	va_start(va, format);
	int ret = fctvprintf(vOutputStream, reinterpret_cast<void*>(&os), format, va);
	va_end(va);
	return ret;
}

/*!
 * snprintf の実装
 * 
 * \param buffer    出力バッファ
 * \param count		バッファサイズ
 * \param format    出力書式
 * \param           書式に応じたパラメータ
 * \return 書き出した文字数
 */
int TWE::snPrintf(char* buffer, size_t count, const char* format, ...) {
	va_list va;
	va_start(va, format);
	int ret = vsnprintf_(buffer, count, format, va);
	va_end(va);
	return ret;
}
