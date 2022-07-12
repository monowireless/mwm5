#pragma once

/* Copyright (C) 2019-2022 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)
#include <mwm5.h>
#include <M5Stack.h>

#include "common.h"

class App_Manual : public APP_DEF, public APP_DESC<App_Manual>, public APP_HNDLR<App_Manual> {
private:
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

	// selection
	TWE_ListView _lv;

	// argument (check if it's called from menu selection or booting)
	int _n_arg;

public:
	App_Manual(int n_arg)
		: APP_DEF(int(E_APP_ID::MANUAL))
#if M5_SCREEN_HIRES == 0
		, the_screen(64, 20, { 0, 18, 320, 240 - 30 - 18 }, M5)
		, the_screen_t(64, 1, { 0, 0, 320, 18 }, M5)
		, the_screen_b(64, 4, { 0, 18 + 192, 320, 20 }, M5)
		, the_screen_c(64, 1, { 0, 18 + 192 + 20, 320, 10 }, M5)
#elif M5_SCREEN_HIRES == 1
		, the_screen_t(80,  1, { 0,   0, 640,  24 }, M5)
		, the_screen  (56, 16, { 0,  24, 640, 300 }, M5)
		, the_screen_b(120,10, { 0, 324, 640, 132 }, M5)
		, the_screen_c(64,  1, { 0, 456, 640,  24 }, M5)
#endif
		, default_bg_color(0)
		, default_fg_color(0)
		, _lv(8) // MAX PORTS = 8
		, _n_arg(n_arg)
	{
		set_appobj((void*)static_cast<ITerm*>(&the_screen)); // store app specific obj into APPDEF class storage.
	}

	~App_Manual() {}

public:
	// ADD_DEF managed virtual function implements.
	void setup();
	void loop();

private:
	// setup procedure
	void setup_screen();

	// update screen
	void screen_refresh(bool = false);
};
#endif