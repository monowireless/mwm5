/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_Graph.hpp"

using APP_BASE = App_Graph;

template<>
const wchar_t* APP_BASE::APP_DESC<APP_BASE>::TITLE_LONG[] = {
	L"グラフ表示 (加速度ﾘｱﾙﾀｲﾑ,ｾﾝｻｰ)",
	L"Graph View (Accl, Sensors)",
};

template<>
const wchar_t* App_Graph::APP_DESC<APP_BASE>::LAUNCH_MSG[] = {
	//....+....1....+....2....+....3....+....4| // 16dots 40cols
	L"TWELITE からの電文を解釈してグラフを" "\r\n"
	L"表示します。" "\r\n"
	L"・加速度ﾘｱﾙﾀｲﾑｸﾞﾗﾌ - CUE/MOTの加速度・" "\r\n"
	L"　周波数領域の表示,CSV出力" "\r\n"
	L"・センサーグラフ   - App_TweLite, PAL," "\r\n"
	L"　CUE, ARIA からのﾃﾞｰﾀをsqlite3ﾃﾞｰﾀﾍﾞｰｽ" "\r\n"
	L"　に保存,日付検索,CSV出力" "\r\n"
	,
	L"Interprets messages from TWELITE and displays graphs." "\r\n"
	L"- Real-time accl view: accl of CUE/MOT, show in freq domain, output in CSV format" "\r\n"
	L"- Sensors view: store messages from App_TweLite, PAL, CUE, ARIA into sqlite3 database, query by data, output in CSV format." "\r\n"
};

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
	the_settings_menu.begin(appid_to_slotid(get_APP_ID()));

	// init the TWE M5 support
	setup_screen(); // initialize TWE M5 support.

	// button navigation
	set_nav_bar();

	// add tab
	for (int i = 0; i < PAGE_ID::_PAGE_END_; i++) {
		switch (i) {
		case PAGE_ID::PAGE_OPEN: _tabs.add(L"---", &App_Graph::hndr_SCR_OPEN); break;
		case PAGE_ID::PAGE_CUE_FIFO: _tabs.add(MLSLW(L"CUEｸﾞﾗﾌ", L"CUE Graph"), &App_Graph::hndr_SCR_CUE_FIFO); break;
		case PAGE_ID::PAGE_WSNS_DB: _tabs.add(MLSLW(L"ｾﾝｻｰｸﾞﾗﾌ", L"Sensor Graph"), &App_Graph::hndr_SCR_WSNS_DB); break;
		case PAGE_ID::PAGE_HELP: _tabs.add(MLSLW(L"解説", L"Info"), &App_Graph::hndr_SCR_HELP); break;
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

		if (c == KeyInput::KEY_ESC || KeyInput::is_mouse_right_up(c)) {
			// handle double ESC/double right click
			static uint32_t t_last;
			uint32_t t_now = millis();

			if (t_now - t_last < STAGE_DOUBLE_ESC_EXIT_TIMEOUT) {
				the_app.exit(get_APP_ID());
			}
			else {
				b_loop = false;
			}
			t_last = t_now;
		}
		//else if (c == KeyInput::KEY_BUTTON_A) { _tabs.select_prev(); }
		//else if (c == KeyInput::KEY_BUTTON_C) { _tabs.select_next(); }
		else if (c == KeyInput::KEY_BUTTON_A_LONG) { the_app.exit(get_APP_ID()); }
		else if (c == KeyInput::KEY_BUTTON_C_LONG) { twe_prog.reset_module(); }
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
	font_IDs.medium = 15;
	font_IDs.tiny = 1;

	TWEFONT::createFontMP10_std(1, 0, 0);

	TWEFONT::createFontShinonome16(10, 0, 0); // normal font

	TWEFONT::createFontShinonome16_full(font_IDs.main, 4, 2); // MP11 font

	TWEFONT::createFontMP10_std(12, 0, 0, TWEFONT::U32_OPT_FONT_YOKOBAI | TWEFONT::U32_OPT_FONT_TATEBAI);

	TWEFONT::createFontShinonome16(13, 0, 0, 0 /* TWEFONT::U32_OPT_FONT_YOKOBAI */);

	TWEFONT::createFontMP12_std(font_IDs.smaller, 0, 0);

	TWEFONT::createFontShinonome14(font_IDs.medium, 0, 1);

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
		the_screen_c << MLSLW(
			L"    ↑/長押:戻る           決定/--                ↓/ﾘｾｯﾄ",
			L"    ↑/Long:BACK         SELECT/--                ↓/RST"
			);
	}
	else {
		the_screen_c << msg;
	}
}

// set title bar
void App_Graph::set_title_bar(int page_id) {
	const char* title = MLSL("\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m グラフ表示\033[0m", "\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m Graph view\033[0m");

	the_screen_t.clear_screen();

	switch (page_id) {
	case PAGE_ID::PAGE_CUE_FIFO:
		the_screen_t << title << MLSLW(L":加速度ﾘｱﾙﾀｲﾑｸﾞﾗﾌ", L":Accel realtime graph"); break;
	case PAGE_ID::PAGE_WSNS_DB:
		the_screen_t << title << MLSLW(L":センサーグラフ", L":Sensor Graph"); break;
	default:
		the_screen_t << title; break;
	}
}

