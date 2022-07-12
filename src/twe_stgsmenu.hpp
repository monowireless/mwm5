#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_console.hpp"

#include "twe_cui_keyboard.hpp"

#include "twesettings/twecommon.h"
#include "twesettings/tweinteractive.h"

#define STGS_SET_VER 0x01         //! 設定バージョン
#define STGS_SET_VER_COMPAT 0x01  //! 互換性のある設定バージョン

namespace TWE {
	class TweStgsMenu {
	protected:
		uint8 u8buf[256];
		TWE_tsFILE _file, _file_title;

		uint8 _u8AppKind; //! Selected Kind.
		uint8 _u8AppSlot; //! Selected Slot.

		uint8_t _MAX_SLOT_NUMBER;

		const TWESTG_tsSettingsListItem *_SetList;
		const TWEINTRCT_tsFuncs* _tblFuncs;

	struct _stg {
		std::unique_ptr<TWESTG_tsDatum[]> sData;
		std::unique_ptr<TWESTG_tuDatum[]> uData;
		std::unique_ptr<TWESTG_tsElement*[]> apEle;
		std::unique_ptr<uint8[]> u8ShortcutKeys;
		
		std::unique_ptr<uint8[]>u8StrPool;
		TWE_tsBuffer sBufStrPool;

		TWESTG_tsFinal sFinal;

		uint8_t _COUNT;
		uint8_t _CUSTDEF;
		uint8_t _STRBUF;

		_stg(uint8_t COUNT, uint8_t STRBUF, uint8_t CUSTDEF) : _COUNT(COUNT), _STRBUF(STRBUF), _CUSTDEF(CUSTDEF) {
			sData.reset(new TWESTG_tsDatum[COUNT]);
			uData.reset(new TWESTG_tuDatum[CUSTDEF]);
			apEle.reset(new TWESTG_tsElement*[COUNT]);
			u8ShortcutKeys.reset(new uint8[COUNT]);
			u8StrPool.reset(new uint8[STRBUF]);
			TWE_vInit_tsBuffer(&sBufStrPool, u8StrPool.get(), 0, STRBUF);

			init();
		}

		void init() {
			TWESTG_vInit_tsFinal(
				  &sFinal
				, NULL
				, sData.get()
				, &sBufStrPool
				, apEle.get()
				, u8ShortcutKeys.get(), _COUNT
				, uData.get(), _CUSTDEF);
		}

		~_stg() {
		}

	} stg;

	public:
		TweStgsMenu(uint8_t COUNT = 32, uint8_t STRBUF = 64, uint8_t CUSTDEF = 4)
			: u8buf()
			, _file(), _file_title()
			, stg(COUNT,STRBUF,CUSTDEF)
			, _u8AppKind(0)
			, _u8AppSlot(0)
			, _MAX_SLOT_NUMBER(1)
			, _SetList(nullptr)
			, _tblFuncs(nullptr)
			{}

		~TweStgsMenu() {}

		// load settings data (with slot specifying)
		bool begin(uint8_t slot = 0xff);

		// load settings data (with slot specifying)
		void begin(const TWESTG_tsSettingsListItem *setList, uint8_t slot);

		// start menu screen
		void begin(
				  TWETERM::ITerm& trm
				, TWETERM::ITerm& trm_title
				, TWE::IStreamIn& inp
				, const TWEINTRCT_tsFuncs* tblFuncs // if set nullptr, get_setlist() is used.
				, const TWESTG_tsSettingsListItem *setList // if set nullptr, get_menulist() is used.
				, uint8_t default_screen = 0
				, uint8_t slot = 0xff
				);

		inline void update() {
			TWEINTRCT_vHandleSerialInput();
		}

	public:
		void vAppLoadData(uint8 u8kind, uint8 u8slot, bool_t bNoLoad);

		virtual void init() = 0;
		virtual void vQueryAppData() = 0;

		uint8& get_kind() { return _u8AppKind; }
		uint8& get_slot() { return _u8AppSlot; }

		// set max slot number
		void set_max_slotnumber(uint8_t slot) { _MAX_SLOT_NUMBER = slot; }
		uint8_t get_max_slotnumber() { return _MAX_SLOT_NUMBER; }

		virtual const char *get_slot_name(uint8 slot) { return nullptr; }

		virtual const TWESTG_tsSettingsListItem *get_setlist() {  return _SetList; }
		virtual const TWEINTRCT_tsFuncs *get_menulist() { return _tblFuncs; }

	public:
		static void _vProcessInputByte(TWEINTRCT_tsContext *, int16);
		static TWECUI::KeyInput key_unhandled;
	};

	extern TweStgsMenu& the_settings_menu;
}

