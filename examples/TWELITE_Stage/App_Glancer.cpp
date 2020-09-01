/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_Glancer.hpp"

#include <algorithm>
#ifdef min
# undef min
#endif

const wchar_t App_Glancer::LAUNCH_MSG[] =
//....+....1....+....2....+....3....+....4| // 16dots 40cols
L"            \033[4m"
            L"Glancer グランサー\033[0m"
		                      L"          ""\r\n"
L"\r\n"
L"Glancer グランサーは、アスキー形式を解釈""\r\n"
L"して、送信元IDやﾘﾝｸ品質など表示します。" "\r\n"
L"リストの中から特定の送信元のみを選択表示""\r\n"
L"できます。設置した無線子機の電波の飛び具""\r\n"
L"合いを簡単に調べることができます。"      "\r\n"
L"TWELITEにはApp_Wingsを書き込みます。"    "\r\n"
;

void App_Glancer::setup() {
	// preference
	the_settings_menu.begin(appid_to_slotid(APP_ID));
	
	// init the TWE M5 support
	setup_screen(); // initialize TWE M5 support.

	// put a init message
	const char* fmt_title = "\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m グランサー\033[0m : %s";
	the_screen_t << printfmt(fmt_title, "---"); // accepts UTF-8 codes
	pkt_data.init_screen(fmt_title);

	// button navigation
	set_nav_bar();
}


// button navigation
void App_Glancer::set_nav_bar() {
	the_screen_c.clear_screen();

	if (pkt_data._bsolo) {
		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << "     --/長押:戻る            --/--                --/ﾘｾｯﾄ";
	}
	else {
		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << "     ↑/長押:戻る          決定/ｿｰﾄ               ↓/ﾘｾｯﾄ";
	}
}

void App_Glancer::loop() {
	// reset the hold screen flag of screen_b 
	if (_timeout_hold_screen_b.available()) _b_hold_screen_b = false;

	// KEYBOARD
	while (the_keyboard.available()) {
		switch (int c = the_keyboard.read()) {

		case KeyInput::KEY_BUTTON_A_LONG:
		case KeyInput::KEY_ESC:
			if (pkt_data._bsolo) {
				pkt_data._bsolo = false;
				pkt_data._dat_solo.clear();
				
				pkt_data.update_term();
				set_nav_bar();
			} else {
				the_app.exit(App_Glancer::APP_ID);
				return;
			}
			break;

		case KeyInput::KEY_BUTTON_B:
		case KeyInput::KEY_ENTER:
			if (!pkt_data._bsolo) {
				pkt_data.enter_solo_mode();
				set_nav_bar();
			}
			break;

		case KeyInput::KEY_BUTTON_A:
		case KeyInput::KEY_UP:
			pkt_data.prev_item();
			pkt_data.update_term();
			break;
			
		case KeyInput::KEY_BUTTON_C:
		case KeyInput::KEY_DOWN:
			pkt_data.next_item();
			pkt_data.update_term();
			break;
			
		case KeyInput::KEY_LEFT:
			// previous page
			pkt_data.prev_page();
			pkt_data.update_term();
			break;

#if 0
		case KeyInput::KEY_BUTTON_B:
			// chenge font
			change_screen_font();

			pkt_data.reinit_screen();
			pkt_data.update_term();
			break;
#endif

		case KeyInput::KEY_BUTTON_B_LONG:
			{
				int sort_key = pkt_data._sort_key;

				const char str_srt_key[_SORT_KEYS_COUNT][32] = {
					"論理ID",
					"シリアル番号",
					"リンク品質 Lq",
					"電圧 mV",
					"時間 s"
				};
				// sort entries
				pkt_data.sort_entries();
				pkt_data.update_term();

				the_screen_b.clear_screen();
				the_screen_b << printfmt("\033[30;33m[%s]でソートしました\033[0m", str_srt_key[sort_key]);

				// keep this message for 3sec
				_b_hold_screen_b = true;
				_timeout_hold_screen_b.start(3000);
			}
			break;

		case KeyInput::KEY_RIGHT:
			// next page
			pkt_data.next_page();
			pkt_data.update_term();
			break;

		case KeyInput::KEY_BUTTON_C_LONG:
			twe_prog.reset_module();
			break;

		default:
			// put input data to uart queue (for debugging)
			if (c >= 0x20 && c <= 0x7E) {
				the_uart_queue.push(uint8_t(c));
			}
			break;
		}
	}

	// check for serial input
	check_for_serial();

	// process input data
	process_input();

	// LCD update
	screen_refresh();
}

