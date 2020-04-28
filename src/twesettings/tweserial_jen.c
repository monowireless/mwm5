/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/**
 * fprintf.h を用いた実装(JN51XX専用)
 */

#ifdef JENNIC_CHIP

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <string.h>

#include "twecommon.h"
#include "tweserial.h"
#include "tweserial_jen.h"

#include <AppHardwareApi.h>
#include "serial.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
// access port number of uart from tsTWETERM_FILE.
#ifdef UARTPORT
# undef UARTPORT
#endif
#define UARTPORT uData.au8[0]

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
static int TWETERM_iGetC(TWE_tsFILE *);
static int TWETERM_iPutC(int c, TWE_tsFILE *);
static int TWETERM_iPutS(const char *s, TWE_tsFILE *fp);
static void TWETERM_vFlush(TWE_tsFILE *);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************///

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

// 初期化
void TWETERM_vInitJen(TWE_tsFILE *fp, uint8 u8Port, TWETERM_tsSerDefs *pser) {
    memset(fp, 0, sizeof(TWE_tsFILE));

    fp->UARTPORT = u8Port; // set uart port
    
    fp->fp_getc = TWETERM_iGetC;
    fp->fp_putc = TWETERM_iPutC;
    fp->fp_puts = TWETERM_iPutS;
    fp->fp_flush = TWETERM_vFlush;

	/* シリアルポートの初期化 */
    tsSerialPortSetup sSerPort; // setup information
    memset(&sSerPort, 0, sizeof(sSerPort));
	sSerPort.pu8SerialRxQueueBuffer = pser->au8RxBuf;
	sSerPort.pu8SerialTxQueueBuffer = pser->au8TxBuf;
	sSerPort.u32BaudRate = pser->u32Baud;
	sSerPort.u16AHI_UART_RTS_LOW = 0xffff;
	sSerPort.u16AHI_UART_RTS_HIGH = 0xffff;
	sSerPort.u16SerialRxQueueSize = pser->u16RxBufLen;
	sSerPort.u16SerialTxQueueSize = pser->u16TxBufLen;
	sSerPort.u8SerialPort = u8Port; // E_AHI_UART_0 or 1
	sSerPort.u8RX_FIFO_LEVEL = E_AHI_UART_FIFO_LEVEL_1;
	SERIAL_vInitEx(&sSerPort, NULL);
}

// 1バイト書き出す
static int TWETERM_iPutC(int c, TWE_tsFILE *fp) {
    return (int)SERIAL_bTxChar(fp->UARTPORT, c);
}

// 文字列を書き出す
static int TWETERM_iPutS(const char *s, TWE_tsFILE *fp) {
    int i = 0;
    while (*s) {
        SERIAL_bTxChar(fp->UARTPORT, *s);
        s++; i++;
    }
    return i;
}

// １バイト読み出す
static int TWETERM_iGetC(TWE_tsFILE *fp) {
    int iChar = -1;

    if (!SERIAL_bRxQueueEmpty(fp->UARTPORT)) {
        iChar = SERIAL_i16RxChar(fp->UARTPORT);
    }

    return iChar;
}

// フラッシュ
static void TWETERM_vFlush(TWE_tsFILE *fp) {
    SERIAL_vFlush(fp->UARTPORT);
}


#endif // JENNIC_CHIP