/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_Twelite.hpp"

const wchar_t App_TweLite::LAUNCH_MSG[] =
//....+....1....+....2....+....3....+....4| // 16dots 40cols
L"    " L"\033[4m"
    L"標準アプリ (0x81ｽﾃｰﾀｽ) ビューア" L"\033[0m"
		                           L"     ""\r\n"
L"\r\n"
L"標準アプリで受信した0x81ｽﾃｰﾀｽを画面に表 ""\r\n"
L"示します。TWELITEには標準アプリを書込ん ""\r\n"
L"でおきます。STAGEボード上のLEDの点灯状態""\r\n"
L"などが画面上で再現されます。";

void App_TweLite::setup() {
	// preference
	the_settings_menu.begin(appid_to_slotid(APP_ID));
	
	// init the TWE M5 support
	setup_screen(); // initialize TWE M5 support.

	// put a init message
	the_screen_t << "\033[G\033[1mTWELITE\033[0m®\033[1m標準アプリ\033[0m (ｺﾏﾝﾄﾞ0x81)";

	// button navigation
	//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
	the_screen_c << "     --/長押:MENU          ﾌｫﾝﾄ/ﾃｽﾄdat            --/ﾘｾｯﾄ";

	// init screen
	update_screen(true);
}

void App_TweLite::loop() {
	// KEYBOARD
	while (the_keyboard.available()) {
		switch (int c = the_keyboard.read()) {
		case KeyInput::KEY_ESC:
			the_app.exit(APP_ID);
			return;

		case KeyInput::KEY_BUTTON_A:
			break;

		case KeyInput::KEY_BUTTON_A_LONG:
			the_app.exit(APP_ID);
			return;

		case KeyInput::KEY_BUTTON_B:
			// chenge font
			change_screen_font();
			update_screen(true);
			break;

		case KeyInput::KEY_BUTTON_B_LONG:
			{
				// test data
				const char* pktdata[] = {
					":7881150175810000380026C9000C04220000FFFFFFFFFFA7\r\n",
					":788115017581000038002785000C05220000FFFFFFFFFFE9\r\n",
					":78811501758100003800284F000C02230000FFFFFFFFFF20\r\n" };
				const char* msgs[] = {
					"DATA1",
					"DATA2",
					"DATA3" };

				static int s_idx = 0;

				s_idx++;
				if (s_idx >= 3) s_idx = 0;

				const char* p = pktdata[s_idx];
				while (*p != 0) {
					parse_a_byte(char_t(*p));
					p++;
				}

				the_screen_b.clear_screen();
				the_screen_b << "TEST DATA: " << msgs[s_idx];
			}
			break;

		case KeyInput::KEY_BUTTON_C:
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


// color table
static const uint16_t COLTBL_MAIN[8] = {
	BLACK,
	RED,
	GREEN,
	YELLOW,
	color565(127, 127, 255), // BLUE,
	color565(255, 0, 142), // MAGENTA,
	CYAN,
	ALMOST_WHITE
};

void App_TweLite::setup_screen() {
	// LCD font color
	default_bg_color = color565(sAppData.u32_TWESTG_STAGE_BG_COLOR); // color565(90, 0, 50); 
	default_fg_color = color565(sAppData.u32_TWESTG_STAGE_FG_COLOR);

	// font register (note: to save flash area, don't create too much!)
	TWEFONT::createFontMP10(1, 0, 0); // MP10 font

	TWEFONT::createFontShinonome16(10, 0, 0, TWEFONT::U32_OPT_FONT_TATEBAI | TWEFONT::U32_OPT_FONT_YOKOBAI); // shinonome 16 font (TATE BAIKAKU)
	TWEFONT::createFontShinonome16(11, 1, 0); // shinonome 16 font
	TWEFONT::createFontMP12(12, 0, 0, TWEFONT::U32_OPT_FONT_TATEBAI | TWEFONT::U32_OPT_FONT_YOKOBAI); // MP10 font
	TWEFONT::createFontMP12(13, 0, 0); // MP10 font

	// main screen area
	the_screen.set_font(10, APP_COLUMN, APP_ROW);
	the_screen.set_color(default_fg_color, default_bg_color);
	the_screen.set_color_table(COLTBL_MAIN);
	the_screen.set_cursor(0); // 0: no 1: curosr 2: blink cursor
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
void App_TweLite::screen_refresh() {
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
void App_TweLite::change_screen_font() {
	static uint8_t idx = 1;
	uint8_t modes[] = { 10, 11, 12, 13 }; // toggle screen 10 .. 13

	the_screen.set_font(modes[idx & 0x3], APP_COLUMN, APP_ROW);
	the_screen.clear_screen();
	the_screen.force_refresh();

	auto font = TWEFONT::queryFont(the_screen.font_id());
	the_screen_b.clear_screen();
	TWE::fPrintf(the_screen_b, "\nFont: %s\n      ID=%d H:%d W:%d W_CHRs:%d",
		font.font_name, font.get_font_code(), font.height, font.width, font.font_wide_count);

	idx++;
}

/**
 * @fn	void update_screen(bool b_redraw = false)
 *
 * @brief	Updates the screen described by b_redraw
 *
 * @param	b_redraw	(Optional) True to redraw whole text.
 */
void App_TweLite::update_screen(bool b_redraw ) {
	if (b_redraw) the_screen.clear_screen();
	ITerm& trm = the_screen; // set alias

	trm << "\033[H"; // set cursor home

	bool b = b_redraw;
	bool c = false;
	if (spLastPacket) c = true;

	if (b || c) {
		if (b) trm << "\033[1;1H"
			"\033[1;30;45mﾀｲﾑCT\033[0m" "  "
			"\033[1;30;45mId#\033[0m" " "
			"\033[1;30;45mｼﾘｱﾙ番号\033[0m";
		if (b) trm << "\033[3;1H"
			"\033[1;30;45m D1 \033[0m"
			" \033[1;30;45m D2 \033[0m"
			" \033[1;30;45m D3 \033[0m"
			" \033[1;30;45m D4 \033[0m";
		if (b) trm << "\033[5;1H"
			"\033[1;30;45m A1 \033[0m"
			" \033[1;30;45m A2 \033[0m"
			" \033[1;30;45m A3 \033[0m"
			" \033[1;30;45m A4 \033[0m";
	}

	if (c) {
		TwePacketTwelite& x = refTwePacketTwelite(spLastPacket);
		if (c) trm << "\033[2;1H" << printfmt("%5d  %3d %8X", x.u16timestamp, x.u8addr_src, x.u32addr_src);
		if (c) trm << "\033[4;1H "
			<< (x.DI1 ? "\033[31m" "●" : "\033[35m" "〇") << "\033[0m   "
			<< (x.DI2 ? "\033[32m" "●" : "\033[35m" "〇") << "\033[0m   "
			<< (x.DI3 ? "\033[33m" "●" : "\033[35m" "〇") << "\033[0m   "
			<< (x.DI4 ? "\033[34m" "●" : "\033[35m" "〇") << "\033[0m";
		//                            01234567890123456789
		if (c) {
			trm << "\033[6;1H";

			uint16_t adcvals[] = { x.u16Adc1, x.u16Adc2, x.u16Adc3, x.u16Adc4 };

			bool bfirst = true;
			for (uint16_t v : adcvals) {
				if (bfirst) bfirst = false;
				else trm << " ";

				if (v < 500) {
					trm << printfmt("\033[1;34m%4d\033[0m", v); // blue
				}
				else if (v < 1000) {
					trm << printfmt("\033[1;32m%4d\033[0m", v); // green
				}
				else if (v < 1500) {
					trm << printfmt("\033[1;33m%4d\033[0m", v); // yellow
				}
				else if (v < 5000) {
					trm << printfmt("\033[1;31m%4d\033[0m", v); // red
				}
				else {
					// no value
					trm << "----";
				}
			}
		}
	}
}

// put a byte to parse
void App_TweLite::parse_a_byte(char_t u8b) {
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

		if (identify_packet_type(pkt) == E_PKT::PKT_TWELITE) {
			auto&& x = refTwePacketTwelite(pkt);

			// put information
			the_screen_b
				<< printfmt(":Lq=%d:Ad=%08X", x.u8lqi, x.u32addr_src)
				<< printfmt(":ID=%02X", x.u8addr_src)
				<< printfmt(":DI=%04b", x.DI_mask)
				;

			spLastPacket = pkt;

			update_screen();
		}
	}
}

// process input
void App_TweLite::process_input() {
	int c;

	// from TWE
	while (-1 != (c = the_uart_queue.read())) {
		// pass them to M5 (normal packet analysis)
		parse_a_byte(char_t(c));
	}
}

// check serial input.
void App_TweLite::check_for_serial() {
	// UART CHECK is DONE at ::loop() procedure.
	;
}