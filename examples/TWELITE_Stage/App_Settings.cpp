/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_Settings.hpp"

#include "menu_defs.h"
#include "menu.hpp"

void App_Settings::setup() {
	// preference
	the_settings_menu.begin(0);

#ifndef ESP32
	// save screen info (if changed, change screen size when exiting screen)
	_screen_info = sAppData.u8_TWESTG_STAGE_SCREEN_MODE;
#endif

	// init the TWE M5 support
	setup_screen(); // initialize TWE M5 support.

	// put a init message
	the_screen_t << "\033[G設定メニュー";

	// button navigation
	//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
	the_screen_c << "     --/長押:MENU        ズーム/--                --/--";

	// menu init
	the_settings_menu.begin(
		the_screen
		, the_screen_t
		, the_keyboard
		, asFuncs
		, nullptr // from get_setlist() method
		, 0 // default screen (0:select menu, 1:direct to setting)
	);
}

App_Settings::~App_Settings() {
#ifndef ESP32
	// if screen mode has been changed, set the mode here.
	if (_screen_info != sAppData.u8_TWESTG_STAGE_SCREEN_MODE) {
		push_window_event(
			SDL2_USERCODE_CREATE_BYTE(SDL2_USERCODE_CHANGE_SCREEN_SIZE, sAppData.u8_TWESTG_STAGE_SCREEN_MODE >> 4),
			nullptr, nullptr);
		push_window_event(
			SDL2_USERCODE_CREATE_BYTE(SDL2_USERCODE_CHANGE_SCREEN_RENDER, sAppData.u8_TWESTG_STAGE_SCREEN_MODE & 0xF),
			nullptr, nullptr);
	}
#endif
}

void App_Settings::loop() { 
	// check for serial input
	the_settings_menu.update();

	while(the_settings_menu.key_unhandled.available()) {
		int c = the_settings_menu.key_unhandled.read();

		c = virt_scrctl(c);

		switch(c) {
		case KeyInput::KEY_BUTTON_A:
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
			//change_screen_color();
			break;

		case KeyInput::KEY_BUTTON_C:
			//twe_prog.reset_module();
			break;

		case KeyInput::KEY_BUTTON_C_LONG:
			break;
		}
	}

	virt_scrctl(0);
	
	// LCD update
	screen_refresh();
}

void App_Settings::setup_screen() {
	// font register (note: to save flash area, don't create too much!)
	TWEFONT::createFontMP10_std(1, 0, 0); // MP10 font

#ifdef ESP32
	TWEFONT::createFontMP12_mini(10, 0, 0); // MP12 font
#else
	TWEFONT::createFontMP12_std(10, 0, 0); // MP12 font
#endif
	TWEFONT::createFontShinonome16_mini(11, 0, 0); // shinonome 16 font

	// main screen area
	the_screen.set_color(ALMOST_WHITE, BLACK);
	the_screen.set_cursor(2); // 0: no 1: curosr 2: blink cursor
	the_screen.set_wraptext(false); // no wrap text mode
	bZoom = true; change_screen_font(); // bZoom is toggled from true to false.

	// bottom area
	the_screen_c.set_font(1);
	the_screen_c.set_color(ORANGE, color565(20, 20, 20));
	the_screen_c.set_cursor(0);
	the_screen_c.force_refresh();

	// top area
	the_screen_t.set_font(11);
	the_screen_t.set_color(BLACK, ALMOST_WHITE);
	the_screen_t.set_cursor(0);
	the_screen_t.force_refresh();
}

// screen refresh timing (every 32ms)
void App_Settings::screen_refresh() {
	static uint32_t u32mills;

	uint32_t u32now = millis();
	if (u32now - u32mills > 32) {
		//if (bZoom) virt_scrctl(0); // in zoom mode, follow cursor position

		the_screen.refresh();
		//the_screen_b.refresh();
		the_screen_c.refresh();
		the_screen_t.refresh();

		u32mills = u32now;
	}
}

// change screen font
void App_Settings::change_screen_font() {
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

// process input
void App_Settings::process_input() {
	int ct = 0;

	// the keyboard
	while (the_keyboard.available()) {
		int c = the_keyboard.read();
		c = virt_scrctl(c);
		if (c >= 0 && c <= 0x7F) WrtTWE << char_t(c);
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
void App_Settings::check_input() {
	;
}

int App_Settings::virt_scrctl(int c, bool reset) {
	if (bZoom) {		
		// follow to the cursor position
		if (c != -1) {
			// set to cursor pos
			int16_t c_x, c_y;

			// current viewing screen area
			int16_t y_start = 0;
			int16_t x_start = 0;

			static int16_t y_last = 0;
			static int16_t x_last = 0;

			// get cursor position as relative coordinate.
			the_screen.get_cursor_pos_relative(c_x, c_y);

			if (c_y >= scr_hz) y_start = c_y - scr_hz;
			if (c_x >= scr_wz) x_start = c_x - scr_wz;

			if (y_last != y_start || x_last != x_start) {
				the_screen.set_view_pos(y_start, x_start);
				the_screen.force_refresh();

				y_last = y_start;
				x_last = x_start;
			}
		}
	}
	else {
		if (reset) the_screen.force_refresh();
	}

	return c;
}