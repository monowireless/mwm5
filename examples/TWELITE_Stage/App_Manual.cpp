/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include <algorithm>
#include <iterator>

#include "App_Manual.hpp"

using APP_BASE = App_Manual;

template<>
const wchar_t* APP_BASE::APP_DESC<APP_BASE>::TITLE_LONG[] = {
	L"マニュアルを開く",
	L"Open manual"
};

template<>
const wchar_t* APP_BASE::APP_DESC<APP_BASE>::LAUNCH_MSG[] =
//....+....1....+....2....+....3....+....4| // 16dots 40cols
{
L"説明書を開く""\r\n"
L"※ ブラウザで資料を開きます""\r\n"
,
L"Open Manual documents""\r\n"
L"※ Documents are opened with web browser.""\r\n"
};

void APP_BASE::setup() {
	// preference
	the_settings_menu.begin(0); // load default preference
	
	// init the TWE M5 support
	setup_screen(); // initialize TWE M5 support.

	// put a init message
	the_screen_t << MLSLW(L"\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m 説明書",
						  L"\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m Manual");
						  
	// button navigation
	the_screen_c.clear_screen();
	//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
	the_screen_c << MLSLW(L"     ↑/長押:戻る          選択/--                ↓/--",
						  L"     ↑/Long:BACK        SELECT/--                ↓/--");
						  
	
	// list ports
	_lv.push_back(
		MLSLW(
			L"TWELITE STAGE 説明書", 
			L"TWELITE STAGE Manual"),
		MLSLW(
			L"app_loc:MANUAL/jp/index.html", 
			L"app_loc:MANUAL/en/index.html"));

	_lv.push_back(
		MLSLW(
			L"MWX ライブラリ",
			L"MWX Library"),
		MLSLW(
			L"jp/MWX/index.html",
			L"en/MWX/index.html"));

	_lv.push_back(
		MLSLW(
			L"(TWENET C ライブラリ)",
			L"(TWENET C Library)"),
		MLSLW(
			L"jp/TWENET_C/index.html",
			L"en/TWENET_C/index.html"));

	_lv.attach_term(the_screen, 1, the_screen.get_rows() - 1);
	_lv.set_view();
	_lv.update_view(true);
}

void APP_BASE::loop() {
	do {
		int c = the_keyboard.read(); //note: even c==-1, it must be passed to list object for timeout check.

		if (_lv.size() > 0 && _lv.key_event(c)) {
			// pass key input to list widget and check select completion.
			if (_lv.is_selection_completed()) {
				int i_sel = _lv.get_selected_index();

				auto item_pair = _lv.get_selected();

				shell_open_help(item_pair.second);
				
				_lv.clear_completion();
				
				return; // wait for exit timer expiration.
			}
			else { // over
				int i_sel = _lv.get_selected_index();

				auto& t = the_screen_b;
				t.clear_screen();
				t << "\033[32m";

				switch (i_sel) {
				case 0:
					t << MLSLW(
							L"画面・メニュー構成や操作方法などについての解説です。"
							,
							L"Explanation of screen and menu structure and operations."
						);
					break;
				case 1:
					t << MLSLW(
							L"MWX ライブラリは setup(), loop() でよりシンプルに記述するためのC++ライブラリです。\r\n"
							L"※ 通常は MWX ライブラリでの実装を推奨します。"
							,
							L"The MWX library is a C++ library for simpler writing with setup(), loop().\r\n"
							L"※ Normally, implementation in the MWX library is recommended."
						);
					break;
				case 2:
					t << MLSLW(
							L"*** 多くの場合 MWX ライブラリでアプリ記述が可能です。通常は参照する必要はありません ***\r\n"
							L"TWENET C ライブラリは TweliteApps や MWX ライブラリなどの記述に用いられています。\r\n"
							L"※ ビルド環境や方法などのドキュメントにはTWELITE STAGE アプリを用いる前の記述もあるため、ご注意ください。"
							,
							L"*** Usually, there is no need to refer this TWENET C documents, but MWX library instead. ***\r\n"
							L"The TWENET C library is used to write the TweliteApps and MWX libraries.\r\n"
							L"※ Please note that this document include old syled the build environment and methods. "
							L"TWELITE STAGE App provices most of them now."
						);
					break;
				default: break;
				}

				t << "\033[0m";
			}
		}
		else
		switch (c) {
		case KeyInput::KEY_BUTTON_A: the_keyboard.push(KeyInput::KEY_UP); break;
		case KeyInput::KEY_BUTTON_B: the_keyboard.push(KeyInput::KEY_ENTER); break;
		case KeyInput::KEY_BUTTON_C: the_keyboard.push(KeyInput::KEY_DOWN); break;

		case KeyInput::KEY_ESC:
		case KeyInput::KEY_BUTTON_A_LONG: the_app.exit(0); break;
		default:
			break;
		}
	}
	while (the_keyboard.available());
	
	screen_refresh();
}

