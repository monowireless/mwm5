#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <mwm5.h>
#include <M5Stack.h>
#include "common.h"


class App_Glancer : public TWE::APP_DEF, public TWE::APP_DESC<App_Glancer>, public TWE::APP_HNDLR<App_Glancer> {

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
		uint8_t medium;
		uint8_t smaller;
		uint8_t tiny;
	} font_IDs;

	// screen layout
	struct {
		bool b_saved;
		Rect the_screen;
		Rect the_screen_b;
	} layout;

public:
	App_Glancer(int exit_id = -1)
		: APP_DEF(int(E_APP_ID::SMPL_VIEWER))
		, parse_ascii(512)
#if M5_SCREEN_HIRES == 0
		, the_screen_t(64, 1, { 0, 0, 320, 18 }, M5)
		, the_screen_tab(64, 20, { 0, 18, 320, 10 }, M5)
		, the_screen(64, 20, { 0, 28, 320, 240 - 18 - 10 - 30 }, M5)
		, the_screen_b(64, 4, { 0, 18 + 192, 320, 20 }, M5)
		, the_screen_c(64, 1, { 0, 18 + 192 + 20, 320, 10 }, M5)
#elif M5_SCREEN_HIRES == 1
		, the_screen_t(80, 1, { 0,   0, 640,  24 }, M5)
		, the_screen_tab(80, 2, { 0,  24, 640,  16 }, M5)
		, the_screen(56, 48, { 0,  40, 640, 300 }, M5)
		, the_screen_b(120, 10, { 0, 340, 640, 116 }, M5)
		//, the_screen(56, 48, { 0,  40, 640, 400 }, M5)
		//, the_screen_b(120, 2, { 0, 440, 640,  16 }, M5)
		, the_screen_c(64, 1, { 0, 456, 640,  24 }, M5)
#endif
		, default_bg_color(0)
		, default_fg_color(0)
		, _tabs(*this, the_screen_tab)
		, u8tab_selection(255)
		, font_IDs()
		, layout()
	{
		if (exit_id != -1) u8tab_selection = exit_id;
		set_appobj((void*)static_cast<ITerm*>(&the_screen)); // store app specific obj into APPDEF class storage.
	}

	~App_Glancer() {
		APP_HNDLR<App_Glancer>::on_close();
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

	// restore layout
	void screen_layout_opening();

	// set screen layout for apps.
	void screen_layout_apps();

public:
	enum PAGE_ID {
		PAGE_OPEN,
		PAGE_SCR_GLANCER,
		PAGE_SCR_CUE,
		PAGE_SCR_ARIA,
		_PAGE_END_
	};

	/**
	 * template function of subscreen handler (APP_HNDLR).
	 */
	EMBED_APP_HNDLR_TEMPLATE_PROCEDURE(hndr);

	// Opening screen data context 
	struct SCR_GLANCER;
	friend struct SCR_GLANCER;
	void hndr_SCR_GLANCER(event_type ev, arg_type arg);

	// Opening screen data context 
	struct SCR_CUE_BASIC;
	friend struct SCR_CUE_BASIC;
	void hndr_SCR_CUE_BASIC(event_type ev, arg_type arg);

	// Opening screen data context 
	struct SCR_ARIA_BASIC;
	friend struct SCR_ARIA_BASIC;
	void hndr_SCR_ARIA_BASIC(event_type ev, arg_type arg);

	// Opening screen data context 
	struct SCR_OPEN;
	friend struct SCR_OPEN;
	void hndr_SCR_OPEN(event_type ev, arg_type arg);
};
