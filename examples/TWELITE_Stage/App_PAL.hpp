#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <mwm5.h>
#include <M5Stack.h>
#include "common.h"



class App_PAL : public TWE::APP_DEF {
public:
	static const int APP_ID = int(E_APP_ID::PAL);
	static const wchar_t LAUNCH_MSG[];

public:
	/**
	 * @struct	pkt_data
	 *
	 * @brief	PAL data management by ID and display them.
	 */
	struct pkt_data_and_view {
		static const int _siz = 32; // entry max
		spTwePacket _pal[_siz]; // store Packet Data

		int _page;      // start from 0
		int _lines;     // lines to display entry
		int _max_entry; // max ID (1...max)
		bool _bwide;    // if screen col >= 38, set true
		bool _bdirty;   // if update all is required

		ITerm& _trm;           // screen for main content
		ITerm& _trm_status;    // status screen
		const char* _fmt_status; // status message template

		pkt_data_and_view(ITerm& trm, ITerm& trm_status)
			: _pal{}
			, _page(0)
			, _lines(0)
			, _max_entry(0)
			, _bwide(false), _bdirty(false)
			, _trm(trm)
			, _trm_status(trm_status)
			, _fmt_status(nullptr)
		{}

		// screen init
		void init_screen(const char* fmt_status) {
			_fmt_status = fmt_status;
			reinit_screen();
		}

		// re-init screen
		void reinit_screen();

		// add coming packet entry
		bool add_entry(spTwePacket spobj);

		// show next page
		void next_page() {
			_page++;

			if (_page * _lines + 1 > _max_entry) {
				_page = 0; // switch to the top
			}
		}

		// show prev page
		void prev_page() {
			_page--;

			if (_page < 0) {
				_page = (_max_entry - 1) / _lines;
			}
		}

		// set page
		void set_page(int entry) {
			if (entry > 0 && entry <= _max_entry) {
				_page = (entry - 1) / _lines;
			}
		}

		// update status string
		void update_status();

		// redraw whole screen
		void update_term() {
			update_term(spTwePacketPal(), true);
		}

		// update screen
		void update_term(spTwePacket pal_upd, bool update_all);
	};

private:
	// Serial Parser
	AsciiParser parse_ascii;

	// top bar
	TWETerm_M5_Console the_screen_t; // init the screen.

	// main screen
	TWETerm_M5_Console the_screen; // init the screen.

	// bottom area (for debug information)
	TWETerm_M5_Console the_screen_b; // init the screen.

	// bottom area (for debug information)
	TWETerm_M5_Console the_screen_c; // init the screen.

	// default color
	uint16_t default_bg_color;
	uint16_t default_fg_color;

	pkt_data_and_view pkt_data;

public:
	App_PAL()
		: parse_ascii(256)
		, the_screen(64, 20, { 0, 18, 320, 240 - 30 - 18 }, M5)
		, the_screen_t(64, 1, { 0, 0, 320, 18 }, M5)
		, the_screen_b(64, 4, { 0, 18 + 192, 320, 20 }, M5)
		, the_screen_c(64, 1, { 0, 18 + 192 + 20, 320, 10 }, M5)
		, default_bg_color(0)
		, default_fg_color(0)
		, pkt_data(the_screen, the_screen_t)
	{
		set_appobj((void*)static_cast<ITerm*>(&the_screen)); // store app specific obj into APPDEF class storage.
	}

	~App_PAL() {}

	void setup();

	void loop();

private:
	void parse_a_byte(char_t u8b);
	void process_input();
	void check_for_serial();

	// setup procedure
	void setup_screen();

	// update screen
	void screen_refresh();

	// change screen font of `the_screen'
	void change_screen_font();
};