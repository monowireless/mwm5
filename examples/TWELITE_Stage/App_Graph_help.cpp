/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_Graph.hpp"

static const wchar_t HELP_MSG[][TWE::LANG_CT][1024] =
{
	
	{//  "0....+....1....+....2....+....3....+....4....+....5..
		L"[加速度リアルタイムグラフ]" L"\r\n"
		L"  加速度のグラフ表示を行います。" L"\r\n"
		L"  ・受信データは log ﾌｫﾙﾀﾞに保存します。" L"\r\n"
		L"    保存先は(l)ｷｰ。ｸﾞﾗﾌ画面終了までﾌｧｲﾙは開けません。" L"\r\n"
		L"  ・操作:" L"\r\n"
		L"    →, ←, ﾏｳｽ左ﾄﾞﾗｯｸﾞ ⇒ 表示位置移動" L"\r\n"
		L"    ↑, ↓, ﾏｳｽﾎｲｰﾙ     ⇒ 横方向拡大縮小" L"\r\n"
		L"    ( ), 右クリック     ⇒ 一時停止" L"\r\n"
		L"    (c) ⇒ 表示位置のデータCSV保存" L"\r\n"
		L"    (f) ⇒ FFTサンプル数を変更" L"\r\n"
		L"※ ｻﾝﾌﾟﾙﾚｰﾄ･周波数は受信時刻からの推測値です。"
		,
		L"[Acceleration real-time graph" L"\r\n"
		L"  Display a graph of acceleration." L"\r\n"
		L"  ・Received data is saved in the log folder." L"\r\n"
		L"    Save to (l) key. The file will not be opened until" L"\r\n"
		L"    the end of the graph screen." L"\r\n"
		L"  ・Operation:" L"\r\n"
		L"    →, ←, mouse L drag⇒ move pos" L"\r\n"
		L"    ↑, ↓, wheel       ⇒ Horizontal scaling" L"\r\n"
		L"    ( ), R click        ⇒ PAUSE" L"\r\n"
		L"※ The sample rate and frequency are estimated values"  L"\r\n"
	    L"   based on the time of reception."
	},
	{ // "0....+....1....+....2....+....3....+....4....+....5..
		L"[センサーグラフ]" L"\r\n"
		L"  各種センサーデータを保存・表示します。" L"\r\n"
		L"  ・操作(24H画面)" L"\r\n"
		L"    →, ←, ﾏｳｽ左ﾄﾞﾗｯｸﾞ ⇒ 表示位置移動" L"\r\n"
		L"    ↑, ↓, ﾏｳｽﾎｲｰﾙ     ⇒ 横方向拡大縮小" L"\r\n"
		,
		L"[Sensor graph]" L"\r\n"
		L"  Stores and displays various sensor data." L"\r\n"
		L"  ・Operation (24H screen)" L"\r\n"
		L"    →, ←, mouse L drag⇒ move pos" L"\r\n"
		L"    ↑, ↓, wheel       ⇒ Horizontal scaling" L"\r\n"
		L""

	}
};
static const int MAX_PAGE_NUM = int(elements_of_array(HELP_MSG) - 1);

struct App_Graph::SCR_HELP : public APP_HANDLR_DC {
	static const int CLS_ID = App_Graph::PAGE_ID::PAGE_HELP;
	int get_class_id() { return CLS_ID; }

	App_Graph& _app;
	TWE_WidSet_Buttons _btns;

	// object references to the App_CUE
	ITerm& the_screen;
	ITerm& the_screen_b;

	// help page
	int _page; // 0..MAX_PAGE_NUM
	static const uint32_t PAGE_NEXT = 0x81;
	static const uint32_t PAGE_PREV = 0x82;

	SCR_HELP(App_Graph& app) : _app(app), _btns(*this, app.the_screen)
		, the_screen(app.the_screen), the_screen_b(app.the_screen_b)
		, _page(0)
		, APP_HANDLR_DC(CLS_ID) {}
	~SCR_HELP() {}

	// called when button was pressed
	void Btn_Press(int id, uint32_t opt = 0) {
		show_message(opt);
	}

	// display help message
	void show_message(int page) {
		// select page
		switch (page) {
		case PAGE_NEXT: _page++; break;
		case PAGE_PREV: _page--; break;
		default: if (page >= 0 && page <= MAX_PAGE_NUM) _page = page; break;
		}
		if (_page < 0) _page = MAX_PAGE_NUM;
		if (_page > MAX_PAGE_NUM) _page = 0;

		// show message
		the_screen.clear_screen();
		the_screen << HELP_MSG[_page][g_lang];

		// controls
		the_screen(23, 14) << printfmt("%d / %d", _page + 1, MAX_PAGE_NUM + 1);
		_btns.update_view();
	}


	void setup() {
		// init screen
		the_screen.clear_screen();
		the_screen_b.clear_screen();
		_app.set_title_bar(PAGE_ID::PAGE_HELP);
		//          "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		_app.set_nav_bar(MLSL(
			" --/長押:戻る     次ページ/--                --/ﾘｾｯﾄ",
			" --/Long:BACK         NEXT/--                --/RST"
			)
		);

		// the message
		show_message(0);

		// add next/prev page buttons
		_btns.add(2, 14, MLSLW(L"＜前", L"<Pre"), &SCR_HELP::Btn_Press, this, SCR_HELP::PAGE_PREV);
		_btns.add(47, 14, MLSLW(L"次＞", L">Nxt"), &SCR_HELP::Btn_Press, this, SCR_HELP::PAGE_NEXT);

		// add direct page selection buttons
		for (int i = 0; i <= MAX_PAGE_NUM; i++) {
			static const wchar_t LBLS[][2] = { L"1", L"2", L"3", L"4", L"5" };
			_btns.add(8 + i * 2, 14
				, i < int(elements_of_array(LBLS)) ? LBLS[i] : L"X"
				, &SCR_HELP::Btn_Press, this, i);
		}
	}

	void loop() {
		_btns.check_events();

		do {
			int c = the_keyboard.read();

			switch (c) {
				// case KeyInput::KEY_BUTTON_A: break; // captured event check by the parent.
				// case KeyInput::KEY_BUTTON_C: break; captured event check by the parent.

			case KeyInput::KEY_BUTTON_A_LONG:
			case KeyInput::KEY_BS:
				_app._tabs.select(PAGE_ID::PAGE_OPEN);
				break;

			case KeyInput::KEY_BUTTON_B:
			case ' ': // space
				show_message(SCR_HELP::PAGE_NEXT);
				break;

			default:
				break;
			}

		} while (the_keyboard.available());

		// read the uart queue
		do {
			int c = the_uart_queue.read();
			// do nothing
		} while (the_uart_queue.available());
	}

	void on_close() {
		// do nothing 
	}

};

// generate handler instance (SCR_XXX needs to have setup(), loop(), on_close() methods)
void App_Graph::hndr_SCR_HELP(event_type ev, arg_type arg) { hndr<SCR_HELP>(ev, arg); }
