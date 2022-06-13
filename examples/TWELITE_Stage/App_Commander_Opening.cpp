/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_Commander.hpp"

struct App_Commander::SCR_OPEN : public APP_HANDLR_DC {
	static const int CLS_ID = App_Commander::PAGE_ID::PAGE_OPEN;
	int get_class_id() { return CLS_ID; }

	App_Commander& _app;
	TWE_WidSet_Buttons _btns;

	SCR_OPEN(App_Commander& app) : _app(app), _btns(*this, app.the_screen), APP_HANDLR_DC(CLS_ID) {}
	~SCR_OPEN() {}

	void show_message() {
		auto& t = _app.the_screen;

		//     "0....+....1....+....2....+....3....+....4....+....5...
		t   << "Commanderはｱｽｷｰ形式のｺﾏﾝﾄﾞをApp_Wingsに渡します。" << crlf
			<< "(App_Wingsを書き込んだ親機に接続します)" << crlf
			<< crlf
			<< "App_Wingsは様々な子機ｱﾌﾟﾘに対しｺﾏﾝﾄﾞを無線経由で送信し" << crlf
			<< "ます。App_Wingsと子機ｱﾌﾟﾘ(TWELITE標準ｱﾌﾟﾘやPALｱﾌﾟﾘ)" << crlf
			<< "の設定(\033[7;41mｱﾌﾟﾘｹｰｼｮﾝID,無線ﾁｬﾈﾙ,暗号化有無･鍵\033[0m)が同じで" << crlf
			<< "ないと無線通信しないようになっています。" << crlf
			<< crlf
			<< "ｲﾝﾀﾗｸﾃｨﾌﾞﾓｰﾄﾞで\033[41;7m親機側(App_Wings)と子機側双方\033[0mの設定を" << crlf
			<< "確認してください。"
			;
	}

	void Btn_Press(int id, uint32_t opt = 0) {
		_app._tabs.select(opt);
	}
};

 // Screen def: opening
void App_Commander::hndr_opening(event_type ev, arg_type arg) {
	// renew object
	auto&& dc = APP_HNDLR::use<SCR_OPEN>();

	switch (ev) {
	case EV_SETUP:
		the_screen.clear_screen();
		the_screen_b.clear_screen();
		set_title_bar(PAGE_ID::PAGE_OPEN);

		dc.show_message();

		dc._btns.add(2,  13, L"App_Twelite 0x80ｺﾏﾝﾄﾞ(t)", &SCR_OPEN::Btn_Press, &dc, 1);
		dc._btns.add(27, 13, L"NOTICE PAL LED(n)", &SCR_OPEN::Btn_Press, &dc, 2);
		break;

	case EV_LOOP:
		dc._btns.check_events();

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
		break;

	case EV_EXIT:
		break;
	}
}