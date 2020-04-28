/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_PAL.hpp"

const wchar_t App_PAL::LAUNCH_MSG[] =
//....+....1....+....2....+....3....+....4| // 16dots 40cols
L"             \033[4m"
             L"ＰＡＬビューア\033[0m"
		                   L"             ""\r\n"
L"\r\n"
L"センサPALから受信したデータを、設定した ""\r\n"
L"子機ID順に表示します。STAGE側のTWELITEに""\r\n"
L"はPAL親機アプリを書換えておきます。"     "\r\n";

void App_PAL::setup() {
	// preference
	the_settings_menu.begin(appid_to_slotid(APP_ID));
	
	// init the TWE M5 support
	setup_screen(); // initialize TWE M5 support.

	// put a init message
	const char* fmt_title = "\033[G\033[1m\033[1mTWELITE\033[0m®PALﾋﾞｭｰｱ\033[0m : %s";
	the_screen_t << printfmt(fmt_title, "---"); // accepts UTF-8 codes
	pkt_data.init_screen(fmt_title);

	// button navigation
	//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
	the_screen_c << "  前ﾍﾟｰｼﾞ/長押:MENU        ﾌｫﾝﾄ/ﾃｽﾄdat       次ﾍﾟｰｼﾞ/ﾘｾｯﾄ";
}

