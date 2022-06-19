/* Copyright (C) 2019-2022 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_Glancer.hpp"

#include <algorithm>
#ifdef min
# undef min
#endif

using APP_BASE = App_Glancer; //! alias to parent class


const wchar_t APP_BASE::LAUNCH_MSG[] =
//....+....1....+....2....+....3....+....4| // 16dots 40cols
L"            \033[4m"
L"簡易確認アプリ\033[0m"
L"          ""\r\n"
L"\r\n"
L"無線パケットの到着を簡易的に確認します。" "\r\n"
L"CUE -> 標準形式のﾊﾟｹｯﾄを解釈します。" "\r\n"
L"Glancer -> 多くのﾊﾟｹｯﾄを簡易リスト""\r\n"
;

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
	the_settings_menu.begin(appid_to_slotid(APP_ID));

	// init the TWE M5 support
	setup_screen(); // initialize TWE M5 support.

	// button navigation
	set_nav_bar();

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

void APP_BASE::set_nav_bar(const char* msg) {
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
void APP_BASE::set_title_bar(int page_id) {
	const char* title = "\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m CUE/ARIAﾋﾞｭｰｱ\033[0m";

	the_screen_t.clear_screen();

	switch (PAGE_ID(page_id)) {
	case PAGE_ID::PAGE_OPEN:
		the_screen_t << title << ": ---"; break;
	case PAGE_ID::PAGE_SCR_CUE:
		the_screen_t << title << ": CUEパケット確認"; break;
	case PAGE_ID::PAGE_SCR_GLANCER:
		the_screen_t << title << ": Glancer ｸﾞﾗﾝｻｰ"; break;
	default:
		the_screen_t << title; break;
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


/*****************************************************************************************
 * SCREEN OPENING
 ****************************************************************************************/

struct APP_BASE::SCR_OPEN : public APP_HANDLR_DC {
	static const int CLS_ID = int(APP_BASE::PAGE_ID::PAGE_OPEN);
	int get_class_id() { return CLS_ID; }

	APP_BASE& _app;
	TWE_WidSet_Buttons _btns;

	SCR_OPEN(APP_BASE& app) : _app(app), _btns(*this, app.the_screen), APP_HANDLR_DC(CLS_ID) {}
	virtual ~SCR_OPEN() {}

	void show_message() {
		auto& t = _app.the_screen;

		//     "0....+....1....+....2....+....3....+....4....+....5...
		t << "TWELITE の無線パケットを簡易確認します。 " << crlf
			<< "(App_Wingsを書き込んだ親機に接続します)" << crlf
			<< crlf
			<< "App_Wingsは様々な種類のTWELITE Apps無線ﾊﾟｹｯﾄに対応し" << crlf
			<< "ます。親子間の通信設定が一致しないと受信できません。" << crlf
			<< "(\033[1mｱﾌﾟﾘｹｰｼｮﾝID,無線ﾁｬﾈﾙ,暗号化有無･鍵\033[0m)" << crlf
			<< crlf
			<< "ｲﾝﾀﾗｸﾃｨﾌﾞﾓｰﾄﾞで\033[1m親機側(App_Wings)と子機側双方\033[0mの設定を" << crlf
			<< "確認してください。"
			;
	}

	void setup() {
		_app.the_screen.clear_screen();
		_app.the_screen_b.clear_screen();
		_app.set_title_bar(int(PAGE_ID::PAGE_OPEN));

		show_message();

		_btns.add(2, 12, L"Glancer - ｸﾞﾗﾝｻｰ(多種ﾊﾟｹｯﾄ情報確認)"
			, [&](int, uint32_t) { _app._tabs.select(int(PAGE_ID::PAGE_SCR_GLANCER)); }
			, 0
		);

		_btns.add(2, 14, L"TWELITE CUE 簡易確認"
			, [&](int, uint32_t) { _app._tabs.select(int(PAGE_ID::PAGE_SCR_CUE)); }
			, 0
		);

		_btns.add(2, 16, L"TWELITE ARIA 簡易確認"
			, [&](int, uint32_t) { _app._tabs.select(int(PAGE_ID::PAGE_SCR_ARIA)); }
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
	the_settings_menu.begin(appid_to_slotid(APP_ID));
	
	// put a init message
	const char* fmt_title = "\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m グランサー\033[0m : %s";
	the_screen_t << printfmt(fmt_title, "---"); // accepts UTF-8 codes
	pkt_data.init_screen(fmt_title);

	//     "0....+....1....+....2....+....3....+....4....+....5..
	the_screen(0, 6)
		<< "  -- TWELITE Wings で受信したパケット情報を表示します --" << crlf
		;

	// button navigation
	set_nav_bar();
}


// button navigation
void APP_BASE::SCR_GLANCER::set_nav_bar() {
	the_screen_c.clear_screen();

	if (pkt_data._bsolo) {
		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << "     --/長押:戻る            --/--                --/ﾘｾｯﾄ";
	}
	else {
		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << "     ↑/長押:戻る          決定/ｿｰﾄ               ↓/ﾘｾｯﾄ";
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
				pkt_data._bsolo = false;
				pkt_data._dat_solo.clear();
				
				pkt_data.update_term();
				set_nav_bar();
			} else {
				the_app.exit(APP_BASE::APP_ID);
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

				const char str_srt_key[_SORT_KEYS_COUNT][32] = {
					"論理ID",
					"シリアル番号",
					"リンク品質 Lq",
					"電圧 mV",
					"時間 s"
				};
				// sort entries
				pkt_data.sort_entries();
				pkt_data.update_term();

				the_screen_b.clear_screen();
				the_screen_b << printfmt("\033[30;33m[%s]でソートしました\033[0m", str_srt_key[sort_key]);

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
