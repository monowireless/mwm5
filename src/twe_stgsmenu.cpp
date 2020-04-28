/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twelite0.hpp"
#include "twesettings/tweset.h"

#include "twe_stgsmenu.hpp"

// version
#include "version_weak.h"

using namespace TWE;

#define APPID 0x12345678   //! アプリケーションID
#define APPVER ((MWM5_APP_VERSION_MAIN << 24) | (MWM5_APP_VERSION_SUB << 16) | (MWM5_APP_VERSION_VAR << 8))  //! バージョン番号

/*!
 * データセーブを行う。twesettings ライブラリから呼び出されるコールバック関数。
 *
 * 本関数ではテストを目的として au8SaveRegion[][] にデータを格納する。
 * 格納時の書式は [LEN] [pBuf の内容 ...] である。
 *
 * \param pBuf   データ領域 pBuf->pu8buff[-16..-1] を利用することができる。
 *               NULL の場合は、当該領域の初期化（少なくとも先頭の４バイトをクリア）
 * \param u8kind 種別
 * \param u8slot スロット
 * \param u32Opt オプション
 * \param
 * \return
 */
TWE_APIRET TWESTG_cbu32SaveSetting(TWE_tsBuffer* pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal* psFinal) {
	if (pBuf != NULL) {
		bool_t bRes = TWENVM_bWrite(pBuf, u8slot * 4 + 1); //先頭セクターはコントロールブロックとして残し、4セクター単位で保存 (1セクター=64byte)
		return bRes ? TWE_APIRET_SUCCESS : TWE_APIRET_FAIL;
	}
	else {
		// pBuf が NULL の時は、該当の EEPROM 領域を初期化する。
		TWENVM_bErase(u8slot * 4 + 1);
		return TWE_APIRET_SUCCESS;
	}
}

// データロード
// データセーブを行う。twesettings ライブラリから呼び出されるコールバック関数。
TWE_APIRET TWESTG_cbu32LoadSetting(TWE_tsBuffer* pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal* psFinal) {
	bool_t bRes = TWENVM_bRead(pBuf, u8slot * 4 + 1); //先頭セクターはコントロールブロックとして残し、4セクター単位で保存 (1セクター=64byte)

#if 0
	Serial.printf("TWENVM_bRead: res=%d ", bRes);
	int i;
	for (i = 0; i < pBuf->u8bufflen; i++) {
		Serial.printf("%02X ", pBuf->pu8buff[i]);
	}
	Serial.printf("\n");
#endif

	return bRes ? TWE_APIRET_SUCCESS : TWE_APIRET_FAIL;
}

/*!
 * 諸処理を行うコールバック。
 * 主としてインタラクティブモードから呼び出されるが、一部は他より呼び出される。
 *
 * \param pContext インタラクティブモードのコンテキスト(NULLの場合はインタラクティブモード以外からの呼び出し)
 * \param u32Op    コマンド番号
 * \param u32Arg1  引数１（役割はコマンド依存）
 * \param u32Arg2  引数２（役割はコマンド依存）
 * \param vpArg    引数３（役割はコマンド依存、主としてデータを戻す目的で利用する）
 * \return コマンド依存の定義。TWE_APIRET_FAILの時は何らかの失敗。
 */
