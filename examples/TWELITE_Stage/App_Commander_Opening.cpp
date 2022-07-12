/* Copyright (C) 2020-2022 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_Commander.hpp"

using APP_BASE = App_Commander; //! alias to parent class

struct APP_BASE::SCR_OPEN : public APP_HANDLR_DC {
	static const int CLS_ID = APP_BASE::PAGE_ID::PAGE_OPEN;
	int get_class_id() { return CLS_ID; }

	APP_BASE& _app;
	TWE_ListView _lv;

	SCR_OPEN(APP_BASE& app) : _app(app), _lv(16), APP_HANDLR_DC(CLS_ID) {}
	~SCR_OPEN() {}

	void Btn_Press(int id, uint32_t opt = 0) {
		_app._tabs.select(opt);
	}

	void setup() {
		_app.screen_layout_opening();

		_app.the_screen.clear_screen();
		_app.the_screen_b.clear_screen();
		_app.set_title_bar(PAGE_ID::PAGE_OPEN);

		auto& t = _app.the_screen_c; t.clear_screen();
		//          "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		t << MLSLW(L"    ↑/長押:戻る           選択/--                ↓/ﾘｾｯﾄ",
			       L"    ↑/Long:BACK         SELECT/--                ↓/RST");

		// list view
		_lv.attach_term(_app.the_screen, 1, int(PAGE_ID::_PAGE_END_) - 1);
		_lv.set_view();

		for (int i = 0; PAGE_ID(i) < PAGE_ID::_PAGE_END_; i++) {
			switch (PAGE_ID(i)) {
				// case PAGE_ID::PAGE_OPEN: _lv.push_back(L"---", uint16_t(PAGE_ID::PAGE_OPEN)); break;
			case PAGE_ID::PAGE_TWELITE80: _lv.push_back(MLSLW(L"App_Twelite 0x80ｺﾏﾝﾄﾞ", L"App_Twelite 0x80 cmd"), uint16_t(PAGE_ID::PAGE_TWELITE80)); break;
			case PAGE_ID::PAGE_NOTICE01: _lv.push_back(L"NOTICE PAL LED", uint16_t(PAGE_ID::PAGE_NOTICE01)); break;

			default: break;
			}
		}

		_lv.update_view(true);
	}

	void loop() {
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
						case PAGE_ID::PAGE_TWELITE80:
							t <<
								MLSLW(L"TWELITE 標準アプリ(App_Twelite) の 0x80 コマンドを送信します。"
									  L"0x80コマンドは無線経由で相手側のディジタル・PWMポート制御します。"
									  ,
									  L"Sends the 0x80 command of the TWELITE standard application (App_Twelite)."
									  L"The 0x80 command controls the other side's digital PWM port via radio."
								);
							t << crlf << 
								MLSLW(L"(STAGEに接続するTWELITEには App_Wings またはMONOSTICK用の App_Twelite を書き込み、"
								      L"接続するTWELITE同士に、同一のｱﾌﾟﾘｹｰｼｮﾝIDと無線ﾁｬﾈﾙを設定しておきます)"
									  ,
									  L"(The TWELITE to be connected to the STAGE is programmed with App_Wings or App_Twelite for MONOSTICK,"
									  L" and the TWELITEs to be connected to each other are configured with the same Application ID and Radio channel.)"
								);
							break;
						case PAGE_ID::PAGE_NOTICE01:
							t <<
								MLSLW(L"通知PAL(NOTICE PAL)のLED制御を行います。"
									  ,
									  L"LED control for notification PAL (NOTICE PAL)."
								);
							t << crlf <<
								MLSLW(L"(STAGEに接続するTWELITEには App_Wings を書き込み、"
								      L"接続するTWELITE同士に、同一のｱﾌﾟﾘｹｰｼｮﾝIDと無線ﾁｬﾈﾙを設定しておきます)"
									  ,
									  L"(The TWELITE to be connected to the STAGE is programmed with App_Wings,"
									  L" and the TWELITEs to be connected to each other are configured with the same Application ID and Radio channel.)"
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
		;
	}
};

/**
 * create an instance of hander for SCR_GLANCER.
 */
void APP_BASE::hndr_SCR_OPEN(event_type ev, arg_type arg) { hndr<SCR_OPEN>(ev, arg); }