void App_Glancer::setup_screen() {
	// LCD font color
	default_bg_color = color565(sAppData.u32_TWESTG_STAGE_BG_COLOR); // color565(90, 0, 50); 
	default_fg_color = color565(sAppData.u32_TWESTG_STAGE_FG_COLOR);

	// font register (note: to save flash area, don't create too much!)
	TWEFONT::createFontMP10_std(1, 0, 0); // MP10 font

	TWEFONT::createFontShinonome16_mini(10, 0, 0); // shinonome 16 font
	TWEFONT::createFontShinonome16_mini(11, 1, 0); // shinonome 16 font
	TWEFONT::createFontMP12_mini(12, 0, 0, TWEFONT::U32_OPT_FONT_TATEBAI | TWEFONT::U32_OPT_FONT_YOKOBAI); // MP10 font
	TWEFONT::createFontMP12_mini(13, 0, 0); // MP10 font

	// main screen area
	the_screen.set_font(10);
	the_screen.set_color(default_fg_color, default_bg_color);
	the_screen.set_cursor(2); // 0: no 1: curosr 2: blink cursor
	the_screen.force_refresh();

	// bottom area
	the_screen_b.set_font(1);
	the_screen_b.set_color(color565(80, 80, 80), color565(20, 20, 20));
	the_screen_b.set_cursor(0);
	the_screen_b.force_refresh();

	// bottom area
	the_screen_c.set_font(1);
	the_screen_c.set_color(ORANGE, color565(20, 20, 20));
	the_screen_c.set_cursor(0);
	the_screen_c.force_refresh();

	// top area
	the_screen_t.set_font(11);
	the_screen_t.set_color(default_bg_color, default_fg_color);
	the_screen_t.set_cursor(0);
	the_screen_t.force_refresh();
}

// screen refresh timing (every 32ms)
void App_Glancer::screen_refresh() {
	static uint32_t u32mills;

	uint32_t u32now = millis();
	if (u32now - u32mills > 32) {
		the_screen.refresh();
		the_screen_b.refresh();
		the_screen_c.refresh();
		the_screen_t.refresh();

		u32mills = u32now;
	}
}

// change screen font
void App_Glancer::change_screen_font() {
	static uint8_t idx = 1;
	uint8_t modes[] = { 10, 11, 12, 13 }; // toggle screen 10 .. 13

	the_screen.set_font(modes[idx & 0x3]);
	the_screen.clear_screen();
	the_screen.force_refresh();

	auto font = TWEFONT::queryFont(the_screen.font_id());
	the_screen_b.clear_screen();
	TWE::fPrintf(the_screen_b, "\nFont: %s\n      ID=%d H:%d W:%d W_CHRs:%d",
		font.font_name, font.get_font_code(), font.height, font.width, font.font_wide_count);

	idx++;
}

// put a byte to parse
void App_Glancer::parse_a_byte(char_t u8b) {
	// parse.
	parse_ascii << u8b;

	// if complete parsing
	if (parse_ascii) {
		// output as parser format
		// the_screen_b << parse_ascii;
		if (!_b_hold_screen_b) the_screen_b.clear_screen();
		static int ct;
		if (!_b_hold_screen_b) the_screen_b << "PKT(" << ct++ << ')';

		// EEPROM TEST
		auto&& p = parse_ascii.get_payload();

		// 1. identify the packet type
		auto&& pkt = newTwePacket(parse_ascii.get_payload());
		if (!_b_hold_screen_b) the_screen_b << ":Typ=" << int(identify_packet_type(pkt));

		if (identify_packet_type(pkt) != E_PKT::PKT_ERROR) {
			// put information
			if (!_b_hold_screen_b) the_screen_b
					<< printfmt(":Lq=%d:Ad=%08X(%02X),Tms=%d"
						, pkt->common.lqi, pkt->common.src_addr, pkt->common.src_lid, pkt->common.tick);

			// store data into `pal_data'
			if (pkt_data.add_entry(pkt)) {
				// update screen.
				pkt_data.update_term(pkt, false);
			}
		}
	}
}

// process input
void App_Glancer::process_input() {
	int c;

	// from TWE
	while (-1 != (c = the_uart_queue.read())) {
		// pass them to M5 (normal packet analysis)
		parse_a_byte(char_t(c));
	}
}

