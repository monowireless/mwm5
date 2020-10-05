#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <mwm5.h>
#include <M5Stack.h>
#include "common.h"

class App_Glancer : public TWE::APP_DEF {
public:
	static const int APP_ID = int(E_APP_ID::GLANCE);
	static const wchar_t LAUNCH_MSG[];

	int get_APP_ID() { return APP_ID; }
	const wchar_t* get_APP_INIT_MSG() { return LAUNCH_MSG; }

	static const int _SORT_KEYS_COUNT = 5;

public:
	typedef TWEUTILS::SimpleBuffer<spTwePacket> pkt_ary;
	/**
	 * @struct	pkt_data
	 *
	 * @brief	PAL data management by ID and display them.
	 */
	struct pkt_data_and_view {
		pkt_ary _dat;
		FixedQueue<spTwePacket> _dat_solo;

		int _page;      // start from 0
		int _lines;     // lines to display entry
		int _max_entry; // max ID (1...max)
		bool _bwide;    // if screen col >= 38, set true
		bool _bdirty;   // if update all is required

		int _nsel;		// selected entry (1..max_entry)

		bool _bsolo;	// solo display mode

		int _sort_key;  // 0:SID, 1:LID, 2:LQI, 3:VOLT 4:TIME

		struct {
			uint32_t src_addr;
			uint32_t n_packets;

			void init() {
				src_addr = 0;
				n_packets = 0;
			}
		} _solo_info;

		ITerm& _trm;           // screen for main content
		ITerm& _trm_status;    // status screen
		const char* _fmt_status; // status message template

		pkt_data_and_view(ITerm& trm, ITerm& trm_status) 
			: _dat(8) // initial reserve
			, _dat_solo(32)
			, _page(0)
			, _lines(0)
			, _max_entry(0)
			, _bwide(false), _bdirty(false)
			, _trm(trm)
			, _trm_status(trm_status)
			, _fmt_status(nullptr)
			, _bsolo(false)
			, _nsel(0)
			, _solo_info{}
			, _sort_key(0)
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

			if (_nsel < _page * _lines || _nsel >= _page *_lines + _lines) {
				_nsel = _page * _lines;
			}
		}

		// show prev page
		void prev_page() {
			_page--;

			if (_page < 0) {
				_page = (_max_entry - 1) / _lines;
			}

			if (_nsel < _page * _lines || _nsel >= _page *_lines + _lines) {
				_nsel = _page * _lines;
			}
		}

		// set page
		void set_page(int entry) {
			if (entry > 0 && entry <= _max_entry) {
				_page = (entry - 1) / _lines;
			}
		}

		void next_item() {
			++_nsel;

			if (_nsel > _max_entry) {
				_nsel = 1;
			}

			if (_nsel <= _page * _lines || _nsel >= _page *_lines + _lines) {
				set_page(_nsel);
			}
		}

		void prev_item() {
			--_nsel;

			if (_nsel < 1) {
				_nsel = _max_entry;
			}

			if (_nsel <= _page * _lines || _nsel >= _page *_lines + _lines) {
				set_page(_nsel);
			}
		}

		// update status string
		void update_status();

		// redraw whole screen
		void update_term() {
			update_term(spTwePacketPal(), true);
		}

		// update screen
		void update_term(spTwePacket pal_upd, bool update_all) {
			if (_bsolo) update_term_solo(pal_upd, update_all);
			else update_term_full(pal_upd, update_all);
		}
		void update_term_full(spTwePacket pal_upd, bool update_all);
		void update_term_solo(spTwePacket pal_upd, bool update_all);
				
		// sort the lists
		void sort_entries();

		// solo mode
		void enter_solo_mode();

		// print obj line
		void print_obj(spTwePacket &spobj);
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

	// packet data management and display
	pkt_data_and_view pkt_data;

	// timeout to hold screen_b
	TWESYS::TimeOut _timeout_hold_screen_b;
	bool _b_hold_screen_b;

public:
	App_Glancer()
		: parse_ascii(256)
		, the_screen(64, 20, { 0, 18, 320, 240 - 30 - 18 }, M5)
		, the_screen_t(64, 1, { 0, 0, 320, 18 }, M5)
		, the_screen_b(64, 4, { 0, 18 + 192, 320, 20 }, M5)
		, the_screen_c(64, 1, { 0, 18 + 192 + 20, 320, 10 }, M5)
		, default_bg_color(0)
		, default_fg_color(0)
		, pkt_data(the_screen, the_screen_t)
		, _timeout_hold_screen_b()
		, _b_hold_screen_b(false)
	{
		set_appobj((void*)static_cast<ITerm*>(&the_screen)); // store app specific obj into APPDEF class storage.
	}

	~App_Glancer() {}

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

	// set navigation bar on the bottom
	void set_nav_bar();
};

