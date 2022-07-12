#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <mwm5.h>
#include <M5Stack.h>

#include "common.h"

class App_TweLite : public TWE::APP_DEF, public TWE::APP_DESC<App_TweLite> {
private:
	// Serial Parser
	AsciiParser parse_ascii;

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

	// Pakcet Data
	spTwePacket spLastPacket;

	// some vars
	uint8_t _screen_font_idx;	// for toggling screen font selection.
	int _ct_packets;			// count of incoming packets
	int _idx_test_data;	        // for toggling test data.
	uint32_t _u32_millis_last_screen_update; // tick when the last screen update was performed.

	// fixed values
	static const int APP_COLUMN = 19;
	static const int APP_ROW = 6;

public:
	App_TweLite()
		: APP_DEF(int(E_APP_ID::TWELITE))
		, default_bg_color(0)
		, default_fg_color(0)
		, parse_ascii(256)
#if M5_SCREEN_HIRES == 0
		, the_screen(64, 20, { 0, 18, 320, 240 - 30 - 18 }, M5)
		, the_screen_t(64, 1, { 0, 0, 320, 18 }, M5)
		, the_screen_b(64, 4, { 0, 18 + 192, 320, 20 }, M5)
		, the_screen_c(64, 1, { 0, 18 + 192 + 20, 320, 10 }, M5)
#elif M5_SCREEN_HIRES == 1
		, the_screen(24, 6, { 0,  24, 640, 400 }, M5)
		, the_screen_t(80, 1, { 0,   0, 640,  24 }, M5)
		, the_screen_b(120, 2, { 0, 424, 640,  32 }, M5)
		, the_screen_c(64, 1, { 0, 456, 640,  24 }, M5)
#endif

		, spLastPacket()
		, _screen_font_idx(0), _ct_packets(0), _idx_test_data(0), _u32_millis_last_screen_update(0)
	{
		set_appobj((void*)static_cast<ITerm*>(&the_screen)); // store app specific obj into APPDEF class storage.
	}

	~App_TweLite() {}

	void setup();

	void loop();

private:
	void update_screen(bool b_redraw = false);
	void parse_a_byte(char_t u8b);
	void process_input();
	void check_for_serial();

	// setup procedure
	void setup_screen();

	// update screen
	void screen_refresh();

	// change screen font of `the_screen'
	void change_screen_font();
};