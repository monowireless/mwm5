/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_CUE.hpp"

static const wchar_t HELP_MSG[][384] =
{
	//   "0....+....1....+....2....+....3....+....4....+....5..
		L"TWELITE CUE/ARIAからのﾒｯｾｰｼﾞを解釈・表示します。" L"\r\n"
		L"\r\n"
		L"親機にApp_Wingsを書込ｱﾌﾟﾘｹｰｼｮﾝIDとﾁｬﾈﾙを設定します。" L"\r\n"
		L"親機とCUE/ARIAは同じ設定でなければ通信できません。" L"\r\n"
		L"\r\n"
		L"CUE/ARIA->App_Wings親機に無線ﾊﾟｹｯﾄが送信されたとき" L"\r\n"
		L"ﾃﾞｰﾀを解釈し、ｾﾝｻｰ情報、ｾﾝｻｰ情報の評価したｲﾍﾞﾝﾄ情報が" L"\r\n"
	    L"都度表示されます。" L"\r\n"
		,
	//   "0....+....1....+....2....+....3....+....4....+....5..
		L"[加速度センサーに基づくイベント]" L"\r\n"
		L"  \033[7mﾑｰﾌﾞ:加速度の変化を検出\033[0m" L"\r\n"
		L"     一定の加速度を検出したときにｽﾘｰﾌﾟ起床した後" L"\r\n"
	    L"     加速度情報を10ｻﾝﾌﾟﾙ収集しﾃﾞｰﾀ送信します。" L"\r\n"
		L"     ※静止時にも検出される場合があります。" L"\r\n"
		L"  \033[7mｼｪｲｸ:加速度の変化->連続的な加速度を検出\033[0m" L"\r\n"
		L"     ﾑｰﾌﾞ検出後の加速度情報を評価し連続的な加速度を" L"\r\n"
		L"     検出した場合はﾑｰﾌﾞの替わりにｼｪｲｸが発生します。" L"\r\n"
		L"  \033[7mﾀﾞｲｽ:ﾀｲﾏｰ送信時に面を確定\033[0m"  L"\r\n"
		L"     ﾑｰﾌﾞ以外のｽﾘｰﾌﾟ起床後に加速度情報を10ｻﾝﾌﾟﾙ収集" L"\r\n"
		L"     加速度情報の評価の結果、動いていないと判定され" L"\r\n"
		L"     る場合、面に対応するｲﾍﾞﾝﾄ(1..6)が発生します。" L"\r\n"
		L"\r\n"
		,
	//   "0....+....1....+....2....+....3....+....4....+....5..
		L"[TWELITE CUE センサー値情報]" L"\r\n"
		L"  電源電圧: 電源電圧をmVで計測した結果" L"\r\n"
		//L"  ADC1電圧: ADC1をmVで計測した結果" L"\r\n"
		L"  磁気ｾﾝｻｰ: 磁気センサーの状態" L"\r\n"
		L"      磁石の極によりS/N極が検出されます。" L"\r\n"
		L"      動かし方によって反対の極も検出されます。" L"\r\n"
		L"  加速度  : 受信した10ｻﾝﾌﾟﾙのうち先頭8ｻﾝﾌﾟﾙの平均" L"\r\n"
		L"      単位はmG(ﾐﾘG, 1G=9.8m/s²=1000mG)です。" L"\r\n"
		,
	//   "0....+....1....+....2....+....3....+....4....+....5..
		L"[TWELITE ARIA センサー値情報]" L"\r\n"
		L"  時間(s) : ｱﾌﾟﾘを起動してから受信するまでの時間。" L"\r\n"
		L"      単位は秒です。" L"\r\n"
		L"  ID      : TWELITE ARIAの論理ﾃﾞﾊﾞｲｽID" L"\r\n"
		L"  VCC(mV) : 電源電圧をmVで計測した結果" L"\r\n"
		//L"  ADC1電圧: ADC1をmVで計測した結果" L"\r\n"
		L"  温度(C) : 計測した温度。単位は℃です。" L"\r\n"
		L"  湿度(%) : 計測した湿度。単位は％です。" L"\r\n"
		L"  磁石    : 磁気センサーの状態" L"\r\n"
		L"      磁石の極によりS/N極が検出されます。" L"\r\n"
		L"      動かし方によって反対の極も検出されます。" L"\r\n"

};
static const int MAX_PAGE_NUM = int(elements_of_array(HELP_MSG) - 1);

struct App_CUE::SCR_HELP : public APP_HANDLR_DC {
	static const int CLS_ID = App_CUE::PAGE_ID::PAGE_HELP;
	int get_class_id() { return CLS_ID; }

	App_CUE& _app;
	TWE_WidSet_Buttons _btns;

	// object references to the App_CUE
	ITerm& the_screen;
	ITerm& the_screen_b;

	// help page
	int _page; // 0..MAX_PAGE_NUM
	static const uint32_t PAGE_NEXT = 0x81;
	static const uint32_t PAGE_PREV = 0x82;

	SCR_HELP(App_CUE& app) : _app(app), _btns(*this, app.the_screen)
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
		the_screen << HELP_MSG[_page];

		// controls
		the_screen(23, 14) << printfmt("%d / %d", _page + 1, MAX_PAGE_NUM + 1);
		_btns.update_view();
	}
};

// Screen def: opening
void App_CUE::hndr_help(event_type ev, arg_type arg) {
	// renew object
	auto&& dc = APP_HNDLR::use<SCR_HELP>();

	switch (ev) {
	case EV_SETUP:
		// init screen
		the_screen.clear_screen();
		the_screen_b.clear_screen();
		set_title_bar(PAGE_ID::PAGE_HELP);
		//          "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		set_nav_bar("  前TAB/長押:戻る      次ページ/--             次TAB/ﾘｾｯﾄ");

		// the message
		dc.show_message(0);

		// add next/prev page buttons
		dc._btns.add( 2, 14, L"＜前", &SCR_HELP::Btn_Press, SCR_HELP::PAGE_PREV);
		dc._btns.add(47, 14, L"次＞", &SCR_HELP::Btn_Press, SCR_HELP::PAGE_NEXT);

		// add direct page selection buttons
		for (int i = 0; i <= MAX_PAGE_NUM; i++) {
			static const wchar_t LBLS[][2] = { L"1", L"2", L"3", L"4", L"5" };
			dc._btns.add(8 + i * 2, 14
				, i < int(elements_of_array(LBLS)) ? LBLS[i] : L"X"
				, &SCR_HELP::Btn_Press, i);
		}
		break;

	case EV_LOOP:
		dc._btns.check_events();

		do {
			int c = the_keyboard.read();

			switch (c) {
			// case KeyInput::KEY_BUTTON_A: break; // captured event check by the parent.
			// case KeyInput::KEY_BUTTON_C: break; captured event check by the parent.
			
			case KeyInput::KEY_BUTTON_A_LONG:
			case KeyInput::KEY_BS:
				dc._app._tabs.select(PAGE_ID::PAGE_BASIC);
				break;

			case KeyInput::KEY_BUTTON_B:
			case ' ': // space
				dc.show_message(SCR_HELP::PAGE_NEXT);
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
		break;

	case EV_EXIT:
		break;
	}
}