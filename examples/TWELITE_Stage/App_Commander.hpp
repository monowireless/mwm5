#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <mwm5.h>
#include <M5Stack.h>
#include "common.h"

#include <utility>


class App_Commander : public TWE::APP_DEF, public TWE::APP_HNDLR<App_Commander> {
public:
	static const int APP_ID = int(E_APP_ID::COMMANDER);
	static const wchar_t LAUNCH_MSG[];

	const wchar_t* get_APP_INIT_MSG() { return LAUNCH_MSG; }
	int get_APP_ID() { return APP_ID;  }

private:
	// Serial Parser
	AsciiParser parse_ascii;

	// top bar
	TWETerm_M5_Console the_screen_t; // init the screen.

	// tab bar
	TWETerm_M5_Console the_screen_tab; // init the screen.
	TWE_WidSet_Tabs _tabs;

	// main screen
	TWETerm_M5_Console the_screen; // init the screen.

	// bottom area (for debug information)
	TWETerm_M5_Console the_screen_b; // init the screen.

	// bottom area (for debug information)
	TWETerm_M5_Console the_screen_c; // init the screen.

	// default color
	uint16_t default_bg_color;
	uint16_t default_fg_color;

	// Components
	uint8_t u8tab_selection;
	SimpleBuffer<upTWE_Button> _btns;

public:
	App_Commander()
		: parse_ascii(256)
		, the_screen_t(64, 1, { 0, 0, 320, 18 }, M5)
		, the_screen_tab(64, 20, { 0, 18, 320, 10 }, M5)
		, the_screen(64, 20, { 0, 28, 320, 240 - 18 - 10 -30 }, M5)
		, the_screen_b(64, 4, { 0, 18 + 192, 320, 20 }, M5)
		, the_screen_c(64, 1, { 0, 18 + 192 + 20, 320, 10 }, M5)
		, default_bg_color(0)
		, default_fg_color(0)
		, _tabs(*this, the_screen_tab)
		, u8tab_selection(255)
	{
		set_appobj((void*)static_cast<ITerm*>(&the_screen_b)); // store app specific obj into APPDEF class storage.
	}

	~App_Commander() {
		APP_HNDLR::on_close();
	}

	void setup();

	void loop();


private:
	// setup procedure
	void setup_screen();

	// update screen
	void screen_refresh();

	// set title bar 
	void set_title_bar(int page_id);

	// set navigation bar on the bottom
	void set_nav_bar();

	// simple screen
	void hndr_opening(event_type ev, arg_type arg = 0);
	void hndr_notice_pal_0x01(event_type ev, arg_type arg = 0);
	void hndr_twelite80(event_type ev, arg_type arg = 0);

	
public:
	struct PAGE_ID {
		static const uint8_t PAGE_OPEN = 0;
		static const uint8_t PAGE_TWELITE80 = 1;
		static const uint8_t PAGE_NOTICE01 = 2;
	};

	// Opening screen data context 
	struct SCR_OPEN;
	friend struct SCR_OPEN;
	struct SCR_NOTICE1;
	friend struct SCR_NOTICE1;
	struct SCR_TWELITE80;
	friend struct SCR_TWELITE80;
};