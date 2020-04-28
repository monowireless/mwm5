/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include <stdio.h>
#include <string.h>
#include "twecommon.h"
#include "msc_eep.h"

static bool_t s_bInit = FALSE;
static uint8 s_au8buff[EEPROM_6X_USER_SIZE];

static const char* s_default_savename = "twestgs.sav";
const char *twesettings_save_filepath = NULL;

static bool_t s_bReadFromFile();
static bool_t s_bWriteToFile();

inline static const char* s_get_savename() {
	return (twesettings_save_filepath == NULL) ? s_default_savename : twesettings_save_filepath;
}

/*!
 * ファイルからデータを読み出す (MSC EEPROM シミュレート用)
 * ファイルがない場合は 0xFF で初期化したデータをセーブする。
 * 
 * \return TRUE:成功 FALSE:失敗
 */
static bool_t s_bReadFromFile() {
	bool_t bRet = FALSE;
	FILE *fp = NULL;
#if defined(_MSC_VER) || defined(__MINGW32__)
	fopen_s(&fp, s_get_savename(), "rb");
#elif defined(__APPLE__) || defined(__linux)
	fp = fopen(s_get_savename(), "rb");
#endif
	if (fp != NULL) {
		int ret = fread(s_au8buff, sizeof(uint8), EEPROM_6X_USER_SIZE, fp); (void)ret;
		fclose(fp);
	}
	else {
		memset(s_au8buff, 0xFF, EEPROM_6X_USER_SIZE);
		s_bWriteToFile();
	}

	s_bInit = TRUE;

	return bRet;
}

/*!
 * ファイルにデータを書き出す (MSC EEPROM シミュレート用)
 * 
 * \return TRUE:成功 FALSE:失敗
 */
static bool_t s_bWriteToFile() {
	bool_t bRet = FALSE;
	FILE *fp = NULL;

#if defined(_MSC_VER) || defined(__MINGW32__)
	fopen_s(&fp, s_get_savename(), "w+b");
#elif defined(__APPLE__) || defined(__linux)
	fp = fopen(s_get_savename(), "w+b");
#endif

	if (fp != NULL) {
		fwrite(s_au8buff, sizeof(uint8), EEPROM_6X_USER_SIZE, fp);
		fclose(fp);
		bRet = TRUE;
	}
	return bRet;
}


/*!
 * EEPROM 読み出し関数 (MSC EEPROM シミュレート用)
 * 
 * \param u16StartAddr
 * \param u16Bytes
 * \param pu8Buffer
 * \return 
 */
bool_t EEP_6x_bRead(uint16  u16StartAddr, uint16 u16Bytes, uint8 *pu8Buffer) {
	bool_t bRet = TRUE;
	if (!s_bInit) s_bReadFromFile(); // 初期化チェック

	memcpy(pu8Buffer, s_au8buff + u16StartAddr, u16Bytes);
	
	return bRet;
}

/*!
 * EEPROM 書き込み関数 (MSC EEPROM シミュレート用)
 * 
 * \param u16StartAddr
 * \param u16Bytes
 * \param pu8Buffer
 * \return 
 */
bool_t EEP_6x_bWrite(uint16 u16StartAddr, uint16 u16Bytes, uint8 *pu8Buffer) {
	bool_t bRet = TRUE;
	if (!s_bInit) s_bReadFromFile(); // 初期化チェック

	memcpy(s_au8buff + u16StartAddr, pu8Buffer, u16Bytes);
	bRet = s_bWriteToFile();

	return bRet;
}

#endif