TWE_APIRET TWEINTRCT_cbu32GenericHandler(TWEINTRCT_tsContext* pContext, uint32 u32Op, uint32 u32Arg1, uint32 u32Arg2, void* vpArg) {
	uint32 u32ApiRet = TWE_APIRET_SUCCESS;
	
	TweStgsMenu *pMenu = reinterpret_cast<TweStgsMenu*>(pContext->pvWrapObjCpp);
	if (!pMenu) return TWE_APIRET_FAIL;

	uint8& u8AppKind = pMenu->get_kind();
	uint8& u8AppSlot = pMenu->get_slot();

	switch (u32Op) {
	case E_TWEINTCT_MENU_EV_LOAD:
		u32ApiRet = TWE_APIRET_SUCCESS_W_VALUE(((uint32)u8AppKind << 8) | u8AppSlot);
		break;

	case E_TWEINRCT_OP_UNHANDLED_CHAR: // 未処理文字列があった場合、呼び出される。
		break;

	case E_TWEINRCT_OP_RESET: // モジュールリセットを行う
		pMenu->vAppLoadData(u8AppKind, u8AppSlot, FALSE);
		break;

	case E_TWEINRCT_OP_REVERT: // 設定をもとに戻す。ただしセーブはしない。u32Arg1がTRUEならデフォルトに再設定。
		pMenu->vAppLoadData(u8AppKind, u8AppSlot, u32Arg1);
		break;

	case E_TWEINRCT_OP_CHANGE_KIND_SLOT:
		// KIND/SLOT の切り替えを行う。切り替え後 pContext->psFinal は、再ロードされること。

		// u32Arg1,2 0xFF: no set, 0xFF00: -1, 0x0100: +1, 0x00?? Direct Set

#if 0
		// KIND の設定
		if (u32Arg1 != 0xFF) {
			if ((u32Arg1 & 0xff00) == 0x0000) {
				u8AppSlot = u32Arg1 & 0xFF;
			}
			else {
				if ((u32Arg1 & 0xff00) == 0x0100) {
					u8AppKind++;
				}
				else {
					u8AppKind--;
				}
			}
		}
#else
		u8AppKind = 0; // fixed
#endif

		// SLOT の設定
		if (u32Arg2 != 0xFF) {
			if ((u32Arg2 & 0xff00) == 0x0000) {
				u8AppSlot = u32Arg2 & 0x7;
			} else {
				if ((u32Arg2 & 0xff00) == 0x0100) {
					u8AppSlot++;
				}
				else {
					u8AppSlot--;
				}
			}
			if (u8AppSlot > 0xF0) {
				u8AppSlot = pMenu->get_max_slotnumber();
			}
			if (u8AppSlot > pMenu->get_max_slotnumber()) {
				u8AppSlot = 0;
			}
		}

		pMenu->vAppLoadData(u8AppKind, u8AppSlot, FALSE); // 設定を行う

		// 値を戻す。
		// ここでは設定の失敗は実装せず、SUCCESS としている。
		// VALUE は現在の KIND と SLOT。
		u32ApiRet = TWE_APIRET_SUCCESS_W_VALUE((uint16)u8AppKind << 8 | u8AppSlot);
		break;

	case E_TWEINTCT_OP_GET_MAX_SLOT:
		u32ApiRet = TWE_APIRET_SUCCESS_W_VALUE(pMenu->get_max_slotnumber());
		break;

	case E_TWEINRCT_OP_WAIT: // 一定時間待つ
		TWESYS::Sleep_ms(u32Arg1);
		break;

	case E_TWEINRCT_OP_GET_APPNAME: // CONFIG行, アプリ名
		if (vpArg != NULL) {
			// &(char *)vpArg: には、バッファ16bytesのアドレスが格納されているため strcpy() でバッファをコピーしてもよいし、
			// 別の固定文字列へのポインタに書き換えてもかまわない。
			//*((const char**)vpArg) = "(APPNAME)";
		}
		break;

	case E_TWEINRCT_OP_GET_KINDNAME: // CONFIG行, KIND種別名
		if (vpArg != NULL) {
			// &(char *)vpArg: には、バッファ16bytesのアドレスが格納されているため strcpy() でバッファをコピーしてもよいし、
			// 別の固定文字列へのポインタに書き換えてもかまわない。
			//*((const char**)vpArg) = "(KIND)";
		}
		break;

	case E_TWEINRCT_OP_GET_SLOTNAME: // CONFIG行, KIND種別名
		if (vpArg != NULL) {
			// &(char *)vpArg: には、バッファ16bytesのアドレスが格納されているため strcpy() でバッファをコピーしてもよいし、
			// 別の固定文字列へのポインタに書き換えてもかまわない。
			//*((const char**)vpArg) = "(KIND)";
			const char* pname = nullptr;
			if (u32Arg2 >= 0 && u32Arg2 <= pMenu->get_max_slotnumber()) {
				pname = pMenu->get_slot_name(u32Arg2);
			}

			if (pname && pname[0] != '\0') {
				*((const char**)vpArg) = pname;
			}
		}
		break;

	case E_TWEINTCT_OP_GET_OPTMSG:
		if (vpArg != NULL) {
			// &(char *)vpArg: には、バッファ32bytesのアドレスが格納されているため strcpy() でバッファをコピーしてもよいし、
			// 別の固定文字列へのポインタに書き換えてもかまわない。
			char* q = *((char**)vpArg);
			*q = '\0';
			// TWE_snprintf(q, 16, "<tick=%d>", TWESYS::u32GetTick_ms());
		}
		break;

	case E_TWEINTCT_OP_GET_SID: // シリアル番号
		if (vpArg != NULL) {
			// シリアル値を書き込む
			//*((uint32*)vpArg) = 0x81234567;
		}
		break;

	case E_TWEINTCT_OP_EXIT: // EXIT from intaractive mode (post press long A event)
		TWE::TweStgsMenu::key_unhandled.push(TWECUI::KeyInput::KEY_BUTTON_A_LONG);
		break;

	default:
		break;
	}

	return u32ApiRet;
}