void App_Graph::screen_restore() {
	if (layout.b_set) {
		// full screen clear
		the_screen.set_draw_area(layout.the_screen);
		the_screen.set_font(font_IDs.main);
		the_screen.clear_screen();
		the_screen.force_refresh();

		the_screen_b.set_draw_area(layout.the_screen_b);
		the_screen_b.set_font(font_IDs.smaller);
		the_screen_b.clear_screen();
		the_screen_b.force_refresh();
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
	TWE_ListView _lv;

	SCR_OPEN(APP_BASE& app) : _app(app), _btns(*this, app.the_screen), _lv(16), APP_HANDLR_DC(CLS_ID) {}
	~SCR_OPEN() {}

	void layout_screen() {
		auto& layout = _app.layout;
		auto& the_screen = _app.the_screen;
		auto& the_screen_b = _app.the_screen_b;
		auto& font_IDs = _app.font_IDs;

		Rect r = layout.the_screen;
		Rect r_b = layout.the_screen_b;

		// move boundary by 100pix
		r.h -= 100;
		r_b.h += 100;
		r_b.y -= 100;

		the_screen_b.set_draw_area(r_b);
		the_screen_b.set_font(font_IDs.medium);
		the_screen_b.clear_screen();
		the_screen_b.force_refresh();

		the_screen.set_draw_area(r);
		the_screen.clear_screen();
		the_screen.force_refresh();
	}

	void setup() {
		layout_screen();
		
		_app.set_title_bar(int(PAGE_ID::PAGE_OPEN));
		_app.set_nav_bar();

		// list view
		_lv.attach_term(_app.the_screen, 1, int(PAGE_ID::_PAGE_END_) - 1);
		_lv.set_view();


		for (int i = 0; PAGE_ID(i) < PAGE_ID::_PAGE_END_; i++) {
			switch (PAGE_ID(i)) {
				// case PAGE_ID::PAGE_OPEN: _lv.push_back(L"---", uint16_t(PAGE_ID::PAGE_OPEN)); break;
			case PAGE_ID::PAGE_CUE_FIFO: _lv.push_back(MLSLW(L"加速度ﾘｱﾙﾀｲﾑｸﾞﾗﾌ", L"Accel realtime graph"), uint16_t(PAGE_ID::PAGE_CUE_FIFO)); break;
			case PAGE_ID::PAGE_WSNS_DB: _lv.push_back(MLSLW(L"センサーグラフ", L"Sensor Graph"), uint16_t(PAGE_ID::PAGE_WSNS_DB)); break;
			case PAGE_ID::PAGE_HELP: _lv.push_back(MLSLW(L"解説", L"Info"), uint16_t(PAGE_ID::PAGE_HELP)); break;
			default: break;
			}
		}

		_lv.update_view(true);
	}

	void loop() {
		_btns.check_events();

		do {
			int c = the_keyboard.read();
			if (_lv.key_event(c)) {
				int isel = _lv.get_selected_index();
				auto sel = _lv.get_selected();
				int itab = sel.second[0]; // tab index

				if (isel >= 0 && isel < _lv.size()) {
					if (_lv.is_selection_completed()) {
						// selection
						_app._tabs.select(itab);
					}
					else if (int n_sel = _lv.is_info_selected()) { // 1:primary 2:secondary
						// select sub item
					}
					else {
						// over
						auto& t = _app.the_screen_b;
						t.clear_screen();
						t << "\033[32m";

						switch (PAGE_ID(itab)) {
						case PAGE_ID::PAGE_CUE_FIFO:
							t << MLSLW(
								L"加速度センサー(TWELITE CUE, PAL MOT)のFIFOモードのデータをリアルタイム表示します。",
								L"Real-time display of FIFO mode data from acceleration sensors (TWELITE CUE, PAL MOT)."
							);
							t << crlf << MLSLW(
								L"画面上では周波数領域のグラフも同時に表示します（パケット欠落のない連続した一定数のサンプルが必要です）。また、CSVファイルへの出力も可能です。",
								L"A graph in the frequency domain is simultaneously displayed on the screen (a certain number of consecutive samples without missing packets is required). The data can also be exported to a CSV file."
							);
							break;
						case PAGE_ID::PAGE_WSNS_DB:
							t << MLSLW(
								L"各種TWELITE (App_Twelite, PAL-AMB, PAL-MOT, PAL-MAG, CUE, ARIA) からのパケットを sqlite3 データベースに保存し、グラフとして表示することが可能です。",
								L"Packets from various TWELITEs (App_Twelite, PAL-AMB, PAL-MOT, PAL-MAG, CUE, ARIA) can be stored in a sqlite3 database and displayed as a graph.");
							t << crlf << MLSLW(
								L"画面上では、過去の1日分のグラフを表示（ﾎｲｰﾙによる拡大表示も可）、リアルタイム表示、CSV出力が可能です。",
								L"On the screen, graphs for the past day can be displayed (zoomed in using the wheel), displayed in real time, and exported to CSV.");
							break;
						case PAGE_ID::PAGE_HELP:
							t << MLSLW(
								L"簡易的な解説です。より詳細は各タブ画面中のヘルプリンクから情報を得てください。",
								L"This is a simplified explanation. For more detailed information, please refer to the help links in each tab.");
							break;
						default: break;
						}

						t << "\033[0m";
					}
				}
			}
			else switch (c) {
			case KeyInput::KEY_BUTTON_A:
				the_keyboard.push(KeyInput::KEY_UP);
				break;
			case KeyInput::KEY_BUTTON_B:
				the_keyboard.push(KeyInput::KEY_ENTER);
				break;
			case KeyInput::KEY_BUTTON_C:
				the_keyboard.push(KeyInput::KEY_DOWN);
				break;

			default:
				break;
			}

		} while (the_keyboard.available());
	}

	void on_close() {
		_app.screen_restore();
	}
};

/**
 * create an instance of hander for SCR_GLANCER.
 */
void APP_BASE::hndr_SCR_OPEN(event_type ev, arg_type arg) { hndr<SCR_OPEN>(ev, arg); }