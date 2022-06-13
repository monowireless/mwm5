#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <mwm5.h>
#include <M5Stack.h>

#include "common.h"

class App_Interactive : public APP_DEF, public APP_HNDLR<App_Interactive> {
public:
	static const int APP_ID = int(E_APP_ID::INTERACTIVE);
	int get_APP_ID() { return APP_ID; }
	const wchar_t* get_APP_INIT_MSG() { return L""; }

	
private:
	// top bar
	TWETerm_M5_Console the_screen_t; // init the screen.

	// main screen
	TWETerm_M5_Console the_screen; // init the screen.

	// bottom area (for debug information)
	TWETerm_M5_Console the_screen_msg; // init the screen.

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

	// refresh hold counter.
	int _ct_refresh;

	// Interactive mode checking
	enum class E_STAT {
		NONE = 0,
		RESET_WITH_SET,
		PRESS_3PLUSES,
		DETECT_ENTERING,
		DETECT_EXIT_MESSAGE
	};

	struct INTR {
		static const uint16_t UART_LINE_CHAR_MAX = 255;
		E_STAT stat;
		TWESYS::TimeOut timeout; // intr stat timeout
		InputQueue<uint8_t> que_uart; // uart line buffer (check input content)
		TWESYS::TimeOut timeout_uart; // line input timeout
		oss_regex re; // regex
		oss_regex re2;
		uint16_t n_lines;
		uint8_t b_now_input;

		INTR()
			: stat()	
			, timeout()
			, que_uart(UART_LINE_CHAR_MAX)
			, timeout_uart()
			, re(), re2()
			, n_lines(0)
			, b_now_input(0)
			{}
	};
	std::unique_ptr<INTR> _sp_intr;

public:
	App_Interactive()
#if M5_SCREEN_HIRES == 0
		: the_screen(scr_max_cols, scr_max_rows, { scr_x, scr_y, scr_w, scr_h }, M5)
		, the_screen_t(64, 1, { 0, 0, 320, 18 }, M5)
		, the_screen_msg(40, 13, { scr_x, scr_y, scr_w, scr_h }, M5)
		, the_screen_c(64, 1, { 0, 18 + 192 + 20, 320, 10 }, M5)
#elif M5_SCREEN_HIRES == 1
		: the_screen(scr_max_cols, scr_max_rows, { scr_x, scr_y, scr_w, scr_h }, M5)
		, the_screen_msg(40, 12, { scr_x, scr_y, scr_w, scr_h }, M5)
		, the_screen_t(64, 1, { 0, 0, 640, 24 }, M5)
		, the_screen_c(64, 1, { 0, 480 - 24, 640, 24 }, M5)
#endif
		, bZoom(false)
		, default_bg_color(0)
		, default_fg_color(0)
		, _ct_refresh(0)
		, _sp_intr()
	{
		set_appobj((void*)static_cast<ITerm*>(&the_screen)); // store app specific obj into APPDEF class storage.
	}

	~App_Interactive() {
		APP_HNDLR::on_close();

		// take back app specific baud
		change_baud(MWM5_DEFAULT_BAUD);
		twe_prog.set_baud_app(MWM5_DEFAULT_BAUD);
	}

	void setup();

	void loop();

private:
	void hndlr_init_screen(event_type ev, arg_type arg = 0);
	void hndlr_main_screen(event_type ev, arg_type arg = 0);

private:
	void monitor_uart(KeyInput::keyinput_type c);

	// setup procedure
	void setup_screen();

	// update screen
	void screen_refresh();

	// change screen font of `the_screen'
	void change_screen_font();

	// change screen color of `the_screen'
	void change_screen_color();

	// virtual scroll
	int virt_scrctl(int c, bool reset = false);
};
