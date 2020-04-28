/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <string.h>

#include <mwm5.h>
#include "twesettings/twesettings_weak.h"

#include "menu_info.h"


/**
 * @fn	static void s_output_within_width(ITerm& trm, SmplBuf_WChar& str, int maxw)
 *
 * @brief	Output string within 'maxw'.
 * 			if excesses, put ... in the middle.
 *
 * @param [in,out]	trm 	The trm.
 * @param [in,out]	str 	The string.
 * @param 		  	maxw	The maxw.
 */
static void s_output_within_width(ITerm& trm, const SmplBuf_WChar& str, int maxw) {
	if (str.size() > uint32_t(maxw)) {
		int left = (maxw - 3) * 1 / 3;
		int right = (maxw - 3) * 2 / 3;
		trm << std::make_pair(str.cbegin(), str.cbegin() + left);
		trm << L"...";
		trm << std::make_pair(str.cend() - right, str.cend());
	}
	else {
		trm << str;
	}
}
/*!
 * update screen.
 */
void TWEINTCT_vSerUpdateScreen_stage_info(TWEINTRCT_tsContext *psIntr) {
	TWE_tsFILE* file = psIntr->pStream;

	TWEINTRCT_vSerHeadLine(psIntr, 0UL);
	TWE_fputs(_TWELB, psIntr->pStream);

	ITerm& trm = *reinterpret_cast<ITerm*>(file->vpContext_output);

	trm << L"TWELITE STAGE (C) 2020 MONO WIRELESS INC." << crlf;
	trm << L"TWELITE 無線モジュールをお楽しみください！" << crlf;
	trm << crlf;

#ifndef ESP32
	trm << L"[フォルダ]" << crlf;
	int maxw = 320 / 6 - 8;
	trm << "BIN   =";
	s_output_within_width(trm, the_cwd.get_dir_tweapps(), maxw);
	trm << crlf << "WksAct=";
	s_output_within_width(trm, the_cwd.get_dir_wks_acts(), maxw);
	trm << crlf << "WksApp=";
	s_output_within_width(trm, the_cwd.get_dir_wks_tweapps(), maxw);
	trm << crlf << "SDK   =";
	s_output_within_width(trm, the_cwd.get_dir_sdk(), maxw);
#endif

	TWE_fputs(_TWELB, psIntr->pStream);
	TWEINTRCT_vSerFootLine(psIntr, 0); // footer line
	TWE_fflush(psIntr->pStream);
}

/*!
 * handles key input
 */
void TWEINTCT_vProcessInputByte_stage_info(TWEINTRCT_tsContext *psIntr, TWEINTRCT_tkeycode keycode) {
	psIntr->u16HoldUpdateScreen = 1; // must set 1 (to refresh screen)
	TWEINTRCT_u32MenuOpKey(psIntr, keycode); // handle ESC or other keys in the library
}

void TWEINTCT_vProcessInputString_stage_info(TWEINTRCT_tsContext *psIntr, TWEINPSTR_tsInpStr_Context *pContext) { }
TWE_APIRET TWEINTCT_u32ProcessMenuEvent_stage_info(TWEINTRCT_tsContext *psIntr, uint32 u32Op, uint32 u32Arg1, uint32 u32Arg2, void *vpArg) {
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