// check serial input.
void App_Glancer::check_for_serial() {
	// UART CHECK is DONE at ::loop() procedure.
	;
}

// Packet name table
typedef struct {
	E_PKT id;
	const wchar_t name[4];
} ts_PktID_to_Name3;

const static ts_PktID_to_Name3 asPktIdToName3[] {
	{ E_PKT::PKT_PAL,   	L"PAL" },
	{ E_PKT::PKT_TWELITE,	L"TWE" },
	{ E_PKT::PKT_APPIO, 	L"IO " },
	{ E_PKT::PKT_APPUART,	L"URT" },
	{ E_PKT::PKT_APPTAG,	L"TAG" },
	{ E_PKT::PKT_ERROR,		L"N/A" },
};

/**
 * @fn	void update_term_full(spTwePacketPal pal_upd, bool update_all)
 *
 * @brief	Updates the term full list
 *
 * @param	pal_upd   	Coming packet (should be updated in the display)
 * @param	update_all	True to update all lines, other than pal_upd.
 */
void App_Glancer::pkt_data_and_view::update_term_full(spTwePacket pal_upd, bool update_all) {
	int idx_start = _page * _lines + 1;
	int idx_end = idx_start + _lines;

	// when max entry count has been changed, this flag is set.
	if (_bdirty) {
		update_all = true;
		_bdirty = false;
	}

	if (update_all) {
		// update all
		_trm << "\033[2J\033[H"; // clear screen (by ESC seq)
	}

	// displays each sensor data.
	for (int i = idx_start; i < idx_end && i <= int(_dat.size()); i++) {
		auto&& spobj = _dat[i - 1];

		// show IDs at head.
		if (update_all || (spobj && spobj == pal_upd)) {
			_trm << printfmt("\033[%d;1H\033[K", i - idx_start + 1) // move cursor and clear the line
				<< printfmt("%2d:", i); // ID:
		}

		// skip when the corresponding ID is not updated.
		if (!spobj || ((update_all == false) && spobj != pal_upd)) continue; // not updated

		// display sensor data
		if (identify_packet_type(spobj) != E_PKT::PKT_ERROR) {
			if (_nsel == i) _trm << "\033[7m";
			print_obj(spobj);
			if (_nsel == i) _trm << "\033[0m";
		}
		else {
			_trm << "---";
		}

		_trm << "\033[4G"; // move cursor at column 2.
	}

	// update status line
	update_status();
}

void App_Glancer::pkt_data_and_view::print_obj(spTwePacket& spobj) {
	// display sensor data
	if (identify_packet_type(spobj) != E_PKT::PKT_ERROR) {
		auto&& pal = refTwePacketPal(spobj);

		const wchar_t* pAppName = nullptr;
		for (auto& x : asPktIdToName3) {
			if (x.id == spobj->get_type()) {
				pAppName = x.name;
				break;
			}
		}
		if (pAppName == nullptr) pAppName = L"N/A";
		else _trm << pAppName;

		_trm << printfmt("%3d/x%08X %3dLq %4dmV"
			, spobj->common.src_lid
			, spobj->common.src_addr
			, spobj->common.lqi
			, spobj->common.volt
		);

		uint32_t t = spobj->common.tick + 50;
		t = t % (10000 * 1000); // loop at 9999.9sec

		_trm << printfmt(" %4d.%ds", t / 1000, (t % 100) / 10);
	}
}

void App_Glancer::pkt_data_and_view::update_term_solo(spTwePacket pal_upd, bool update_all) {
	update_all = true; // always update everything

	if (update_all) {
		// update all
		_trm << "\033[2J\033[H"; // clear screen (by ESC seq)
	}

	// displays each sensor data.
	if (_dat_solo.size() > 0) {
		if (update_all || (pal_upd && pal_upd->common.src_addr == _solo_info.src_addr)) {
			// move cursor at home position
			_trm << "\033[H";

			int i_end = std::min(int(_dat_solo.size()), int(_lines - 1));
			for (int i = i_end; i > 0; i--) {
				auto& spobj = _dat_solo[-i];

				print_obj(spobj);

				_trm << crlf;
			}

			// check lqi average
			int lqav = 0;
			for (int i = 0; i < _dat_solo.size(); i++) lqav += _dat_solo[i]->common.lqi;
			lqav /= _dat_solo.size();

			_trm << printfmt("\033[%d;1H", _trm.get_rows());
			_trm << "\033[7m\033[K\033[G"; // clear the line
			if(_bwide) _trm << printfmt("Total=%dpkt LQav=%d(of%d)", _solo_info.n_packets, lqav, _dat_solo.size());
			else       _trm << printfmt("%dpkt LQav=%d", _solo_info.n_packets, lqav);
			_trm << printfmt("\033[0m\033[%d;1H", i_end); // move the cursor at the latest item.
		}
	}

	// update status line
	update_status();
}


