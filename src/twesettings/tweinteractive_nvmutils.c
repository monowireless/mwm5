/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * 
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(JENNIC_CHIP_NAME)
#include <jendefs.h>
#include <AppHardwareApi.h>
#elif defined(ESP32)
#include "esp32_eep.h"
#define vAHI_WatchdogRestart()
#elif defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)
#include "msc_eep.h"
#define vAHI_WatchdogRestart()
#endif

 /* generic stdio debugging, __GNUC__ or _MSC_VER */
#include <string.h>

#include "twecommon.h"
#include "twesettings0.h"
#include "twesettings_std.h"
#include "twesettings_validator.h"
#include "tweinteractive.h"
#include "tweinteractive_keycode.h"
#include "twenvm.h"

#include "twesettings_weak.h"

#define MENU_ITEM_COUNT 4
static const char menu_string[MENU_ITEM_COUNT][24] = {
	"r: Read sector.",
	"R: Read ALL sectors.",
	"e: Erase sector.",
	"E: Erase ALL sectors.",
};

#if 0
static void s_DumpMem(TWE_tsFILE *pSer, uint8 *p, uint8 l, uint16 u16AddrStart) {
    int i;

    for (i = 0; i < l; i++) {
        if ((i & 0xF) == 0) {
            TWE_fprintf(pSer, _TWELB"%04x:", u16AddrStart + i);
            TWE_fflush(pSer);
        }
        TWE_fprintf(pSer, " %02x", p[i]);
    }
    TWE_fputs(_TWELB, pSer);
}
#endif

static bool_t s_bDumpSector(TWE_tsFILE *pSer, uint8 u8sec) {
	uint8 u8buff[EEPROM_6X_SEGMENT_SIZE];
	bool_t bRet;
	uint16 u16AddrStart = u8sec * EEPROM_6X_SEGMENT_SIZE;

	bRet = EEP_6x_bRead(EEPROM_6X_SEGMENT_SIZE * u8sec, EEPROM_6X_SEGMENT_SIZE, u8buff);

	if (bRet) {	
		int i;
		for (i = 0; i < EEPROM_6X_SEGMENT_SIZE; i++) {
			if ((i & 0xF) == 0) {
				TWE_fprintf(pSer, _TWELB "%04x:", u16AddrStart + i);
				TWE_fflush(pSer);
			}
			if((i & 0x3) == 0) {
				TWE_fputc(' ', pSer);
			}
			TWE_fprintf(pSer, "%02x", u8buff[i]);
		}
	} else {
		TWE_fprintf(pSer, _TWELB "%04x: ...(error)...", u16AddrStart);
	}

	return bRet;
}

/*!
 * Output one line to the display
 */
static inline void s_disp_menuitem(TWEINTRCT_tsContext *psIntr, const char* itemtext, int index) {
	TWE_fputs(_TWET_INV, psIntr->pStream);
	TWE_fputc(itemtext[0], psIntr->pStream);
	TWE_fputs(_TWET_RST, psIntr->pStream);
	TWE_fputc(itemtext[1], psIntr->pStream);
	TWE_fputc(itemtext[2], psIntr->pStream);

	if (index == psIntr->i16SelectedIndex) {
		TWE_fputs(_TWET_INV, psIntr->pStream);
	}

	TWE_fputs(itemtext + 3, psIntr->pStream);
	TWE_fputs(_TWELB, psIntr->pStream);

	if (index == psIntr->i16SelectedIndex) {
		TWE_fputs(_TWET_RST, psIntr->pStream);
	}
}

/*!
 * インタラクティブモードで画面クリアと設定の全表示を行う
 */
void TWEINTCT_vSerUpdateScreen_nvmutils(TWEINTRCT_tsContext *psIntr) {
	TWEINTRCT_vSerHeadLine(psIntr, 0UL);
	TWE_fputs(_TWELB, psIntr->pStream);

	int i;
	for (i = 0; i < MENU_ITEM_COUNT; i++) {
		s_disp_menuitem(psIntr, menu_string[i], i);
	}

	TWE_fputs(_TWELB, psIntr->pStream);
	TWEINTRCT_vSerFootLine(psIntr, 0); // フッター行の表示
	TWE_fflush(psIntr->pStream);
}