void App_PAL::loop() {
	// KEYBOARD
	while (the_keyboard.available()) {
		switch (int c = the_keyboard.read()) {
		case KeyInput::KEY_ESC:
			the_app.exit(App_PAL::APP_ID);
			return;

		case KeyInput::KEY_BUTTON_A:
			// previous page
			pkt_data.prev_page();
			pkt_data.update_term();
			break;

		case KeyInput::KEY_BUTTON_A_LONG:
			// back to menu
			the_app.exit(App_PAL::APP_ID);
			break;

		case KeyInput::KEY_BUTTON_B:
			// chenge font
			change_screen_font();

			pkt_data.reinit_screen();
			pkt_data.update_term();
			break;

		case KeyInput::KEY_BUTTON_B_LONG:
			{	// test data
				const char* paldata[] = {
					":800000008D0011810EE29A01808103113008020CE411300102048A00000001006163\r\n",
					":800000006F0010810EE29A01808103113008020CE411300102052C0000000101A59A\r\n",
					":80000000D50079810EE29A07808312113008020CE41130010203CC15040006FFF0FF2003F015040106FFE8FF28040015040206FFE0FF3003F815040306FFF0FF2003F015040406FFE8FF2803F015040506FFE0FF2803F815040606FFE8FF20040015040706FFE0FF20041015040806FFE8FF28040815040906FFF0FF28040015040A06FFF0FF4003F815040B06FFE8FF20040815040C06FFF8FF2003F015040D06FFE0FF28042015040E06FFE0FF2003F815040F06FFE8FF2004103D28\r\n" };
				const char* msgs[] = {
					"ID=1 MAG OPEN",
					"ID=1 MAG CLOSE",
					"ID=7 MOT" };

				static int s_idx = 0;

				s_idx++;
				if (s_idx >= 3) s_idx = 0;

				pkt_data.set_page(1);

				const char* p = paldata[s_idx];
				while (*p != 0) {
					parse_a_byte(char_t(*p));
					p++;
				}

				pkt_data.update_term();

				the_screen_b.clear_screen();
				the_screen_b << "TEST DATA: " << msgs[s_idx];
			}
			break;

		case KeyInput::KEY_BUTTON_C:
			// next page
			pkt_data.next_page();
			pkt_data.update_term();
			break;

		case KeyInput::KEY_BUTTON_C_LONG:
			twe_prog.reset_module();
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

void App_PAL::setup_screen() {
	// LCD font color
	default_bg_color = color565(sAppData.u32_TWESTG_STAGE_BG_COLOR); // color565(90, 0, 50); 
	default_fg_color = color565(sAppData.u32_TWESTG_STAGE_FG_COLOR);

	// font register (note: to save flash area, don't create too much!)
	TWEFONT::createFontMP10(1, 0, 0); // MP10 font

	TWEFONT::createFontShinonome16(10, 0, 0, TWEFONT::U32_OPT_FONT_TATEBAI); // shinonome 16 font (TATE BAIKAKU)
	TWEFONT::createFontShinonome16(11, 1, 0); // shinonome 16 font
	TWEFONT::createFontMP12(12, 0, 0, TWEFONT::U32_OPT_FONT_TATEBAI | TWEFONT::U32_OPT_FONT_YOKOBAI); // MP10 font
	TWEFONT::createFontMP12(13, 0, 0); // MP10 font

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
void App_PAL::screen_refresh() {
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
void App_PAL::change_screen_font() {
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
void App_PAL::parse_a_byte(char_t u8b) {
	// parse.
	parse_ascii << u8b;

	// if complete parsing
	if (parse_ascii) {
		// output as parser format
		// the_screen_b << parse_ascii;
		the_screen_b.clear_screen();
		static int ct;
		the_screen_b << "PKT(" << ct++ << ')';

		// EEPROM TEST
		auto&& p = parse_ascii.get_payload();

		// 1. identify the packet type
		auto&& pkt = newTwePacket(parse_ascii.get_payload());
		the_screen_b << ":Typ=" << int(identify_packet_type(pkt));

		if (identify_packet_type(pkt) == E_PKT::PKT_PAL) {
			auto&& pal = refTwePacketPal(pkt);

			// put information
			the_screen_b
				<< printfmt(":Lq=%d:Ad=%08X", pal.u8lqi, pal.u32addr_src)
				<< ":PAL=" << int(pal.u8palpcb)
				<< ":ID=" << int(pal.u8addr_src)
				<< ":Dat=" << int(pal.u8sensors)
				;

			// store data into `pal_data'
			if (pkt_data.add_entry(pkt)) {
				// update screen.
				pkt_data.update_term(pkt, false);
			}
		}
	}
}

// process input
void App_PAL::process_input() {
	int c;

	// from TWE
	while (-1 != (c = the_uart_queue.read())) {
		// pass them to M5 (normal packet analysis)
		parse_a_byte(char_t(c));
	}
}

// check serial input.
void App_PAL::check_for_serial() {
	// UART CHECK is DONE at ::loop() procedure.
	;
}

/**
 * @fn	void update_term(spTwePacketPal pal_upd, bool update_all)
 *
 * @brief	Updates the term
 *
 * @param	pal_upd   	The pal data to be updated.
 * @param	update_all	True to update all lines, other than pal_upd.
 */
void App_PAL::pkt_data_and_view::update_term(spTwePacket pal_upd, bool update_all) {
	int idx_start = _page * _lines + 1;
	int idx_end = idx_start + _lines;

	// check the index
	int idx = -1;
	if (pal_upd) {
		switch (identify_packet_type(pal_upd)) {
		case E_PKT::PKT_PAL: idx = refTwePacketPal(pal_upd).u8addr_src; break;
		default: break;
		}
	}

	// when max entry count has been changed, this flag is set.
	if (_bdirty) {
		update_all = true;
		_bdirty = false;
	}

	if (update_all) {
		// update all
		_trm << "\033[2J\033[H"; // clear screen (by ESC seq)
	}
	else {
		if (!(idx >= idx_start && idx < idx_end)) {
			return; // out of scope (not update)
		}
	}

	// displays each sensor data.
	for (int i = idx_start; i < idx_end && i <= _siz; i++) {
		auto&& spobj = _pal[i - 1];

		// show IDs at head.
		if (update_all || (spobj && spobj == pal_upd)) {
			_trm << printfmt("\033[%d;1H\033[K", i - idx_start + 1) // move cursor and clear the line
				<< printfmt("%2d:", i); // ID:
		}

		// skip when the corresponding ID is not updated.
		if (!spobj || ((update_all == false) && spobj != pal_upd)) continue; // not updated

		// display sensor data
		if (identify_packet_type(spobj) == E_PKT::PKT_PAL) {
			auto&& pal = refTwePacketPal(spobj);

			switch (pal.u8palpcb) {
			case E_PAL_PCB::MAG:
			{
				// generate pal board specific data structure.
				PalMag mag = pal.get_PalMag();

				_trm << TermAttr(TERM_COLOR_BG_BLACK | TERM_COLOR_FG_CYAN);
				_trm << "開閉";
				_trm << TermAttr(TERM_ATTR_OFF);

				_trm << ':';

				if (mag.u8MagStat == 0) {
					_trm << TermAttr(TERM_COLOR_FG_RED | TERM_BOLD);
					_trm << "OPEN(開)";
					_trm << TermAttr(TERM_ATTR_OFF);
				}
				else {
					_trm << "CLOSE(閉)";
				}
			} break;

			case E_PAL_PCB::AMB:
			{
				// generate pal board specific data structure.
				PalAmb amb = pal.get_PalAmb();

				_trm << TermAttr(TERM_COLOR_BG_BLACK | TERM_COLOR_FG_GREEN);
				_trm << printfmt("環境", pal.u8addr_src);
				_trm << TermAttr(TERM_ATTR_OFF);
				_trm << ":";

				_trm << TermAttr(TERM_COLOR_FG_RED | TERM_BOLD);
				_trm << printfmt(_bwide ? "温度=%02.1f℃" : "%02.1fC", (double)amb.i16Temp / 100.0);
				_trm << TermAttr(TERM_ATTR_OFF);

				_trm << ' ';

				_trm << TermAttr(TERM_COLOR_FG_BLUE | TERM_BOLD);
				_trm << printfmt(_bwide ? "湿度=%02d%%" : "%02d%%", (amb.u16Humd + 50) / 100);
				_trm << TermAttr(TERM_ATTR_OFF);

				_trm << ' ';

				_trm << TermAttr(TERM_COLOR_FG_YELLOW | TERM_BOLD);
				_trm << printfmt(_bwide ? "照度=%4d" : "L%4d", amb.u32Lumi > 9999 ? 9999 : amb.u32Lumi);
				_trm << TermAttr(TERM_ATTR_OFF);
			} break;

			case E_PAL_PCB::MOT:
			{
				// generate pal board specific data structure.
				PalMot mot = pal.get_PalMot();

				_trm << TermAttr(TERM_COLOR_BG_BLACK | TERM_COLOR_FG_GREEN);
				_trm << printfmt("加速", pal.u8addr_src);
				_trm << TermAttr(TERM_ATTR_OFF);
				_trm << ":";

				if (mot.u8samples > 0) {
					_trm << printfmt(_bwide ? "X=%5d Y=%5d Z=%5d" : "%5d,%5d,%5d", mot.i16X[0], mot.i16Y[0], mot.i16Z[0]);
				}
				else {
					_trm << "n/a.";
				}
			} break;

			default:
				_trm << "n.a";
			}
		}

		_trm << "\033[4G"; // move cursor at column 2.
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
void App_PAL::pkt_data_and_view::update_status() {
	int max_page = (_max_entry - 1) / _lines;
	char buff[16];

	snPrintf(buff, sizeof(buff), "[%d/%d]", _page + 1, max_page + 1);

	_trm_status.clear_screen();
	_trm_status << printfmt(_fmt_status, buff);
}

/**
 * @fn	void reinit_screen()
 *
 * @brief	Reinitializes the screen, in case the screen size has been changed.
 *
 */
void App_PAL::pkt_data_and_view::reinit_screen() {
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
bool App_PAL::pkt_data_and_view::add_entry(spTwePacket spobj) {
	if (identify_packet_type(spobj) == E_PKT::PKT_PAL) {
		auto&& pal = refTwePacketPal(spobj);

		if (pal.u8addr_src >= 1 && pal.u8addr_src <= _siz) {
			_pal[pal.u8addr_src - 1] = spobj;
			if (_max_entry < pal.u8addr_src) {
				_max_entry = pal.u8addr_src;
				_bdirty = true;
			}

			return true;
		}
	}

	return false;
}
