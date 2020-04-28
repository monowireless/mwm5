/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdarg.h>

#include "twecommon.h"

#include "tweserial.h"
#include "twesettings0.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/**
 * fputc を用いた fputs 実装
 *
 * @param s   出力文字列
 * @param fp  出力ストリーム
 * @return 0
 */
int _TWE_fputs(const char *s, TWE_tsFILE *fp) {
	while (*s != '\0') {
		fp->fp_putc(*s++, fp);
	}
	return 0;
}