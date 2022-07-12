/* Copyright (C) 2019-2022 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_Glancer.hpp"

#include <algorithm>
#ifdef min
# undef min
#endif

using APP_BASE = App_Glancer; //! alias to parent class

template<>
const wchar_t* APP_BASE::APP_DESC<APP_BASE>::TITLE_LONG[] = {
	L"簡易確認アプリ (CUE/ARIA/Glancer)",
	L"Simple Confirmation App (CUE/ARIA/Glancer)",
};

template<>
const wchar_t* APP_BASE::APP_DESC<APP_BASE>::LAUNCH_MSG[] = {
	//....+....1....+....2....+....3....+....4| // 16dots 40cols
	L"無線パケットの到着を簡易的に確認します。" "\r\n"
	L"CUE -> CUE のﾊﾟｹｯﾄを解釈します。" "\r\n"
	L"CUE -> ARIA のﾊﾟｹｯﾄを解釈します。" "\r\n"
	L"Glancer -> 多くのﾊﾟｹｯﾄを簡易リスト""\r\n"
	,
	L"Simplified confirmation of wireless packet arrival." "\r\n"
	L"CUE -> Interprets CUE packets." "\r\n"
	L"CUE -> Interprets ARIA packets." "\r\n"
	L"Glancer -> List many kinds of packets from TWELITE.""\r\n"
};

/*****************************************************************************************
 * App_Glancer
 ****************************************************************************************/

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

void APP_BASE::setup() {
	// preference
	the_settings_menu.begin(appid_to_slotid(get_APP_ID()));

	// init the TWE M5 support
	setup_screen(); // initialize TWE M5 support.

	// save layout
	layout.the_screen = the_screen.get_draw_area();
	layout.the_screen_b = the_screen_b.get_draw_area();
	layout.b_saved = true;

	// add tab item by page id order.
	for (int i = 0; PAGE_ID(i) < PAGE_ID::_PAGE_END_; i++) {
		switch (PAGE_ID(i)) {
		case PAGE_ID::PAGE_OPEN: _tabs.add(L"---", &APP_BASE::hndr_SCR_OPEN); break;
		case PAGE_ID::PAGE_SCR_CUE: _tabs.add(L"CUE", &APP_BASE::hndr_SCR_CUE_BASIC); break;
		case PAGE_ID::PAGE_SCR_ARIA: _tabs.add(L"ARIA", &APP_BASE::hndr_SCR_ARIA_BASIC); break;
		case PAGE_ID::PAGE_SCR_GLANCER: _tabs.add(L"Glancer", &APP_BASE::hndr_SCR_GLANCER); break;
		default: break;
		}
	}
	_tabs.update_view();

	if (u8tab_selection < _tabs.size()) {
		_tabs.select(u8tab_selection);
	}

	// put a init message
	set_title_bar(int(PAGE_ID::PAGE_SCR_CUE));
}