/*!
 * 確定設定データを再構築する。
 *
 * \param u8kind  種別
 * \param u8slot  スロット
 * \param bNoLoad TRUEならslotに対応するセーブデータを読み込まない。
 */
void TweStgsMenu::vAppLoadData(uint8 u8kind, uint8 u8slot, bool_t bNoLoad) {
	TWE_APIRET ret;

	// tsFinal 構造体の初期化
	stg.init();
	
	// tsFinal 構造体に基本情報を適用する
	ret = TWESTG_u32SetBaseInfoToFinal(&stg.sFinal, APPID, APPVER, STGS_SET_VER, STGS_SET_VER_COMPAT);
	// tsFinal 構造体に kind, slot より、デフォルト設定リストを構築する
	ret = TWESTG_u32SetSettingsToFinal(&stg.sFinal, u8kind, u8slot, _SetList);
	// セーブデータがあればロードする
	ret= TWESTG_u32LoadDataFrAppstrg(&stg.sFinal, u8kind, u8slot, APPID, STGS_SET_VER_COMPAT, bNoLoad ? TWESTG_LOAD_OPT_NOLOAD : 0);

	// 設定データをアプリケーション内のデータ領域に反映
	vQueryAppData();
}

bool TWE::TweStgsMenu::begin(uint8_t slot) {
	if (_SetList == nullptr) {
		_SetList = get_setlist();
	}

	// data load
	if (_SetList != nullptr) {
		vAppLoadData(_u8AppKind, slot == 0xff ? _u8AppSlot : slot, FALSE);
		return true;
	}
	return false;
}

void TWE::TweStgsMenu::begin(const TWESTG_tsSettingsListItem *setList, uint8_t slot) {
	// register setting tables
	_SetList = setList;

	// data load
	vAppLoadData(_u8AppKind, slot == 0xff ? _u8AppSlot : slot, FALSE);
}

void TWE::TweStgsMenu::begin(
		  TWETERM::ITerm& trm
		, TWETERM::ITerm& trm_title
		, TWE::IStreamIn& inp
		, const TWEINTRCT_tsFuncs* tblFuncs
		, const TWESTG_tsSettingsListItem *setList
		, uint8_t default_screen
		, uint8_t slot
		) {
	
	if (setList == nullptr) {
		 _SetList = get_setlist();
	}
	begin(_SetList, slot);
	
	// create TWE_tsFile structure
	C_TWE_printf_support::s_init(&_file, &trm, &inp);
	C_TWE_printf_support::s_init(&_file_title, &trm_title, nullptr);
	
	if (tblFuncs == nullptr) {
		_tblFuncs = get_menulist();
	} else {
		_tblFuncs = tblFuncs;
	}
	TWEINTRCT_tsContext* psIntr = TWEINTRCT_pscInit(
		&stg.sFinal, nullptr, &_file, (void*)TWE::TweStgsMenu::_vProcessInputByte, tblFuncs);

	psIntr->config.u8Mode = 1; // init mode is the interactive.
	psIntr->config.u8AlwaysKeyReport = 1; // always has a key call back
	psIntr->config.u8OptSerCmd = 0x01; // echo back seeting
	psIntr->config.u8NoSysReset = 1; // no reset required to take effect
	psIntr->config.u8screen_default = default_screen; // opening screen
	psIntr->config.u8DefMenusSlots = 1; // list slot 1..MAX
	psIntr->pvWrapObjCpp = (void*)this; // store this object pointer
	psIntr->pStreamTitle = &_file_title; // title bar
	psIntr->u16HoldUpdateScreen = 0; // refresh count (set 1 or above)
	TWEINTRCT_vReConf(psIntr); // activate the settings
}

void TWE::TweStgsMenu::_vProcessInputByte(TWEINTRCT_tsContext *_psIntr, int16 key) {
	TWE::TweStgsMenu::key_unhandled.push(key);
}

TWECUI::KeyInput TWE::TweStgsMenu::key_unhandled;

