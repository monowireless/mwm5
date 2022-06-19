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

#ifndef TWESERIAL_H_
#define TWESERIAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/************************************************
 * 入出力（シリアル）の抽象化
 ************************************************/
typedef struct _tsTWETERM_FILE {
    int (*fp_getc)(struct _tsTWETERM_FILE *fp);
    int (*fp_putc)(int c, struct _tsTWETERM_FILE *fp);
	int (*fp_puts)(const char *s, struct _tsTWETERM_FILE *fp); // OPTIONAL
    void (*fp_flush)(struct _tsTWETERM_FILE *fp);
    
    void* vpContext_output; // pointer to context data
	void* vpContext_input; // pointer to context data#2 (for output)

    union { // extra 4 bytes for optional use (e.g. to store port id.)
        uint8 au8[4];
        uint16 au16[2];
        uint32 au32[1];
        string chr[4];
    } uData;
} TWE_tsFILE;

// シリアルポートの汎用定義
typedef struct _tsTWETERM_SerDefs {
    uint32 u32Baud;
    uint8 u8BitLen; // 8:8Bit
    uint8 u8Parity; // 0:None
    uint8 *au8TxBuf, *au8RxBuf; // local buffer
    uint16 u16TxBufLen, u16RxBufLen; // buffer len
} TWETERM_tsSerDefs;

/************************************************
 * PRINTF, etc
 ************************************************/
int _TWE_fputs(const char *s, TWE_tsFILE *fp);

#define TWE_fputc(c, fp) (((fp)->fp_putc)(c, fp))
#define TWE_fgetc(fp) (((fp)->fp_getc)(fp))
#define TWE_fputs(s, fp) ((fp)->fp_puts ? (fp)->fp_puts : _TWE_fputs)(s, fp)
#define TWE_fflush(fp) (((fp)->fp_flush)(fp))
#define IS_TWETERM_FILE_INITIALIZED(fp) ((fp) && ((fp)->fp_putc != NULL))

#ifdef __cplusplus
}
#endif

#endif // TWESERIAL_H_