void APP_BASE::loop() {
	// tab event handling
	_tabs.check_events();

	// peek keyevent and perform common event handling.
	// if the event is not for this controls, APP_HNDLR will take care of.
	bool b_loop = true;
	do {
		int c = the_keyboard.peek_a_byte();

		if (c == KeyInput::KEY_BUTTON_A) {
			the_keyboard.push(KeyInput::KEY_UP);
		}
		else if (c == KeyInput::KEY_BUTTON_B) {
			the_keyboard.push(KeyInput::KEY_ENTER);
		}
		else if (c == KeyInput::KEY_BUTTON_C) {
			the_keyboard.push(KeyInput::KEY_DOWN);
		}
		else if (c == KeyInput::KEY_ESC || KeyInput::is_mouse_right_up(c)) {
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
		else if (c == KeyInput::KEY_BUTTON_A_LONG) {
			the_app.exit(get_APP_ID());
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

void APP_BASE::setup_screen() {
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

	TWEFONT::createFontMP10_std(font_IDs.tiny, 0, 0);

	TWEFONT::createFontShinonome16(10, 0, 0); // normal font

	TWEFONT::createFontShinonome16_full(font_IDs.main, 4, 2); // MP11 font

	TWEFONT::createFontMP10_std(12, 0, 0, TWEFONT::U32_OPT_FONT_YOKOBAI | TWEFONT::U32_OPT_FONT_TATEBAI);

	TWEFONT::createFontShinonome16(13, 0, 0, 0 /* TWEFONT::U32_OPT_FONT_YOKOBAI */);

	TWEFONT::createFontMP12_std(font_IDs.smaller, 0, 0);

	TWEFONT::createFontShinonome14(font_IDs.medium, 1, 0);

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

// set title bar
void APP_BASE::set_title_bar(int page_id) {
	const char* title = "\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m ";
	const wchar_t* title_2 = MLSLW(L"簡易モニタ\033[0m", L"Simple Monitor\033[0m");

	the_screen_t.clear_screen();

	switch (PAGE_ID(page_id)) {
	case PAGE_ID::PAGE_OPEN:
		the_screen_t << title << title_2 << ": ---"; break;
	case PAGE_ID::PAGE_SCR_CUE:
		the_screen_t << title << title_2 << MLSLW(L": CUEパケット確認", L": CUE Packet Mon"); break;
	case PAGE_ID::PAGE_SCR_ARIA:
		the_screen_t << title << title_2 << MLSLW(L": ARIAパケット確認", L": ARIA Packet Mon"); break;
	case PAGE_ID::PAGE_SCR_GLANCER:
		the_screen_t << title << title_2 << MLSLW(L": Glancer ｸﾞﾗﾝｻｰ", L": Glancer"); break;
	default:
		the_screen_t << title << title_2; break;
	}
}

void APP_BASE::screen_refresh() {
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

void APP_BASE::screen_layout_opening() {
	if (layout.b_saved) {
		// full screen clear
		the_screen.set_draw_area(layout.the_screen);
		the_screen_b.set_draw_area(layout.the_screen_b);
		the_screen_b.set_font(font_IDs.medium);
		the_screen.clear_screen();
		the_screen_b.clear_screen();
		the_screen.force_refresh();
		the_screen_b.force_refresh();
	}
}

void APP_BASE::screen_layout_apps() {
	Rect r = layout.the_screen;
	Rect r_b = layout.the_screen_b;

	// move boundary by 100pix
	r.h += 100;
	r_b.h -= 100;
	r_b.y += 100;

	the_screen_b.set_draw_area(r_b);
	the_screen_b.set_font(font_IDs.smaller);
	the_screen_b.clear_screen();
	the_screen_b.force_refresh();

	the_screen.set_draw_area(r);
	the_screen.clear_screen();
	the_screen.force_refresh();
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

	SCR_OPEN(APP_BASE& app) : _app(app), _lv(8), _btns(*this, app.the_screen), APP_HANDLR_DC(CLS_ID) {}
	virtual ~SCR_OPEN() {}

	void setup() {
		_app.screen_layout_opening();

		_app.the_screen.clear_screen();
		_app.the_screen_b.clear_screen();
		_app.set_title_bar(int(PAGE_ID::PAGE_OPEN));

		auto& t = _app.the_screen_c; t.clear_screen();
		//    "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		t << MLSLW(L"    ↑/長押:戻る           選択/--                ↓/ﾘｾｯﾄ",
			       L"    ↑/Long:BACK         SELECT/--                ↓/RST"
			);

		// list view
		_lv.attach_term(_app.the_screen, 1, int(PAGE_ID::_PAGE_END_) - 1);
		_lv.set_view();

		for (int i = 0; PAGE_ID(i) < PAGE_ID::_PAGE_END_; i++) {
			switch (PAGE_ID(i)) {
			// case PAGE_ID::PAGE_OPEN: _lv.push_back(L"---", uint16_t(PAGE_ID::PAGE_OPEN)); break;
			case PAGE_ID::PAGE_SCR_CUE: _lv.push_back(MLSLW(L"TWELITE CUE の簡易モニタ", L"Simple Monitor for TWELITE CUE"), uint16_t(PAGE_ID::PAGE_SCR_CUE)); break;
			case PAGE_ID::PAGE_SCR_ARIA: _lv.push_back(MLSLW(L"TWELITE ARIA の簡易モニタ", L"Simple Monitor for TWELITE ARIA"), uint16_t(PAGE_ID::PAGE_SCR_ARIA)); break;
			case PAGE_ID::PAGE_SCR_GLANCER: _lv.push_back(MLSLW(L"Glancer グランサー", L"Glancer"), uint16_t(PAGE_ID::PAGE_SCR_GLANCER)); break;
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
						case PAGE_ID::PAGE_SCR_CUE: 
							t << MLSLW(
								L"TWELITE CUE の無線パケットの情報を表示します。TWELITE CUE には加速度情報のほかに動作などのイベント情報が含まれます。インタラクティブモードの設定によっては情報が出ない場合もあります。",
								L"Displays information on TWELITE CUE radio packets; TWELITE CUE includes acceleration information as well as motion and other event information. Depending on the interactive mode setting, some information may not be displayed."
							);
							t << crlf << MLSLW(
								L"(連続的に加速度情報を得るFIFOモードでは、別ページにあるグラフ機能を用いることで、より詳細な情報を得ることができます。)", 
								L"(In FIFO mode, where acceleration information is obtained continuously, more detailed information can be obtained by using the graph function on a separate page.)"
							);
						break;
						case PAGE_ID::PAGE_SCR_ARIA:
							t << MLSLW(
								L"TWELITE ARIA の無線パケットの情報を表示します。到着順に温湿度情報をリスト表示するのみの単純なものです。",
								L"Displays information on TWELITE ARIA radio packets. It is simple enough to list temperature and humidity information in order of arrival."
							);
							t << crlf << MLSLW(
								L"(別ページのセンサーグラフ機能を用いることで、センサー情報の記録とグラフ表示を行えます)",
								L"(The sensor information can be recorded and displayed in a graph by using the sensor graph function on a separate page.)"
							);
						break;
						case PAGE_ID::PAGE_SCR_GLANCER:
							t << MLSLW(
								L"TWELITEの多くの種別の無線パケット(App_Twelite, PAL各種など)をリスト表示します。表示内容はアドレスやLQIなど最小限です。",
								L"Displays a list of many types of TWELITE radio packets (App_Twelite, various PALs, etc.). Displayed content is minimal, like addresses and LQI."
							);
						break;
						default: break;
						}

						t << "\033[0m";
					}
				}
			} 
			else switch (c) {
			case KeyInput::KEY_BUTTON_A: the_keyboard.push(KeyInput::KEY_UP); break;
			case KeyInput::KEY_BUTTON_B: the_keyboard.push(KeyInput::KEY_ENTER); break;
			case KeyInput::KEY_BUTTON_C: the_keyboard.push(KeyInput::KEY_DOWN); break;

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

/*****************************************************************************************
 * SCREEN GLANCER
 ****************************************************************************************/

// App_CUE::SCR_CUE_BASIC : public APP_HANDLR_DC
struct APP_BASE::SCR_GLANCER : public APP_HANDLR_DC {
public:
	static const int CLS_ID = int(APP_BASE::PAGE_ID::PAGE_SCR_GLANCER);
	
	static const int _SORT_KEYS_COUNT = 5;

public:
	typedef TWEUTILS::SimpleBuffer<spTwePacket> pkt_ary;
	/**
	 * @struct	pkt_data
	 *
	 * @brief	PAL data management by ID and display them.
	 */
	struct pkt_data_and_view {
		pkt_ary _dat;
		FixedQueue<spTwePacket> _dat_solo;

		int _page;      // start from 0
		int _lines;     // lines to display entry
		int _max_entry; // max ID (1...max)
		bool _bwide;    // if screen col >= 38, set true
		bool _bdirty;   // if update all is required

		int _nsel;		// selected entry (1..max_entry)

		bool _bsolo;	// solo display mode

		int _sort_key;  // 0:SID, 1:LID, 2:LQI, 3:VOLT 4:TIME

		struct {
			uint32_t src_addr;
			uint32_t n_packets;

			void init() {
				src_addr = 0;
				n_packets = 0;
			}
		} _solo_info;

		ITerm& _trm;           // screen for main content
		ITerm& _trm_status;    // status screen
		const char* _fmt_status; // status message template

		pkt_data_and_view(ITerm& trm, ITerm& trm_status)
			: _dat(8) // initial reserve
			, _dat_solo(32)
			, _page(0)
			, _lines(0)
			, _max_entry(0)
			, _bwide(false), _bdirty(false)
			, _trm(trm)
			, _trm_status(trm_status)
			, _fmt_status(nullptr)
			, _bsolo(false)
			, _nsel(0)
			, _solo_info()
			, _sort_key(0)
		{}

		// screen init
		void init_screen(const char* fmt_status) {
			_fmt_status = fmt_status;
			reinit_screen();
		}

		// re-init screen
		void reinit_screen();

		// add coming packet entry
		bool add_entry(spTwePacket spobj);

		// show next page
		void next_page() {
			_page++;

			if (_page * _lines + 1 > _max_entry) {
				_page = 0; // switch to the top
			}

			if (_nsel < _page * _lines || _nsel >= _page * _lines + _lines) {
				_nsel = _page * _lines;
			}
		}

		// show prev page
		void prev_page() {
			_page--;

			if (_page < 0) {
				_page = (_max_entry - 1) / _lines;
			}

			if (_nsel < _page * _lines || _nsel >= _page * _lines + _lines) {
				_nsel = _page * _lines;
			}
		}

		// set page
		void set_page(int entry) {
			if (entry > 0 && entry <= _max_entry) {
				_page = (entry - 1) / _lines;
			}
		}

		void next_item() {
			++_nsel;

			if (_nsel > _max_entry) {
				_nsel = 1;
			}

			if (_nsel <= _page * _lines || _nsel >= _page * _lines + _lines) {
				set_page(_nsel);
			}
		}

		void prev_item() {
			--_nsel;

			if (_nsel < 1) {
				_nsel = _max_entry;
			}

			if (_nsel <= _page * _lines || _nsel >= _page * _lines + _lines) {
				set_page(_nsel);
			}
		}

		// update status string
		void update_status();

		// redraw whole screen
		void update_term() {
			update_term(spTwePacketPal(), true);
		}

		// update screen
		void update_term(spTwePacket pal_upd, bool update_all) {
			if (_bsolo) update_term_solo(pal_upd, update_all);
			else update_term_full(pal_upd, update_all);
		}
		void update_term_full(spTwePacket pal_upd, bool update_all);
		void update_term_solo(spTwePacket pal_upd, bool update_all);

		// sort the lists
		void sort_entries();

		// solo mode
		void enter_solo_mode();

		// print obj line
		void print_obj(spTwePacket& spobj);
	};

private:
	// Serial Parser
	AsciiParser parse_ascii;

	// default color
	uint16_t default_bg_color;
	uint16_t default_fg_color;

	// packet data management and display
	pkt_data_and_view pkt_data;

	// timeout to hold screen_b
	TWESYS::TimeOut _timeout_hold_screen_b;
	bool _b_hold_screen_b;

	// base app.
	APP_BASE& _base;

	// top
	TWETerm_M5_Console& the_screen_t; // init the screen.

	// main screen
	TWETerm_M5_Console& the_screen; // init the screen.

	// bottom area (for debug information)
	TWETerm_M5_Console& the_screen_b; // init the screen.

	// bottom area (for debug information)
	TWETerm_M5_Console& the_screen_c; // init the screen.


public:
	SCR_GLANCER(APP_BASE& app)
		: APP_HANDLR_DC(CLS_ID)
		, _base(app)
		, the_screen_t(app.the_screen_t)
		, the_screen(app.the_screen)
		, the_screen_b(app.the_screen_b)
		, the_screen_c(app.the_screen_c)
		, parse_ascii(256)
		, default_bg_color(0)
		, default_fg_color(0)
		, pkt_data(app.the_screen, app.the_screen_t)
		, _timeout_hold_screen_b()
		, _b_hold_screen_b(false)
	{
	}

	virtual ~SCR_GLANCER() {}

	void setup();

	void loop();

	void on_close() {}

private:
	void parse_a_byte(char_t u8b);
	void process_input();
	void check_for_serial();

	// set navigation bar on the bottom
	void set_nav_bar();
};

/**
 * create an instance of hander for SCR_GLANCER.
 */
void APP_BASE::hndr_SCR_GLANCER(event_type ev, arg_type arg) { hndr<SCR_GLANCER>(ev, arg); }


void APP_BASE::SCR_GLANCER::setup() {
	// preference
	the_settings_menu.begin(appid_to_slotid(_base.get_APP_ID()));
	
	// put a init message
	const char* fmt_title = MLSL(
		"\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m 簡易モニタ: ｸﾞﾗﾝｻｰ\033[0m : %s", 
		"\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m Simple Mon: Glancer\033[0m : %s");
	the_screen_t << printfmt(fmt_title, "---"); // accepts UTF-8 codes
	pkt_data.init_screen(fmt_title);

	// change screen size
	_base.screen_layout_apps();

	//     "0....+....1....+....2....+....3....+....4....+....5..
	the_screen(0, 6)
		<< MLSLW(L"  -- TWELITE Wings で受信したパケット情報を表示します --",
			     L"-- Displays packet information received in TWELITE Wings --")
		<< crlf
		;

	// button navigation
	set_nav_bar();
}


// button navigation
void APP_BASE::SCR_GLANCER::set_nav_bar() {
	the_screen_c.clear_screen();

	if (pkt_data._bsolo) {
		//e_s"....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << MLSLW(
			L"     --/長押:戻る            --/--                --/ﾘｾｯﾄ",
			L"     --/Long:BACK            --/--                --/RST"
		);

	}
	else {
		//e_s"....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << MLSLW(
			L"     ↑/長押:戻る          決定/ｿｰﾄ               ↓/ﾘｾｯﾄ",
			L"     ↑/Long:BACK         ELECT/SORT              ↓/RST"
		);
	}
}

void APP_BASE::SCR_GLANCER::loop() {
	// reset the hold screen flag of screen_b 
	if (_timeout_hold_screen_b.available()) _b_hold_screen_b = false;

	// KEYBOARD
	while (the_keyboard.available()) {
		switch (int c = the_keyboard.read()) {

		case KeyInput::KEY_BUTTON_A_LONG:
		case KeyInput::KEY_ESC:
			if (pkt_data._bsolo) {
				// if going into single node list, go back to nodes list.
				pkt_data._bsolo = false;
				pkt_data._dat_solo.clear();
				
				pkt_data.update_term();
				set_nav_bar();
			} else {
				// the_app.exit(APP_BASE::APP_ID); // APP EXIT is handled by APP_BASE.
				return;
			}
			break;

		case KeyInput::KEY_BUTTON_B:
		case KeyInput::KEY_ENTER:
			if (!pkt_data._bsolo) {
				pkt_data.enter_solo_mode();
				set_nav_bar();
			}
			break;

		case KeyInput::KEY_BUTTON_A:
		case KeyInput::KEY_UP:
			pkt_data.prev_item();
			pkt_data.update_term();
			break;
			
		case KeyInput::KEY_BUTTON_C:
		case KeyInput::KEY_DOWN:
			pkt_data.next_item();
			pkt_data.update_term();
			break;
			
		case KeyInput::KEY_LEFT:
			// previous page
			pkt_data.prev_page();
			pkt_data.update_term();
			break;

#if 0
		case KeyInput::KEY_BUTTON_B:
			// chenge font
			change_screen_font();

			pkt_data.reinit_screen();
			pkt_data.update_term();
			break;
#endif

		case KeyInput::KEY_BUTTON_B_LONG:
			{
				int sort_key = pkt_data._sort_key;

				const char str_srt_key[_SORT_KEYS_COUNT][TWE::LANG_CT][32] = {
					{ "論理ID", "LID" },
					{ "シリアル番号", "SerialNo" },
					{ "リンク品質 Lq", "Link Quality" },
					{ "電圧 mV", "Vcc mV" },
					{ "時間 s", "Time s" }
				};
				// sort entries
				pkt_data.sort_entries();
				pkt_data.update_term();

				the_screen_b.clear_screen();
				the_screen_b << printfmt(
					MLSL("\033[30;33m[%s]でソートしました\033[0m", "\033[30;33mSorted by [%s]\033[0m"),
					str_srt_key[sort_key][g_lang]);

				// keep this message for 3sec
				_b_hold_screen_b = true;
				_timeout_hold_screen_b.start(3000);
			}
			break;

		case KeyInput::KEY_RIGHT:
			// next page
			pkt_data.next_page();
			pkt_data.update_term();
			break;

		case KeyInput::KEY_BUTTON_C_LONG:
			twe_prog.reset_module();
			break;

		default:
			// put input data to uart queue (for debugging)
			if (c >= 0x20 && c <= 0x7E) {
				the_uart_queue.push(uint8_t(c));
			}
			break;
		}
	}

	// check for serial input
	check_for_serial();

	// process input data
	process_input();
}

// put a byte to parse
void APP_BASE::SCR_GLANCER::parse_a_byte(char_t u8b) {
	// parse.
	parse_ascii << u8b;

	// if complete parsing
	if (parse_ascii) {
		// output as parser format
		// the_screen_b << parse_ascii;
		if (!_b_hold_screen_b) the_screen_b.clear_screen();
		static int ct;
		if (!_b_hold_screen_b) the_screen_b << "PKT(" << ct++ << ')';

		// EEPROM TEST
		auto&& p = parse_ascii.get_payload();

		// 1. identify the packet type
		auto&& pkt = newTwePacket(parse_ascii.get_payload());
		if (!_b_hold_screen_b) the_screen_b << ":Typ=" << int(identify_packet_type(pkt));

		if (identify_packet_type(pkt) != E_PKT::PKT_ERROR) {
			// put information
			if (!_b_hold_screen_b) the_screen_b
					<< printfmt(":Lq=%d:Ad=%08X(%02X),Tms=%d"
						, pkt->common.lqi, pkt->common.src_addr, pkt->common.src_lid, pkt->common.tick);

			// store data into `pal_data'
			if (pkt_data.add_entry(pkt)) {
				// update screen.
				pkt_data.update_term(pkt, false);
			}
		}
	}
}

// process input
void APP_BASE::SCR_GLANCER::process_input() {
	int c;

	// from TWE
	while (-1 != (c = the_uart_queue.read())) {
		// pass them to M5 (normal packet analysis)
		parse_a_byte(char_t(c));
	}
}

// check serial input.
void APP_BASE::SCR_GLANCER::check_for_serial() {
	// UART CHECK is DONE at ::loop() procedure.
	;
}

// Packet name table
typedef struct {
	E_PKT id;
	const wchar_t name[4];
} ts_PktID_to_Name3;

// 3 Chars label of packet_type (shall follow the order of E_PKT.
const static ts_PktID_to_Name3 asPktIdToName3[] {
	{ E_PKT::PKT_ERROR,		L"N/A" },
	{ E_PKT::PKT_TWELITE,	L"TWE" },
	{ E_PKT::PKT_PAL,   	L"PAL" },
	{ E_PKT::PKT_APPIO, 	L"IO " },
	{ E_PKT::PKT_APPUART,	L"URT" },
	{ E_PKT::PKT_APPTAG,	L"TAG" },
	{ E_PKT::PKT_ACT_STD,	L"ACT" },
};

static const int E_PAL_DERIVED_EVT = 0;
static const int E_PAL_DERIVED_CUE = 1;
static const int E_PAL_DERIVED_ARIA = 2;
static const wchar_t wstr_Name3_PALderived[][4]{
	L"EVT",
	L"CUE",
	L"ARA"
};

/**
 * @fn	void update_term_full(spTwePacketPal pal_upd, bool update_all)
 *
 * @brief	Updates the term full list
 *
 * @param	pal_upd   	Coming packet (should be updated in the display)
 * @param	update_all	True to update all lines, other than pal_upd.
 */
void APP_BASE::SCR_GLANCER::pkt_data_and_view::update_term_full(spTwePacket pal_upd, bool update_all) {
	int idx_start = _page * _lines + 1;
	int idx_end = idx_start + _lines;

	// when max entry count has been changed, this flag is set.
	if (_bdirty) {
		update_all = true;
		_bdirty = false;
	}

	if (update_all) {
		// update all
		_trm << "\033[2J\033[H"; // clear screen (by ESC seq)
	}

	// displays each sensor data.
	for (int i = idx_start; i < idx_end && i <= int(_dat.size()); i++) {
		auto&& spobj = _dat[i - 1];

		// show IDs at head.
		if (update_all || (spobj && spobj == pal_upd)) {
			_trm << printfmt("\033[%d;1H\033[K", i - idx_start + 1) // move cursor and clear the line
				<< printfmt("%2d:", i); // ID:
		}

		// skip when the corresponding ID is not updated.
		if (!spobj || ((update_all == false) && spobj != pal_upd)) continue; // not updated

		// display sensor data
		if (identify_packet_type(spobj) != E_PKT::PKT_ERROR) {
			if (_nsel == i) _trm << "\033[7m";
			print_obj(spobj);
			if (_nsel == i) _trm << "\033[0m";
		}
		else {
			_trm << "---";
		}

		_trm << "\033[4G"; // move cursor at column 2.
	}

	// update status line
	update_status();
}

void APP_BASE::SCR_GLANCER::pkt_data_and_view::print_obj(spTwePacket& spobj) {
	// display sensor data
	E_PKT pkt_type = identify_packet_type(spobj);
	if (pkt_type != E_PKT::PKT_ERROR) {
		
		const wchar_t* pAppName = nullptr;
		bool b_pal_event = false;
		uint8_t u8event_id = 0;

		for (auto& x : asPktIdToName3) {
			if (x.id == spobj->get_type()) {
				pAppName = x.name;
				break;
			}
		}

		if (pkt_type == E_PKT::PKT_PAL) {
			// PAL Packet
			auto&& pal = refTwePacketPal(spobj);

			if (pal.is_PalEvent()) {
				u8event_id = pal.get_PalEvent().u8event_id;
			}

			// change 3chars label (EVT, CUE)
			switch (pal.get_PalDataType()) {
			case E_PAL_DATA_TYPE::EVENT_ONLY: pAppName = wstr_Name3_PALderived[E_PAL_DERIVED_EVT]; break;
			case E_PAL_DATA_TYPE::EX_CUE_STD: pAppName = wstr_Name3_PALderived[E_PAL_DERIVED_CUE]; break;
			case E_PAL_DATA_TYPE::EX_ARIA_STD: pAppName = wstr_Name3_PALderived[E_PAL_DERIVED_ARIA]; break;
			default: break;
			}
		}

		// display packet type
		if (b_pal_event) {
			// PAL event
			_trm << printfmt("!%02X", u8event_id);
		} else {
			// others
			if (pAppName == nullptr) pAppName = L"N/A";
			else _trm << pAppName;
		}

		_trm << printfmt("%3d/x%08X %3dLq %4dmV"
			, spobj->common.src_lid
			, spobj->common.src_addr
			, spobj->common.lqi
			, spobj->common.volt
		);

		uint32_t t = spobj->common.tick + 50;
		t = t % (10000 * 1000); // loop at 9999.9sec

		_trm << printfmt(" %4d.%ds", t / 1000, (t % 100) / 10);
	}
}

void APP_BASE::SCR_GLANCER::pkt_data_and_view::update_term_solo(spTwePacket pal_upd, bool update_all) {
	update_all = true; // always update everything

	if (update_all) {
		// update all
		_trm << "\033[2J\033[H"; // clear screen (by ESC seq)
	}

	// displays each sensor data.
	if (_dat_solo.size() > 0) {
		if (update_all || (pal_upd && pal_upd->common.src_addr == _solo_info.src_addr)) {
			// move cursor at home position
			_trm << "\033[H";

			int i_end = std::min(int(_dat_solo.size()), int(_lines - 1));
			for (int i = i_end; i > 0; i--) {
				auto& spobj = _dat_solo[-i];

				print_obj(spobj);

				_trm << crlf;
			}

			// check lqi average
			int lqav = 0;
			for (int i = 0; i < _dat_solo.size(); i++) lqav += _dat_solo[i]->common.lqi;
			lqav /= _dat_solo.size();

			_trm << printfmt("\033[%d;1H", _trm.get_rows());
			_trm << "\033[7m\033[K\033[G"; // clear the line
			if(_bwide) _trm << printfmt("Total=%dpkt LQav=%d(of%d)", _solo_info.n_packets, lqav, _dat_solo.size());
			else       _trm << printfmt("%dpkt LQav=%d", _solo_info.n_packets, lqav);
			_trm << printfmt("\033[0m\033[%d;1H", i_end); // move the cursor at the latest item.
		}
	}

	// update status line
	update_status();
}


/**
 * @fn	void update_status()
 *
 * @brief	Updates the status string.
 *
 */
void APP_BASE::SCR_GLANCER::pkt_data_and_view::update_status() {
	char buff[16];

	if (_bsolo) {
		_trm_status.clear_screen();
		_trm_status << printfmt(_fmt_status, "---");
	} else {
		int max_page = (_max_entry - 1) / _lines;
		snPrintf(buff, sizeof(buff), "[%d/%d]", _page + 1, max_page + 1);

		_trm_status.clear_screen();
		_trm_status << printfmt(_fmt_status, buff);
	}
}

/**
 * @fn	void reinit_screen()
 *
 * @brief	Reinitializes the screen, in case the screen size has been changed.
 *
 */
void APP_BASE::SCR_GLANCER::pkt_data_and_view::reinit_screen() {
	int scrw = _trm.get_cols();
	int scrh = _trm.get_rows();

	_page = 0;
	_lines = scrh;
	_bwide = (scrw >= 38);

	_bdirty = true;

	_trm << "\033[2J\033[H"; // clear screen (by ESC seq)
	update_status();
}

/**
 * @fn	bool add_entry(spTwePacketPal pal)
 *
 * @brief	Adds an entry of PAL packet.
 *
 * @param	pal PAL object to be added.
 *
 * @returns	True if it succeeds, false if it fails.
 */
bool APP_BASE::SCR_GLANCER::pkt_data_and_view::add_entry(spTwePacket spobj) {
	if (spobj
		&& identify_packet_type(spobj) != E_PKT::PKT_ERROR
		&& spobj->common.src_addr & 0x80000000
	) {
		bool b_found = false;
		for (int i = 0; i < _max_entry; i++) {
			if (spobj->common.src_addr == _dat[i]->common.src_addr) {
				b_found = true;
				_dat[i] = spobj;
				break;
			}
		}

		if (!b_found) {
			_dat.push_back(spobj);
			_max_entry = _dat.size();

			b_found = true;
		}

		if (b_found) {
			if (_bsolo) {
				// if solo mode, check the coming packt's id with monitoring id and push it.
				if (!_dat_solo.empty() && _dat_solo.front()->common.src_addr == spobj->common.src_addr) {
					if (_dat_solo.is_full()) _dat_solo.pop();
					_solo_info.n_packets++;
					_dat_solo.push(spobj);

					return true;
				}

				return false;
			}

			return true;
		}
	}

	return false;
}

void APP_BASE::SCR_GLANCER::pkt_data_and_view::sort_entries() {
	switch (_sort_key) {
	case 0: // LID
		SmplBuf_Sort(_dat,
			[](spTwePacket& a, spTwePacket& b) {
				return a->common.src_lid > b->common.src_lid;
			}
		);
		break;

	case 1: // SID
		SmplBuf_Sort(_dat,
			[](spTwePacket& a, spTwePacket& b) {
				return a->common.src_addr > b->common.src_addr;
			}
		);
		break;

	case 2: // LQI
		SmplBuf_Sort(_dat,
			[](spTwePacket& a, spTwePacket& b) {
				return a->common.lqi < b->common.lqi; // bigger first
			}
		);
		break;

	case 3: // Volt 
		SmplBuf_Sort(_dat,
			[](spTwePacket& a, spTwePacket& b) {
				return a->common.volt > b->common.volt;
			}
		);
		break;

	case 4: // Time
		SmplBuf_Sort(_dat,
			[](spTwePacket& a, spTwePacket& b) {
				return a->common.tick < b->common.tick; // bigger first
			}
		);
		break;
	}

	if (++_sort_key >= _SORT_KEYS_COUNT) _sort_key = 0;
}

void APP_BASE::SCR_GLANCER::pkt_data_and_view::enter_solo_mode() {
	if (_nsel > 0 && _nsel <= int(_dat.size())) {
		
		auto obj = _dat[_nsel - 1];

		if (obj) {
			_bsolo = true;
			_dat_solo.clear();
			_solo_info.init();

			_dat_solo.push(obj);

			_solo_info.src_addr = obj->common.src_addr;
			_solo_info.n_packets = 1;

			update_term();
		}
	}
}
