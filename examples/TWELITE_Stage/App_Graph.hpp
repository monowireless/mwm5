#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <mwm5.h>
#include <M5Stack.h>
#include "common.h"

#include <utility>

class App_Graph : public TWE::APP_DEF, public TWE::APP_HNDLR<App_Graph> {
public:
	static const int APP_ID = int(E_APP_ID::CUE);
	static const wchar_t LAUNCH_MSG[];

	const wchar_t* get_APP_INIT_MSG() { return LAUNCH_MSG; }
	int get_APP_ID() { return APP_ID; }

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

	// font IDs
	struct {
		uint8_t main;
		uint8_t smaller;
		uint8_t tiny;
	} font_IDs;

public:
	App_Graph(int exit_id = -1)
		: parse_ascii(512)
#if M5_SCREEN_HIRES == 0
		, the_screen_t(64, 1, { 0, 0, 320, 18 }, M5)
		, the_screen_tab(64, 20, { 0, 18, 320, 10 }, M5)
		, the_screen(64, 20, { 0, 28, 320, 240 - 18 - 10 - 30 }, M5)
		, the_screen_b(64, 4, { 0, 18 + 192, 320, 20 }, M5)
		, the_screen_c(64, 1, { 0, 18 + 192 + 20, 320, 10 }, M5)
#elif M5_SCREEN_HIRES == 1
		, the_screen_t(80, 1, { 0,   0, 640,  24 }, M5)
		, the_screen_tab(80, 2, { 0,  24, 640,  16 }, M5)
		, the_screen(56, 48, { 0,  40, 640, 400 }, M5)
		, the_screen_b(120, 2, { 0, 440, 640,  16 }, M5)
		, the_screen_c(64, 1, { 0, 456, 640,  24 }, M5)
#endif
		, default_bg_color(0)
		, default_fg_color(0)
		, _tabs(*this, the_screen_tab)
		, u8tab_selection(255)
		, font_IDs()
	{
		if (exit_id != -1) u8tab_selection = exit_id;
		set_appobj((void*)static_cast<ITerm*>(&the_screen)); // store app specific obj into APPDEF class storage.
	}

	~App_Graph() {
		APP_HNDLR<App_Graph>::on_close();
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
	void set_nav_bar(const char *msg=nullptr);

	// simple screen
	
public:
	enum PAGE_ID {
		PAGE_OPEN,
		PAGE_CUE_FIFO,
		PAGE_WSNS_DB,
		PAGE_HELP,
		_PAGE_END_
	};

	/**
	 * template function of subscreen handler (APP_HNDLR).
	 */
	EMBED_APP_HNDLR_TEMPLATE_PROCEDURE(hndr);

	// Opening screen data context 
	struct SCR_CUE_FIFO;
	friend struct SCR_CUE_FIFO;
	void hndr_SCR_CUE_FIFO(event_type ev, arg_type arg);

	// Opening screen data context 
	struct SCR_OPEN;
	friend struct SCR_OPEN;
	void hndr_SCR_OPEN(event_type ev, arg_type arg);

	// Opening screen data context 
	struct SCR_WSNS_DB;
	friend struct SCR_WSNS_DB;
	void hndr_SCR_WSNS_DB(event_type ev, arg_type arg);
	
	// Opening screen data context 
	struct SCR_HELP;
	friend struct SCR_HELP;
	void hndr_SCR_HELP(event_type ev, arg_type arg);
};

