/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/** @file
 *
 * @defgroup TWESER ヘッダファイル
 *
 */

#ifndef TWECOMMON_H_
#define TWECOMMON_H_

/*
 * 基本定義部
 */
#if defined(JENNIC_CHIP_NAME)
/* JENNIC 関連は jendefsより */
#include <jendefs.h>
#else
/* generic stdio debugging, __GNUC__ or _MSC_VER */
#include <stdint.h> // for type name

#if !defined FALSE && !defined TRUE
#define TRUE            (1)   /* page 207 K+R 2nd Edition */
#define FALSE           (0)
#endif /* !defined FALSE && #if !defined TRUE */

#if !defined NULL
#define NULL                    ( (void *) 0 )
#endif

#if !defined PUBLIC
#define PUBLIC
#endif

#if !defined PRIVATE
#define PRIVATE static
#endif

typedef uint8_t                 BOOL_T;     /* boolean type nothing to do with C++ */
typedef uint8_t                 bool_t;     /* boolean type nothing to do with C++ */

typedef int8_t                  int8;
typedef int16_t                 int16;
typedef int32_t                 int32;
typedef int64_t                 int64;
typedef uint8_t                 uint8;
typedef uint16_t                uint16;
typedef uint32_t                uint32;
typedef uint64_t                uint64;

typedef char *                  string;
#endif

/* その他基本定義 */
// 標準API戻り値の定義
typedef uint32 TWE_APIRET;
#define TWE_APIRET_SUCCESS_W_VALUE(v) (0x80000000 + ((v) & 0x7FFFFFFF))
#define TWE_APIRET_FAIL_W_VALUE(v) ((v) & 0x7FFFFFFF)
#define TWE_APIRET_SUCCESS 0x80000000
#define TWE_APIRET_FAIL 0x0
#define TWE_APIRET_VALUE(r) ((r) & 0x7FFFFFFF)
#define TWE_APIRET_IS_SUCCESS(r) ((r) & 0x80000000)
#define TWE_APIRET_IS_FAIL(r) (!TWE_APIRET_IS_SUCCESS(r))

/*!
 * 汎用バッファ(tsBuffer)
 * 確保されたバッファへのポインタ、利用済みのバイト数、最大長を格納し、
 * バッファーオーバーなどのチェックを統一・明示化する。
 */
typedef struct _TWE_sBuffer {
	uint8 *pu8buff;
	uint8 u8bufflen_max;
	uint8 u8bufflen;
} TWE_tsBuffer;

/*!
 * 汎用バッファ(tsBuffer)の初期化
 * 
 * \param b TWE_tsBuffer*
 * \param p バイト列へのポインタ
 * \param l バイト列の利用済みバイト数
 * \param m バイト列の最大長
 */
#define TWE_vInit_tsBuffer(b,p,l,m) ((b)->u8bufflen_max=(m),(b)->u8bufflen=(l),(b)->pu8buff=(p))
 // void TWE_vInit_tsBuffer(TWE_tsBuffer *pBuf, uint8 *pu8Buf, uint8 u8Len, uint8 u8MaxLen); //単純な処理なので関数化はやめる


#endif