/*!
 * インタラクティブモードでの１バイトコマンド入力を処理する。
 * 
 * \param keycode 入力バイト
 */
void TWEINTCT_vProcessInputByte_nvmutils(TWEINTRCT_tsContext *psIntr, TWEINTRCT_tkeycode keycode) {
	bool_t bHandled = FALSE;
	bool_t bForceRedraw = FALSE;
	TWE_APIRET ret = 0;

	psIntr->u16HoldUpdateScreen = 0;

	// commit selection
	if (keycode == _TWECR) {
		if (psIntr->i16SelectedIndex >= 0 && psIntr->i16SelectedIndex < MENU_ITEM_COUNT) {
			keycode = menu_string[psIntr->i16SelectedIndex][0]; // the first byte of menu string is shortcut key.
		}
	}

	switch (keycode) {		
	case TWEINTRCT_KEY_UP:
		if (psIntr->i16SelectedIndex > 0) {
			psIntr->i16SelectedIndex--;
		}
		break;

	case TWEINTRCT_KEY_DOWN:
		if (psIntr->i16SelectedIndex < MENU_ITEM_COUNT - 1) {
			psIntr->i16SelectedIndex++;
		}
		break;

	case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9': // DIRECT SECTOR READ
		{
			uint8 u8n = keycode - '0';

			u8n *= 4;
			TWE_fprintf(psIntr->pStream, "\033[2J\033[H", u8n, u8n+3);
			s_bDumpSector(psIntr->pStream, u8n++);
			s_bDumpSector(psIntr->pStream, u8n++);
			s_bDumpSector(psIntr->pStream, u8n++);
			s_bDumpSector(psIntr->pStream, u8n++);
			bHandled = TRUE;
		}
		break;
	
	case 'r':
		TWE_fprintf(psIntr->pStream, _TWELB"Input Sector to READ: ");
		TWEINPSTR_vStart(psIntr->pSerInpStr, E_TWEINPUTSTRING_DATATYPE_DEC, 2, 'r');
		bHandled = TRUE;
		break;

	case 'R':
		TWE_fprintf(psIntr->pStream, _TWELB"All Sectors READ(type 'YES' and Enter)?: ");
		TWEINPSTR_vStart(psIntr->pSerInpStr, E_TWEINPUTSTRING_DATATYPE_STRING, 3, 'R');
		bHandled = TRUE;
		break;

	case 'e':
		TWE_fprintf(psIntr->pStream, _TWELB"Input Sector to ERASE: ");
		TWEINPSTR_vStart(psIntr->pSerInpStr, E_TWEINPUTSTRING_DATATYPE_DEC, 2, 'e');
		bHandled = TRUE;
		break;

	case 'E':
		TWE_fprintf(psIntr->pStream, _TWELB"All Sectors ERASE(type 'YES' and Enter)?: ");
		TWEINPSTR_vStart(psIntr->pSerInpStr, E_TWEINPUTSTRING_DATATYPE_STRING, 3, 'E');
		bHandled = TRUE;
		break;
	
	default:
		ret = TWEINTRCT_u32MenuOpKey(psIntr, keycode);

		if (TWE_APIRET_IS_SUCCESS(ret)) {
			bHandled = TRUE;
			if (TWE_APIRET_VALUE(ret) == 1) {
				bForceRedraw = TRUE;
			}
		}
		break;
	}

	// 処理されなかったキーはコールバックで処理
	//int i = TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINRCT_OP_UNHANDLED_CHAR, u8Byte, u8lastbyte, NULL);
	//if (i) bHandled = TRUE;

	// 処理されない場合
	if (!bHandled || bForceRedraw) {
		psIntr->u16HoldUpdateScreen = 5;
	}
}

/*!
 * 入力処理(TWEINPSTR)完了時の処理
 * 
 * 入力された文字列をバリデータ(TWESTGS_VLD_*)に渡し、要素データ(tuDatum)への変換と
 * データのチェックを行う。
 * 
 * \param pContext 入力処理(TWEINPSTR)
 */