/**
 * @fn	void update_status()
 *
 * @brief	Updates the status string.
 *
 */
void App_Glancer::pkt_data_and_view::update_status() {
	char buff[16];

	if (_bsolo) {
		_trm_status.clear_screen();
		_trm_status << printfmt(_fmt_status, "---");
	} else {
		int max_page = (_max_entry - 1) / _lines;
		snPrintf(buff, sizeof(buff), "[%d/%d]", _page + 1, max_page + 1);

		_trm_status.clear_screen();
		_trm_status << printfmt(_fmt_status, buff);
	}
}

/**
 * @fn	void reinit_screen()
 *
 * @brief	Reinitializes the screen, in case the screen size has been changed.
 *
 */
void App_Glancer::pkt_data_and_view::reinit_screen() {
	int scrw = _trm.get_cols();
	int scrh = _trm.get_rows();

	_page = 0;
	_lines = scrh;
	_bwide = (scrw >= 38);

	_bdirty = true;

	_trm << "\033[2J\033[H"; // clear screen (by ESC seq)
	update_status();
}

/**
 * @fn	bool add_entry(spTwePacketPal pal)
 *
 * @brief	Adds an entry of PAL packet.
 *
 * @param	pal PAL object to be added.
 *
 * @returns	True if it succeeds, false if it fails.
 */
bool App_Glancer::pkt_data_and_view::add_entry(spTwePacket spobj) {
	if (spobj
		&& identify_packet_type(spobj) != E_PKT::PKT_ERROR
		&& spobj->common.src_addr & 0x80000000
	) {
		bool b_found = false;
		for (int i = 0; i < _max_entry; i++) {
			if (spobj->common.src_addr == _dat[i]->common.src_addr) {
				b_found = true;
				_dat[i] = spobj;
				break;
			}
		}

		if (!b_found) {
			_dat.push_back(spobj);
			_max_entry = _dat.size();

			b_found = true;
		}

		if (b_found) {
			if (_bsolo) {
				// if solo mode, check the coming packt's id with monitoring id and push it.
				if (!_dat_solo.empty() && _dat_solo.front()->common.src_addr == spobj->common.src_addr) {
					if (_dat_solo.is_full()) _dat_solo.pop();
					_solo_info.n_packets++;
					_dat_solo.push(spobj);

					return true;
				}

				return false;
			}

			return true;
		}
	}

	return false;
}

void App_Glancer::pkt_data_and_view::sort_entries() {
	switch (_sort_key) {
	case 0: // LID
		SmplBuf_Sort(_dat,
			[](spTwePacket& a, spTwePacket& b) {
				return a->common.src_lid > b->common.src_lid;
			}
		);
		break;

	case 1: // SID
		SmplBuf_Sort(_dat,
			[](spTwePacket& a, spTwePacket& b) {
				return a->common.src_addr > b->common.src_addr;
			}
		);
		break;

	case 2: // LQI
		SmplBuf_Sort(_dat,
			[](spTwePacket& a, spTwePacket& b) {
				return a->common.lqi < b->common.lqi; // bigger first
			}
		);
		break;

	case 3: // Volt 
		SmplBuf_Sort(_dat,
			[](spTwePacket& a, spTwePacket& b) {
				return a->common.volt > b->common.volt;
			}
		);
		break;

	case 4: // Time
		SmplBuf_Sort(_dat,
			[](spTwePacket& a, spTwePacket& b) {
				return a->common.tick < b->common.tick; // bigger first
			}
		);
		break;
	}

	if (++_sort_key >= _SORT_KEYS_COUNT) _sort_key = 0;
}

void App_Glancer::pkt_data_and_view::enter_solo_mode() {
	if (_nsel > 0 && _nsel <= int(_dat.size())) {
		
		auto obj = _dat[_nsel - 1];

		if (obj) {
			_bsolo = true;
			_dat_solo.clear();
			_solo_info.init();

			_dat_solo.push(obj);

			_solo_info.src_addr = obj->common.src_addr;
			_solo_info.n_packets = 1;

			update_term();
		}
	}
}
