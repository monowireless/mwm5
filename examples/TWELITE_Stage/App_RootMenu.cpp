/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_RootMenu.hpp"
#include "AppIDs.h"

// flag of working state
static int s_bstarted = 0;

void App_RootMenu::setup() {
	// load the default profile
	the_settings_menu.begin(0); // load slot 0 (=default) profile

	// at he first load, run the specified app upon setting.
	if (!s_bstarted) {
		s_bstarted = 1;

		// select autorun app
		if (sAppData.u8_TWESTG_STAGE_START_APP > 0 && sAppData.u8_TWESTG_STAGE_START_APP < (int)E_APP_ID::_APPS_END_) {
			s_bstarted = 2; // launch app on boot
			_b_appmenu = true;
			the_keyboard.push('0' + sAppData.u8_TWESTG_STAGE_START_APP);
		}
	}
	else {
		// if the previous app is in the appmenu, set the view.
		if (_nSel > 0 && _nSel < (int)E_APP_ID::FIRM_PROG) {
			_b_appmenu = true;
		}
	}

	// init the TWE M5 support
	setup_screen(); // initialize TWE M5 support.

	// nav-bars
	set_title_status();

	// menus
	set_listview();
}

void App_RootMenu::set_listview() {
	_listMenu.clear();

	// for sub data (stores app_id)
	SmplBuf_WCharL<4> l_appid;
	SmplBuf_WChar& appid = l_appid.get();
	appid.resize(1);

	if (_b_appmenu) { // アプリ一覧
#if !(defined(ESP32) || defined(MWM5_BUILD_RASPI))
		_listMenu.set_info_area(L"ｳｪﾌﾞ");
#endif
		for (int menu_id = 1; menu_id < (int)E_APP_ID::_APPS_END_; menu_id++) {
			appid[0] = menu_id;
			_listMenu.push_back(str_appnames[menu_id], appid);
		}
	}
	else {
#if !(defined(ESP32) || defined(MWM5_BUILD_RASPI))
		_listMenu.set_info_area(L"ｳｪﾌﾞ");
#endif

		appid[0] = 0;
		_listMenu.push_back(str_appnames[0], appid);

		for (int menu_id = (int)E_APP_ID::_APPS_END_ + 1; menu_id < (int)E_APP_ID::_END_; menu_id++) {
			if (menu_id != (int)E_APP_ID::_UTILS_END_) {
				appid[0] = menu_id;
#ifdef ESP32
				_listMenu.push_back(str_appnames[menu_id], appid);
#else

				if (menu_id == (int)E_APP_ID::SELECT_PORT) {
					SmplBuf_WChar str;
					str << str_appnames[menu_id];

					if (Serial2.is_opened()) {
						str << L'[' << Serial2.get_devname() << L']';
					} else {
						str << L"[未接続]";
					}
					_listMenu.push_back(as_moving(str), appid);

					if (menu_id == _nSel) _nSel = -1;
				} else {
					_listMenu.push_back(str_appnames[menu_id], appid);
				}
#endif
			}
		}
	}

	if (_nSel > 0) {
		for (int i = 0; i < _listMenu.size(); i++) {
			if (_listMenu[i].second[0] == _nSel) {
				_listMenu.set_view(0, i);
			}
		}
	}
	_listMenu.attach_term(the_screen);
	_listMenu.update_view(true);
}