void TWEINTCT_vProcessInputString_nvmutils(TWEINTRCT_tsContext *psIntr, TWEINPSTR_tsInpStr_Context *pContext) {
	uint8 u8sec = 0;
	psIntr->u16HoldUpdateScreen = 0;
	bool_t bRet;

	// 処理
	switch (pContext->u32Opt & 0xFF) {
		case 'r':
			u8sec = TWESTR_i32DecstrToNum(pContext->au8Data, pContext->u8Idx);
			TWE_fputs(_TWELB, psIntr->pStream);
			bRet = s_bDumpSector(psIntr->pStream, u8sec);
			if (!bRet) {
				TWE_fputs("(READ ERROR)"_TWELB, psIntr->pStream);
				psIntr->u16HoldUpdateScreen = 96;
			}
			break;

		case 'R':
			if(pContext->au8Data[0] == 'Y' && pContext->au8Data[1] == 'E' && pContext->au8Data[2] == 'S') {
				u8sec = 0;
				TWE_fputs(_TWELB, psIntr->pStream);
				while(u8sec < EEPROM_6X_USER_SEGMENTS) {
					bRet = s_bDumpSector(psIntr->pStream, u8sec);
					vAHI_WatchdogRestart();
					u8sec++;
				}
			} else {
				TWE_fputs("(canceled)", psIntr->pStream);
				psIntr->u16HoldUpdateScreen = 96;
			}
			break;

		case 'e':
			u8sec = TWESTR_i32DecstrToNum(pContext->au8Data, pContext->u8Idx);
			TWE_fputs(_TWELB, psIntr->pStream);
			s_bDumpSector(psIntr->pStream, u8sec);
			TWE_fprintf(psIntr->pStream, _TWELB"Erase %d (type 'Y' and Enter)?: ", u8sec);
			TWEINPSTR_vStart(psIntr->pSerInpStr, E_TWEINPUTSTRING_DATATYPE_STRING, 1, 0x80 | 'e' | (u8sec << 8));
			break;

		case (0x80 | 'e'): // Force Erace
			if (pContext->au8Data[0] == 'Y') {
				u8sec = (pContext->u32Opt >> 8) & 0xFF;
				bRet = TWENVM_bErase(u8sec);
				if (bRet) {
					TWE_fputs(_TWELB "(ERASED)", psIntr->pStream);
				} else {
					TWE_fputs("(ERASE ERROR)", psIntr->pStream);
				}
				psIntr->u16HoldUpdateScreen = 96;
			} else {
				TWE_fputs("(canceled)", psIntr->pStream);
				psIntr->u16HoldUpdateScreen = 96;
			}
			break;

		case 'E':
			if(pContext->au8Data[0] == 'Y' && pContext->au8Data[1] == 'E' && pContext->au8Data[2] == 'S') {
				u8sec = 0;
				TWE_fputs(_TWELB, psIntr->pStream);
				while(u8sec < EEPROM_6X_USER_SEGMENTS) {
					TWE_fputc('.', psIntr->pStream);
					bool_t b = TWENVM_bErase(u8sec);
					u8sec++;
				}
				TWE_fputs(_TWELB "(ERASED)", psIntr->pStream);
				psIntr->i16SelectedIndex = -1;
				psIntr->u16HoldUpdateScreen = 0;
			} else {
				TWE_fputs("(canceled)", psIntr->pStream);
				psIntr->u16HoldUpdateScreen = 96;
			}
			break;
			
		default:
		break;
	}
}

/**
 * @brief NVMEM 用のメニュー
 * 
 * @param pContext 
 * @param u32Op 
 * @param u32Arg1 
 * @param u32Arg2 
 * @param vpArg 
 * @return TWE_APIRET 
 */
TWE_APIRET TWEINTCT_u32ProcessMenuEvent_nvmutils(TWEINTRCT_tsContext *psIntr, uint32 u32Op, uint32 u32Arg1, uint32 u32Arg2, void *vpArg) {
	TWE_APIRET ret = TWE_APIRET_FAIL;

	switch (u32Op) {
		case E_TWEINTCT_MENU_EV_LOAD:
			TWEINTRCT_cbu32GenericHandler(psIntr, E_TWEINTCT_MENU_EV_LOAD, psIntr->u8screen, 0, NULL); // メニューへの遷移（初期化）をアプリケーションに伝える
			ret = TWE_APIRET_SUCCESS;

			psIntr->i16SelectedIndex = -1;
		break;

		default:
		break;

	}

	return ret;
}