void APP_BASE::setup_screen() {
	// LCD font color
	default_bg_color = color565(90, 0, 50); 
	default_fg_color = ALMOST_WHITE;

#if M5_SCREEN_HIRES == 0
	// font register (note: to save flash area, don't create too much!)
	TWEFONT::createFontMP10_std(1, 0, 0); // MP10 font

	TWEFONT::createFontShinonome16_mini(10, 0, 0); // shinonome 16 font
	TWEFONT::createFontShinonome16_mini(11, 1, 0); // shinonome 16 font
	TWEFONT::createFontMP12_mini(12, 0, 0, TWEFONT::U32_OPT_FONT_TATEBAI | TWEFONT::U32_OPT_FONT_YOKOBAI); // MP10 font
	TWEFONT::createFontMP12_mini(13, 0, 0); // MP10 font

	the_screen.set_font(10);
	the_screen_b.set_font(1);
	the_screen_c.set_font(1);
	the_screen_t.set_font(11);
#elif M5_SCREEN_HIRES == 1
	// font register (note: to save flash area, don't create too much!)
	TWEFONT::createFontMP10_std(1, 0, 0); // MP10 font

	TWEFONT::createFontShinonome16(10, 5, 3);
	
	//TWEFONT::createFontMP12(11, 0, 0, TWEFONT::U32_OPT_FONT_YOKOBAI | TWEFONT::U32_OPT_FONT_TATEBAI);
	TWEFONT::createFontShinonome16(11, 0, 0, /* TWEFONT::U32_OPT_FONT_YOKOBAI */0);

	TWEFONT::createFontMP10_std(12, 0, 0, TWEFONT::U32_OPT_FONT_YOKOBAI | TWEFONT::U32_OPT_FONT_TATEBAI);
	TWEFONT::createFontShinonome14(13, 1, 0);

	the_screen.set_font(10);
	the_screen_b.set_font(13);
	the_screen_c.set_font(12);
	the_screen_t.set_font(11);
#endif

	// main screen area
	the_screen.set_color(default_fg_color, default_bg_color);
	the_screen.set_cursor(0); // 0: no 1: curosr 2: blink cursor
	the_screen.force_refresh();

	// bottom area
	the_screen_b.set_color(color565(80, 80, 80), color565(20, 20, 20));
	the_screen_b.set_cursor(0);
	the_screen_b.force_refresh();

	// bottom area
	the_screen_c.set_color(ORANGE, color565(20, 20, 20));
	the_screen_c.set_cursor(0);
	the_screen_c.force_refresh();

	// top area
	the_screen_t.set_color(default_bg_color, default_fg_color);
	the_screen_t.set_cursor(0);
	the_screen_t.force_refresh();
}

// screen refresh timing (every 32ms)
void APP_BASE::screen_refresh(bool bforce) {
	static uint32_t u32mills;

	uint32_t u32now = millis();
	if (bforce || u32now - u32mills > 32) {
		the_screen.refresh();
		the_screen_b.refresh();
		the_screen_c.refresh();
		the_screen_t.refresh();

		u32mills = u32now;
	}
}
#endif