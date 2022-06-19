/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_Graph.hpp"

using APP_BASE = App_Graph;

const wchar_t App_Graph::LAUNCH_MSG[] =
//....+....1....+....2....+....3....+....4| // 16dots 40cols
L" \033[4m"
   L"グラフ表示\033[0m"
	                      L" ""\r\n"
L"\r\n"
L"TWELITE からの電文を解釈してグラフを" "\r\n"
L"表示します" "\r\n"
;

// color table
static uint16_t COLTBL_MAIN[8] = {
	BLACK,
	RED,
	GREEN,
	YELLOW,
	BLUE, // color565(127, 127, 255), // BLUE,
	MAGENTA, //color565(255, 0, 142), // MAGENTA,
	CYAN,
	ALMOST_WHITE
};

void App_Graph::setup() {
	// preference
	the_settings_menu.begin(appid_to_slotid(APP_ID));

	// init the TWE M5 support
	setup_screen(); // initialize TWE M5 support.

	// button navigation
	set_nav_bar();

	// add tab
	for (int i = 0; i < PAGE_ID::_PAGE_END_; i++) {
		switch (i) {
		case PAGE_ID::PAGE_OPEN: _tabs.add(L"---", &App_Graph::hndr_SCR_OPEN); break;
		case PAGE_ID::PAGE_CUE_FIFO: _tabs.add(L"CUEｸﾞﾗﾌ", &App_Graph::hndr_SCR_CUE_FIFO); break;
		case PAGE_ID::PAGE_WSNS_DB: _tabs.add(L"ｾﾝｻｰｸﾞﾗﾌ", &App_Graph::hndr_SCR_WSNS_DB); break;
		case PAGE_ID::PAGE_HELP: _tabs.add(L"解説", &App_Graph::hndr_SCR_HELP); break;
		default: break;
		}
	}
	_tabs.update_view();

	if (u8tab_selection < _tabs.size()) {
		_tabs.select(u8tab_selection);
	}
}

void App_Graph::loop() {
	// tab event handling
	_tabs.check_events();

	// peek keyevent and perform common event handling.
	// if the event is not for this controls, APP_HNDLR will take care of.
	bool b_loop = true;
	do {
		int c = the_keyboard.peek_a_byte();

		if (c == KeyInput::KEY_BUTTON_A) {
			_tabs.select_prev();
		}
		else if (c == KeyInput::KEY_BUTTON_C) {
			_tabs.select_next();
		}
		else if (c == KeyInput::KEY_ESC || KeyInput::is_mouse_right_up(c)) {
			// handle double ESC/double right click
			static uint32_t t_last;
			uint32_t t_now = millis();

			if (t_now - t_last < 300) {
				the_app.exit(APP_ID);
			}
			else {
				b_loop = false;
			}
			t_last = t_now;
		}
		else if (c == KeyInput::KEY_BUTTON_A_LONG) {
			the_app.exit(APP_ID);
		}
		else if (c == KeyInput::KEY_BUTTON_C_LONG) {
			twe_prog.reset_module();
		}
		else {
			b_loop = false; // message is not used, pass the event to TAB content.
		}

		if (b_loop) { // still looping
			the_keyboard.get_a_byte(); // skip this byte
			c = the_keyboard.peek_a_byte(); // next byte
		}
	} while (b_loop);

	// loop by APP_HNDLR
	APP_HNDLR::loop();

	// LCD update
	screen_refresh();
}

void App_Graph::setup_screen() {
	// LCD font color
	default_bg_color = color565(sAppData.u32_TWESTG_STAGE_BG_COLOR); // color565(90, 0, 50); 
	default_fg_color = color565(sAppData.u32_TWESTG_STAGE_FG_COLOR);

#if M5_SCREEN_HIRES == 0
	// font register (note: to save flash area, don't create too much!)
	TWEFONT::createFontMP10_std(1, 0, 0); // MP10 font
	TWEFONT::createFontMP12(10, 0, 0); // MP11 font
	TWEFONT::createFontShinonome16_mini(11, 1, 0); // shinonome 16 font

	the_screen_t.set_font(11);
	the_screen_tab.set_font(1);
	the_screen.set_font(10);
	the_screen_b.set_font(1);
	the_screen_c.set_font(1);

#elif M5_SCREEN_HIRES == 1

	font_IDs.main = 11;
	font_IDs.smaller = 14;
	font_IDs.tiny = 1;

	TWEFONT::createFontMP10_std(1, 0, 0);

	TWEFONT::createFontShinonome16(10, 0, 0); // normal font

	TWEFONT::createFontShinonome16_full(font_IDs.main, 4, 2); // MP11 font

	TWEFONT::createFontMP10_std(12, 0, 0, TWEFONT::U32_OPT_FONT_YOKOBAI | TWEFONT::U32_OPT_FONT_TATEBAI);

	TWEFONT::createFontShinonome16(13, 0, 0, 0 /* TWEFONT::U32_OPT_FONT_YOKOBAI */);

	TWEFONT::createFontMP12_std(font_IDs.smaller, 0, 0);

	the_screen_t.set_font(13);
	the_screen_tab.set_font(10);
	the_screen.set_font(font_IDs.main);
	the_screen_b.set_font(font_IDs.smaller);
	the_screen_c.set_font(12);

#endif

	// main screen area
	the_screen.set_color(default_fg_color, default_bg_color);
	the_screen.set_cursor(0); // 0: no 1: curosr 2: blink cursor
	the_screen.set_wraptext(false);
	the_screen.force_refresh();

	// tab area
	the_screen_tab.set_color(default_bg_color, color565(0x80, 0x80, 0x80));
	the_screen_tab.set_cursor(0); // 0: no 1: curosr 2: blink cursor
	the_screen_tab.set_color_table(COLTBL_MAIN);
	the_screen_tab.force_refresh();

	// bottom area (sub screen)
	the_screen_b.set_color(color565(80, 80, 80), color565(20, 20, 20));
	the_screen_b.set_cursor(0);
	the_screen_b.force_refresh();

	// top area
	the_screen_t.set_color(default_bg_color, default_fg_color);
	the_screen_t.set_cursor(0);
	the_screen_t.force_refresh();

	// nav bar area
	the_screen_c.set_color(ORANGE, color565(20, 20, 20));
	the_screen_c.set_cursor(0);
	the_screen_c.force_refresh();
}

