#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <mwm5.h>
#include <M5Stack.h>

#include "AppIDs.h"

#include "common.h"

class App_Settings : public TWE::APP_DEF {
public:
	static const int APP_ID = int(E_APP_ID::SETTINGS);
	int get_APP_ID() { return APP_ID; }
	const wchar_t* get_APP_INIT_MSG() { return L""; }

	
private:
	// top bar
	TWETerm_M5_Console the_screen_t; // init the screen.

	// main screen
	TWETerm_M5_Console the_screen; // init the screen.

	// bottom area (for debug information)
	TWETerm_M5_Console the_screen_c; // init the screen.

	// Pakcet Data
#if M5_SCREEN_HIRES == 0
	static const uint8_t scr_font_w = 6;
	static const uint8_t scr_font_w_zoom = 8;
	static const uint8_t scr_font_h = 12;
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
	static const uint8_t scr_font_w = 8;
	static const uint8_t scr_font_w_zoom = 8;
	static const uint8_t scr_font_h = 16;
	static const uint8_t scr_font_h_zoom = 32;
	static const uint8_t scr_max_cols = 80;
	static const uint8_t scr_max_rows = 30;
	static const int16_t scr_x = 0;
	static const int16_t scr_y = 24;
	static const int16_t scr_w = 640;
	static const int16_t scr_h = 480 - 24 - 24;
	static const int16_t scr_wz = scr_w / scr_font_w * scr_font_w_zoom; // when zoomed, assume 16x8 font
	static const int16_t scr_hz = scr_h / scr_font_h * scr_font_h_zoom;
#endif
	bool bZoom;

#ifndef ESP32
	// store screen size info
	uint8_t _screen_info;
#endif

public:
	App_Settings()
#if M5_SCREEN_HIRES == 0
		: the_screen(scr_max_cols, scr_max_rows, { scr_x, scr_y, scr_w, scr_h }, M5)
		, the_screen_t(64, 1, { 0, 0, 320, 18 }, M5)
		, the_screen_c(64, 1, { 0, 18 + 192 + 20, 320, 10 }, M5)
#elif M5_SCREEN_HIRES == 1
		: the_screen(scr_max_cols, scr_max_rows, { scr_x, scr_y, scr_w, scr_h }, M5)
		, the_screen_t(64, 1, { 0, 0, 640, 24 }, M5)
		, the_screen_c(64, 1, { 0, 480-24, 640, 24 }, M5)
#endif
		, bZoom(false)
#ifndef ESP32
		, _screen_info(0)
#endif
	{
		set_appobj((void*)static_cast<ITerm*>(&the_screen)); // store app specific obj into APPDEF class storage.
	}

	~App_Settings();

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

	// virtual scroll
	int virt_scrctl(int c, bool reset = false);
};