void App_RootMenu::loop() {	
	// Keyboard
	do {
		int key = the_keyboard.read();

		// just wait to next button/Enter to launch viewer.
		if (_i_selected_viewer_app != APP_ID) {
			switch (key) {
			case KeyInput::KEY_BUTTON_A_LONG:
			case KeyInput::KEY_ESC:
				the_app.exit(-1); // reload this memu
				return;
			case KeyInput::KEY_ENTER:
			case KeyInput::KEY_BUTTON_B:
				the_app.exit(-1, _i_selected_viewer_app);
				break;
			default:
				if (TWECUI::KeyInput::MOUSE_UP::is_type(key)) {
					// press LEFT mouse button to proceed.
					TWECUI::KeyInput::MOUSE_UP ev(key);
					if (auto&& coord = the_screen.get_term_coord_from_screen(ev.get_x(), ev.get_y())) {				
						the_app.exit(-1, _i_selected_viewer_app);
					}
				}
			}

			continue;
		}

		if (_listMenu.key_event(key)) {
			if (_listMenu.is_info_selected()) {
#ifndef ESP32
				// open info url
				if (_b_appmenu) {
					int i = _listMenu.get_selected_index() + 1;
					if (i > 0 && i < int(E_APP_ID::_APPS_END_)) {
						shell_open_url(str_appurls[i]);
					}
				} else {
					unsigned i = _listMenu.get_selected_index();
					auto wstr = str_appurls[0]; // Viewer
					if (i > 0 && i < int(E_APP_ID::_END_)) {
						wstr = str_appurls[i + int(E_APP_ID::_APPS_END_)];
					}
					if (i < int(E_APP_ID::_END_)) {
						shell_open_url(wstr);
					}
				}
#endif
			} else
			if (_listMenu.is_selection_completed()) {
				if (!_b_appmenu && _listMenu.get_selected_index() == 0) {
					// move to appmenu
					_b_appmenu = true;
					set_listview();
					set_title_status();
				}
				else {
					uint16_t sel_app_id = (uint16_t)_listMenu.get(_listMenu.get_selected_index()).second[0];

					if (_b_appmenu) {
						// auto-launch at booting.
						if (s_bstarted == 2) {
							s_bstarted = 3;
							the_app.exit(-1, sel_app_id);
							return;
						}

						// show init message.
						_i_selected_viewer_app = int(sel_app_id);

						// show help screen
						the_screen.clear_screen();
						the_screen << query_app_launch_message(sel_app_id);

						the_screen_c.clear_screen();
						//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
						the_screen_c << "     --/長押:戻る          進む/--                --/--";

						the_screen << crlf
							<< printfmt("\033[%d;1H", the_screen.get_rows())
							//  "....+....1....+....2....+....3....+....4"
							//         中ボタンまたは[Enter]で進む
							<< L"      \033[7m中ボタン\033[0mまたは[\033[7mEnter\033[0m]で進む";

						continue;
					}
					else the_app.exit(-1, sel_app_id); // switch to the app.
				}
				return;
			}
		}
		else switch (key) {
		case KeyInput::KEY_ESC:
			if (_b_appmenu) {
				_b_appmenu = false;
				set_listview();
				set_title_status();
			}
			else {
				_listMenu.set_view(0, -1);
			}
			return;

		case KeyInput::KEY_BUTTON_A:
			the_keyboard.push(TWECUI::KeyInput::KEY_UP);
			break;

		case KeyInput::KEY_BUTTON_A_LONG:
			if (_b_appmenu) {
				_b_appmenu = false;
				set_listview();
				set_title_status();
			}
			else {
				M5.powerOFF();
			}
			break;

		case KeyInput::KEY_BUTTON_B:
			the_keyboard.push(TWECUI::KeyInput::KEY_ENTER);
			break;

		case KeyInput::KEY_BUTTON_B_LONG:
			break;

		case KeyInput::KEY_BUTTON_C:
			the_keyboard.push(TWECUI::KeyInput::KEY_DOWN);
			break;

		case KeyInput::KEY_BUTTON_C_LONG:
			break;
		}
	} while (the_keyboard.available());

	// LCD update
	check_for_refresh();
}

void App_RootMenu::setup_screen() {
	// LCD font color
	default_bg_color = color565(sAppData.u32_TWESTG_STAGE_BG_COLOR); // color565(90, 0, 50); 
	default_fg_color = color565(sAppData.u32_TWESTG_STAGE_FG_COLOR);

	// font register (note: to save flash area, don't create too much!)
	TWEFONT::createFontMP10_std(1, 0, 0); // MP10 font

	TWEFONT::createFontShinonome16_mini(10, 1, 0); // shinonome 16 font
	TWEFONT::createFontShinonome16_mini(11, 1, 0); // shinonome 16 font
	TWEFONT::createFontMP12_mini(12, 0, 0, TWEFONT::U32_OPT_FONT_TATEBAI | TWEFONT::U32_OPT_FONT_YOKOBAI); // MP10 font
	TWEFONT::createFontMP12_mini(13, 0, 0); // MP10 font

	// main screen area
	the_screen.set_font(10);
	the_screen.set_color(default_fg_color, default_bg_color);
	the_screen.set_cursor(0); // 0: no 1: curosr 2: blink cursor
	the_screen.clear_screen();
	the_screen.force_refresh();

	// bottom area
	the_screen_b.set_font(1);
	the_screen_b.set_color(color565(80, 80, 80), color565(20, 20, 20));
	the_screen_b.set_cursor(0);
	the_screen_b.clear_screen();
	the_screen_b.force_refresh();

	// bottom area
	the_screen_c.set_font(1);
	the_screen_c.set_color(ORANGE, color565(20, 20, 20));
	the_screen_c.set_cursor(0);
	the_screen_c.clear_screen();
	the_screen_c.force_refresh();

	// top area
	the_screen_t.set_font(11);
	the_screen_t.set_color(default_bg_color, default_fg_color);
	the_screen_t.set_cursor(0);
	the_screen_t.clear_screen();
	the_screen_t.force_refresh();
}

// screen refresh timing (every 32ms)
void App_RootMenu::check_for_refresh() {
	static uint32_t u32mills;

	uint32_t u32now = millis();
	if (u32now - u32mills > 32) {
		the_screen.refresh();
		the_screen_b.refresh();
		the_screen_c.refresh();
		the_screen_t.refresh();

		u32mills = u32now;
	}
}


void App_RootMenu::set_title_status() {
	the_screen_c.clear_screen();

	if (this->_b_appmenu) {
		// put a init message
		the_screen_t.clear_screen();
		the_screen_t << "\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m ビューア";

		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << "     ↑/長押:戻る          決定/--                ↓/--";
	} else {
		// put a init message
		the_screen_t.clear_screen();
		the_screen_t << "\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m"
			<< printfmt(" (v%d-%d-%d)", MWM5_APP_VERSION_MAIN, MWM5_APP_VERSION_SUB, MWM5_APP_VERSION_VAR);

#ifdef ESP32
		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << "     ↑/長押:電源OFF       決定/--                ↓/--";
#else
		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << "     ↑/長押:終了          決定/--                ↓/--";
#endif
	}
}