#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <mwm5.h>
#include <M5Stack.h>

#include "common.h"


class App_Console : public TWE::APP_DEF, public TWE::APP_DESC<App_Console> {
private:
	// top bar
	TWETerm_M5_Console the_screen_t; // init the screen.

	// main screen
	TWETerm_M5_Console the_screen; // init the screen.

	// bottom area (for debug information)
	//TWETerm_M5_Console the_screen_b; // init the screen.

	// bottom area (for debug information)
	TWETerm_M5_Console the_screen_c; // init the screen.

	// default color
	uint16_t default_bg_color;
	uint16_t default_fg_color;

	// Pakcet Data
#if M5_SCREEN_HIRES == 0
	static const uint8_t scr_font_w = 5;
	static const uint8_t scr_font_w_zoom = 8;
	static const uint8_t scr_font_h = 10;
	static const uint8_t scr_font_h_zoom = 16;
	static const uint8_t scr_max_cols = 64;
	static const uint8_t scr_max_rows = 21;
	static const int16_t scr_x = 0;
	static const int16_t scr_y = 18;
	static const int16_t scr_w = 320;
	static const int16_t scr_h = 240 - 18 - 10;
	static const int16_t scr_wz = scr_w / scr_font_w * scr_font_w_zoom; // when zoomed, assume 16x8 font
	static const int16_t scr_hz = scr_h / scr_font_h * scr_font_h_zoom;
#elif M5_SCREEN_HIRES == 1
	static const uint8_t scr_font_w = 6;
	static const uint8_t scr_font_w_zoom = 8;
	static const uint8_t scr_font_h = 12;
	static const uint8_t scr_font_h_zoom = 16;
	static const uint8_t scr_max_cols = 120;
	static const uint8_t scr_max_rows = 40;
	static const int16_t scr_x = 0;
	static const int16_t scr_y = 24;
	static const int16_t scr_w = 640;
	static const int16_t scr_h = 480 - 24 - 24;
	static const int16_t scr_wz = scr_w / scr_font_w * scr_font_w_zoom; // when zoomed, assume 16x8 font
	static const int16_t scr_hz = scr_h / scr_font_h * scr_font_h_zoom;
#endif
	bool bZoom;
	bool bWrap;

	int _ct_refresh;

	// Interactive menu
	

public:
	App_Console() : APP_DEF(int(E_APP_ID::CONSOLE))
#if M5_SCREEN_HIRES == 0
		, the_screen(scr_max_cols, scr_max_rows, { scr_x, scr_y, scr_w, scr_h }, M5)
		, the_screen_t(64, 1, { 0, 0, 320, 18 }, M5)
		, the_screen_c(64, 1, { 0, 18 + 192 + 20, 320, 10 }, M5)
#elif M5_SCREEN_HIRES == 1
		, the_screen(scr_max_cols, scr_max_rows, { scr_x, scr_y, scr_w, scr_h }, M5)
		, the_screen_t(80, 1, { 0, 0, 640, 24 }, M5)
		, the_screen_c(64, 1, { 0, 480 - 24, 640, 24 }, M5)
#endif
		, bZoom(false), bWrap(true)
		, default_bg_color(0)
		, default_fg_color(0)
		,_ct_refresh(0)
	{
		set_appobj((void*)static_cast<ITerm*>(&the_screen)); // store app specific obj into APPDEF class storage.
	}

	~App_Console() {
		// take back app specific baud
		change_baud(MWM5_DEFAULT_BAUD);
		twe_prog.set_baud_app(MWM5_DEFAULT_BAUD);
	}

	void setup();

	void loop();

private:
	void process_input();
	void check_input();

	// setup procedure
	void setup_screen();

	// update screen
	void screen_refresh();

	// change screen font of `the_screen'
	void change_screen_font();

	// change screen color of `the_screen'
	void change_wrap();

	// change navigation string.
	void set_navbtn_bar();

	// virtual scroll
	int virt_scrctl(int c, bool reset = false);
};
