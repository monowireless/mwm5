/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_Console.hpp"

const wchar_t App_Console::LAUNCH_MSG[] =
//....+....1....+....2....+....3....+....4| // 16dots 40cols
L"              " 
   L"\033[4m" L"ターミナル" L"\033[0m"
		                L"                ""\r\n"
L"\r\n"
L"TWELITE無線モジュールのシリアル入出力を ""\r\n"
L"行うターミナルビューアです。通信条件は、""\r\n"
L"115200bps 8N1 に固定されています。"      "\r\n"
L"左ボタン長押・ESCを素早く２回入力するこ ""\r\n"
L"とでビューアを終了します。" "\r\n";

 void App_Console::setup() {
	// preference
	the_settings_menu.begin(appid_to_slotid(APP_ID));

	// init the TWE M5 support
	setup_screen(); // initialize TWE M5 support.

	// put a init message
	the_screen_t << L"\033[G\033[1mTWELITE\033[0m®\033[1mﾀｰﾐﾅﾙ\033[0m";

	// button navigation
	//              "0....+....1....+....2....+....3....+....4....+....5....+...."
	set_navbtn_bar();
}

void App_Console::loop() { 
	// check for serial input
	check_input();

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

void App_Console::setup_screen() {
	// LCD font color
	default_bg_color = color565(sAppData.u32_TWESTG_STAGE_BG_COLOR); // color565(90, 0, 50); 
	default_fg_color = color565(sAppData.u32_TWESTG_STAGE_FG_COLOR);

	// font register (note: to save flash area, don't create too much!)
	TWEFONT::createFontMP10(1, 0, 0); // MP10 font

	TWEFONT::createFontMP10(10, 0, 0); // MP10 font
	TWEFONT::createFontShinonome16(11, 0, 0); // shinonome 16 font

	// main screen area
	the_screen.set_color(default_fg_color, default_bg_color);
	the_screen.set_color_table(COLTBL_MAIN);
	the_screen.set_cursor(2); // 0: no 1: curosr 2: blink cursor
	the_screen.set_wraptext(bWrap); // no wrap text mode
	bZoom = true; change_screen_font(); // bZoom is toggled from true to false.

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
void App_Console::screen_refresh() {
	static uint32_t u32mills;

	uint32_t u32now = millis();
	if (u32now - u32mills > 32) {
		//if (bZoom) virt_scrctl(0); // in zoom mode, follow cursor position

		if (_ct_refresh == 0) {
			the_screen.refresh();
			//the_screen_b.refresh();
			the_screen_c.refresh();
			the_screen_t.refresh();
		}
		else {
			_ct_refresh--;

			if (!_ct_refresh) the_screen.force_refresh();
		}
		
		u32mills = u32now;
	}
}

// change screen font
void App_Console::change_screen_font() {
	// toggle the state
	bZoom = !bZoom;

	signed rows = the_screen.get_rows();
	signed cols = the_screen.get_cols();

	std::unique_ptr<GChar[]> scrbuf(new GChar[scr_max_cols * scr_max_cols]);

	the_screen.get_screen_buf(scrbuf.get());

	if (bZoom) {
		the_screen.set_draw_area({scr_x, scr_y, scr_wz, scr_hz});
		the_screen.set_view_pos(0, 0);
		the_screen.set_view_size(scr_w, scr_h);
		the_screen.set_font(11, scr_max_cols, scr_max_rows);
	}
	else {
		the_screen.set_draw_area({ scr_x, scr_y, scr_w, scr_h });
		the_screen.set_view_pos(0, 0);
		the_screen.set_view_size(scr_w, scr_h);
		the_screen.set_font(10, scr_max_cols, scr_max_rows);
	}

	the_screen.set_screen_buf(scrbuf.get(), cols, rows);
	virt_scrctl(0, true); // set visible area including cursor position (and refresh screen)
}

void App_Console::set_navbtn_bar() {
	the_screen_c.clear_screen();

	if (bWrap) {
		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << L" 設定(+++)/長押:MENU    ズーム/折返[ON]          --/ﾘｾｯﾄ";
	}
	else {
		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << L" 設定(+++)/長押:MENU    ズーム/折返        　    --/ﾘｾｯﾄ";
	}

	the_screen_c.force_refresh();
}

// change screen color
void App_Console::change_wrap() {
	bWrap = !bWrap;

	the_screen.set_wraptext(bWrap);
	the_screen.force_refresh();

	set_navbtn_bar();
}

// process input
void App_Console::process_input() {
	int ct = 0;

	// the keyboard
	while (the_keyboard.available()) {
		int c = the_keyboard.read();
		
		c = virt_scrctl(c);

		// double ESC will leave.
		if (c == KeyInput::KEY_ESC) {
			static uint32_t tick_last;
			if (millis() - tick_last < 300) {
				the_app.exit(APP_ID);
				return;
			}
			tick_last = millis();
		}

		if (c >= 0 && c <= 0x7F) {
			WrtTWE << char_t(c);
		} 
		else switch (c) {
		case KeyInput::KEY_BUTTON_A:
			// interactive mode
			WrtTWE << '+';
			delay(400);
			WrtTWE << '+';
			delay(400);
			WrtTWE << '+';
			break;

		case KeyInput::KEY_BUTTON_A_LONG:
			// back to menu
			the_app.exit(APP_ID);
			break;

		case KeyInput::KEY_BUTTON_B:
			// change font
			change_screen_font();
			break;

		case KeyInput::KEY_BUTTON_B_LONG:
			// change colour
			change_wrap();
			break;

		case KeyInput::KEY_BUTTON_C:
			break;

		case KeyInput::KEY_BUTTON_C_LONG:
			twe_prog.reset_module();
			break;

		case KeyInput::KEY_UP: WrtTWE << "\033[A"; break;
		case KeyInput::KEY_DOWN: WrtTWE << "\033[B"; break;
		case KeyInput::KEY_LEFT: WrtTWE << "\033[D"; break;
		case KeyInput::KEY_RIGHT: WrtTWE << "\033[C"; break;
		}
	}

	while (the_uart_queue.available()) {
		int c = the_uart_queue.read();
		// pass them to M5 (normal packet analysis)
		if (c >= 0) {
			the_screen << char_t(c);
			ct++;
		}
	}

	if (ct) {
		// if changed in screen, set the virt screen with cursor visible.
		virt_scrctl(0);
	}
}

// check serial input.
void App_Console::check_input() {
	;
}

int App_Console::virt_scrctl(int c, bool reset) {
	if (bZoom) {
		static uint8_t ix;
		static uint8_t iy;
		bool bForceRedraw = false;
		uint8_t ix2 = ix, iy2 = iy;

		if (reset) {
			ix = ix2 = 0;
			iy = iy2 = 0;
			bForceRedraw = true;
		}

		// view change by keyboard operation
		switch (c) {
		case KeyInput::KEY_LEFT: ix2 = (ix - 1) & 1; c = -1;  break;
		case KeyInput::KEY_RIGHT: ix2 = (ix + 1) & 1; c = -1; break;
		case KeyInput::KEY_UP: iy2 = (iy - 1) & 1; c = -1;  break;
		case KeyInput::KEY_DOWN: iy2 = (iy + 1) & 1; c = -1; break;
		}
		
		// follow to the cursor position
		if (c != -1) {
			// set to cursor pos
			int16_t c_x, c_y;

			// current viewing screen area
			int16_t y1 = iy ? (scr_hz - scr_h) : 0;
			int16_t y2 = y1 + scr_h - 1;
			int16_t x1 = ix ? (scr_wz - scr_w) : 0;
			int16_t x2 = x1 + scr_w - 1;

			// get cursor position as relative coordinate.
			the_screen.get_cursor_pos_relative(c_x, c_y);

			// check if cursor is in the current viewing area.
			if (c_x < x1 || c_x > x2 - 16) ix2 = (ix + 1) & 1;
			if (c_y < y1 || c_y > y2) iy2 = (iy + 1) & 1;
		}

		// do redraw
		if (bForceRedraw || ix != ix2 || iy != iy2) {
			ix = ix2;
			iy = iy2;

			the_screen.set_view_pos(ix ? (scr_wz - scr_w) : 0, iy ? (scr_hz - scr_h) : 0);
			_ct_refresh = 2;
		}
	}
	else {
		if (reset)
			_ct_refresh = 2;
	}

	return c;
}