// screen refresh timing (every 32ms)
void App_Graph::screen_refresh() {
	static uint32_t u32mills;

	uint32_t u32now = millis();
	if (u32now - u32mills > 32) {
		the_screen.refresh();
		the_screen_tab.refresh();
		the_screen_b.refresh();
		the_screen_c.refresh();
		the_screen_t.refresh();

		u32mills = u32now;
	}
}

// button navigation
void App_Graph::set_nav_bar(const char *msg) {
	the_screen_c.clear_screen();

	if (msg == nullptr) {
		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << "  前TAB/長押:戻る            --/--             次TAB/ﾘｾｯﾄ";
	}
	else {
		the_screen_c << msg;
	}
}

// set title bar
void App_Graph::set_title_bar(int page_id) {
	const char* title = "\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m CUE/ARIAﾋﾞｭｰｱ\033[0m";

	the_screen_t.clear_screen();

	switch (page_id) {
	case PAGE_ID::PAGE_CUE_FIFO:
		the_screen_t << title << ":加速度ﾘｱﾙﾀｲﾑｸﾞﾗﾌ"; break;
	case PAGE_ID::PAGE_WSNS_DB:
		the_screen_t << title << ":センサーグラフ"; break;
	default:
		the_screen_t << title; break;
	}
}


/*****************************************************************************************
 * SCREEN OPENING
 ****************************************************************************************/

struct APP_BASE::SCR_OPEN : public APP_HANDLR_DC {
	static const int CLS_ID = int(APP_BASE::PAGE_ID::PAGE_OPEN);
	int get_class_id() { return CLS_ID; }

	APP_BASE& _app;
	TWE_WidSet_Buttons _btns;

	SCR_OPEN(APP_BASE& app) : _app(app), _btns(*this, app.the_screen), APP_HANDLR_DC(CLS_ID) {}
	~SCR_OPEN() {}

	void show_message() {
		auto& t = _app.the_screen;

		//     "0....+....1....+....2....+....3....+....4....+....5...
		t << "TWELITEから受信した無線ﾊﾟｹｯﾄをｸﾞﾗﾌ表示します。" << crlf
			<< "(App_Wingsを書き込んだ親機に接続します)" << crlf
			<< crlf
			<< "App_Wingsと子機ｱﾌﾟﾘ(TWELITE標準ｱﾌﾟﾘやPALｱﾌﾟﾘ)の設定" << crlf
			<< "(\033[7;41mｱﾌﾟﾘｹｰｼｮﾝID,無線ﾁｬﾈﾙ,暗号化有無･鍵\033[0m)が同じで" << crlf
			<< "ないと無線通信しないようになっています。" << crlf
			<< crlf
			<< "ｲﾝﾀﾗｸﾃｨﾌﾞﾓｰﾄﾞで\033[41;7m親機側(App_Wings)と子機側双方\033[0mの設定を" << crlf
			<< "確認してください。"
			;
	}

	void setup() {
		_app.the_screen.clear_screen();
		_app.the_screen_b.clear_screen();
		_app.set_title_bar(int(PAGE_ID::PAGE_OPEN));

		show_message();

		_btns.add(2, 13, L"加速度ﾘｱﾙﾀｲﾑｸﾞﾗﾌ"
			, [&](int, uint32_t) { _app._tabs.select(int(PAGE_ID::PAGE_CUE_FIFO)); }
			, 0
		);

		_btns.add(2, 15, L"センサーグラフ"
			, [&](int, uint32_t) { _app._tabs.select(int(PAGE_ID::PAGE_WSNS_DB)); }
			, 0
		);
	}

	void loop() {
		_btns.check_events();

		do {
			int c = the_keyboard.read();

			switch (c) {
			case KeyInput::KEY_BUTTON_A:
				break;
			case KeyInput::KEY_BUTTON_B:
				break;
			case KeyInput::KEY_BUTTON_C:
				break;

			default:
				break;
			}

		} while (the_keyboard.available());
	}

	void on_close() {

	}
};

/**
 * create an instance of hander for SCR_GLANCER.
 */
void APP_BASE::hndr_SCR_OPEN(event_type ev, arg_type arg) { hndr<SCR_OPEN>(ev, arg); }