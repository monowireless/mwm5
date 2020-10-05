#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <mwm5.h>
#include <M5Stack.h>

#include "common.h"

class App_RootMenu : public TWE::APP_DEF {
public:
	static const int APP_ID = int(E_APP_ID::ROOT_MENU);
	int get_APP_ID() { return APP_ID; }
	const wchar_t* get_APP_INIT_MSG() { return L""; }

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
	TWECUI::TWE_ListView _listMenu;
	TWESYS::TimeOut _exit_timer;

	TWECUI::TWE_ListView::index_type _nSel;

	bool _b_appmenu; // true when viwer app menu is in select.
	int _i_selected_viewer_app; // if viewer app is selected

public:
	App_RootMenu(int nSel = 0)
		: the_screen(64, 20, { 0, 18, 320, 240 - 30 - 18 }, M5)
		, the_screen_t(64, 1, { 0, 0, 320, 18 }, M5)
		, the_screen_b(64, 4, { 0, 18 + 192, 320, 20 }, M5)
		, the_screen_c(64, 1, { 0, 18 + 192 + 20, 320, 10 }, M5)
		, default_bg_color(0)
		, default_fg_color(0)
		, _listMenu(8)
		, _exit_timer()
		, _nSel(nSel)
		, _b_appmenu(false)
		, _i_selected_viewer_app(APP_ID)
	{
		set_appobj((void*)static_cast<ITerm*>(&the_screen)); // store app specific obj into APPDEF class storage.
	}

	~App_RootMenu() {}

	void setup();

	void loop();

private:
	// menu selection
	void set_listview();
	
	// setup procedure
	void setup_screen();

	// update screen
	void check_for_refresh();

	// set title or status bar 
	void set_title_status();
};