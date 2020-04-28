/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <string.h>

#if defined(JENNIC_CHIP_NAME)
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppApi.h>
#elif defined(ESP32)
#include "esp32_eep.h"
#elif defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)
#include "msc_eep.h"
#endif

#include "twecommon.h"
#include "tweutils.h"
#include "twecrc8.h"

#include "twenvm.h"

#define NVM_MAXBUFFER 128
#define NVM_HEADER_LEN 6

#define FLASH_MAGIC_NUMBER (0xA501EF5A) //!< フラッシュ書き込み時のマジック番号  @ingroup FLASH

// コンパイルチェック
#if (EEPROM_6X_SEGMENT_SIZE != 64)
# error "EEPROM_6X_SEGMENT_SIZE is not 64."
#endif

/**
 * @brief ブロックイレース用の 0xFF 配列
 */
static const uint8 s_au8blkFF[EEPROM_6X_SEGMENT_SIZE] = { // 64bytes block
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
};

/* SERIAL DEBUG */
#undef DEBUG_SER
#ifdef DEBUG_SER
#include "tweserial.h"
#include "tweprintf.h"
#include "tweutils.h"
static TWE_tsFILE *pSer;
void TWENVR_vDebugSer(TWE_tsFILE *_pSer) { pSer = _pSer; }
#define DEBUGOUT(...) TWE_fprintf(pSer, __VA_ARGS__)
#else
#define DEBUGOUT(...)
#endif

#ifdef DEBUG_SER
void DEBUGPRINTMEM(uint8 *p, uint8 l) {
    int i;

    for (i = 0; i < l; i++) {
        if ((i & 0xF) == 0) {
            TWE_fprintf(&sSer, _TWELB"%04x:", i);
            TWE_fflush(&sSer);
        }
        TWE_fprintf(&sSer, " %02x", p[i]);
    }
    TWE_fputs(_TWELB, &sSer);
}
#else
#define DEBUGPRINTMEM(p, l)
#endif

/**
 * @brief セクター１ブロックを読み出す
 * 
 * @param u8buff 
 * @param u8sector 
 * @return bool_t 
 */
bool_t TWENVM_bReadSectorRaw(uint8 *u8buff, uint8 u8sector, uint8 u8len) {
    uint16 u16start = EEPROM_6X_SEGMENT_SIZE * u8sector;
    return EEP_6x_bRead(u16start, u8len == 0 ? EEPROM_6X_SEGMENT_SIZE : u8len, u8buff);
}

/**
 * @brief EEPROM から読み出す。
 * 
 * @param pBuff 
 * @param u8sector 
 * @return bool_t 
 */
bool_t TWENVM_bRead(TWE_tsBuffer *pBuff, uint8 u8sector) {
    bool_t bRet = TRUE;

    uint8 u8buff[NVM_MAXBUFFER], *p = u8buff;
    uint16 u16start = EEPROM_6X_SEGMENT_SIZE * u8sector;
    uint8 u8len = 0;
    uint8 u8crc = 0;

    uint8 u8sbyte = 0;

    // 頭の８バイトを読み出す
    if (bRet && EEP_6x_bRead(u16start, 8, u8buff)) {
        uint32 u32magic = TWE_G_DWORD(p);
        u8len = TWE_G_OCTET(p);
        u8crc = TWE_G_OCTET(p);
        DEBUGOUT(_TWELB"NVMREAD:sec=%d/%08X/len=%02d/crc=%02X", u8sector, u32magic, u8len, u8crc);
        DEBUGPRINTMEM(u8buff, 8);

        if (u32magic != FLASH_MAGIC_NUMBER || u8len == 0) {
            DEBUGOUT(_TWELB"NVMREAD:ERR:Check Magic, no savedata?", 0);
            bRet = FALSE;
        }
    } else {
        DEBUGOUT(_TWELB"NVMREAD:ERR:eeprom read error(1).", 0);
        bRet = FALSE;
    }

    // 必要バイト数読み出す
    if (bRet && EEP_6x_bRead(u16start, u8len + NVM_HEADER_LEN, u8buff)) { // 余分に読みだす
        // CRC チェック
        uint8 c = TWE_CRC8_u8Calc(p, u8len); // p はペイロードの先頭
        DEBUGPRINTMEM(u8buff, u8len + NVM_HEADER_LEN);

        if (u8crc != c) {
            DEBUGOUT("NVMREAD:ERR:CRC", 0);
            bRet = FALSE;            
        }
    } else {
        DEBUGOUT("NVMREAD:ERR:eeprom read error(2).", 0);
        bRet = FALSE;
    }

    // バッファに戻す
    if (bRet) {
        memcpy(pBuff->pu8buff, u8buff + NVM_HEADER_LEN + u8sbyte, u8len);
        pBuff->u8bufflen = u8len;
    }

    return bRet;
}

/**
 * @brief 書き込みを行う
 * 
 * @param pBuff 
 * @param u8sector 
 * @return bool_t 
 */
bool_t TWENVM_bWrite(TWE_tsBuffer *pBuff, uint8 u8sector) {
    bool_t bRet = FALSE;
    
    uint8 u8buff[NVM_MAXBUFFER];
    uint8 *q = u8buff;
    int i;
    
    uint8 u8start = u8sector;
    uint8 u8end = u8start + ((pBuff->u8bufflen + NVM_HEADER_LEN) / EEPROM_6X_SEGMENT_SIZE);

    DEBUGOUT("NVMWRT:ssec=%d/esec=%d/bytes=%d."_TWELB, u8start, u8end, pBuff->u8bufflen);

    // ブロックイレース
    for (i = u8sector; i <= u8end; i++) {
		TWENVM_bErase(i);
    }

    // データ書き込み
    TWE_S_DWORD(q, FLASH_MAGIC_NUMBER);
    TWE_S_OCTET(q, pBuff->u8bufflen);
    uint8 u8crc = TWE_CRC8_u8Calc(pBuff->pu8buff, pBuff->u8bufflen);
    TWE_S_OCTET(q, u8crc);
    TWE_S_STRING(q, pBuff->pu8buff, pBuff->u8bufflen);
    
    // 書き込み
    if (EEP_6x_bWrite(u8sector * (uint32)EEPROM_6X_SEGMENT_SIZE, (uint8)(q - u8buff), u8buff)) {
        bRet = TRUE;
        DEBUGOUT("NVMWRT:Success(%dbytes)"_TWELB, q - u8buff);
        DEBUGPRINTMEM(u8buff, q - u8buff);
    }

    // returns
    return bRet;
}

/**
 * @brief セクター消去を行う
 * 
 * @param u8sector 削除セクター番号
 * @return bool_t 成功・失敗
 */
bool_t TWENVM_bErase(uint8 u8sector) {
    return EEP_6x_bWrite(u8sector * (uint32)EEPROM_6X_SEGMENT_SIZE, EEPROM_6X_SEGMENT_SIZE, (uint8 *)s_au8blkFF);
}
