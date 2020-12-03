/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_FirmProg.hpp"
#include "App_Interactive.hpp"

#include <utility>
#include <algorithm>

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <cstdlib>
#include <wchar.h>
namespace fs = std::filesystem;
#endif

// updates Serial/keyboard input queue
extern void update_serial_keyb_input(bool);

#ifndef ESP32
static const char STR_BUILD_ERROR_LOG[] = "builderr.log";
static const wchar_t WSTR_BUILD_ERROR_LOG[] = L"builderr.log";

static wchar_t STR_SDK_ACT_DEV[] = L"Act_samples";
static wchar_t STR_SDK_TWEAPPS_DEV[] = L"Wks_TweApps";

/** @brief	The last selection menu of Screen_OpenMenu. (to identify last item, a firmware file or build project.) */
int App_FirmProg::_last_menu_number = App_FirmProg::Screen_OpenMenu::MENU_NONE;

/** @brief	act/tweapps name (a single porjects) */
SmplBuf_WChar App_FirmProg::_build_name;
/** @brief	workspace name (Act_samples or Wks_TweApps) */
SmplBuf_WChar App_FirmProg::_build_workspace;
/** @brief	project name,
 *			if set: {workspace}/{projects}/{act or tweapps name}
 *			not   : {workspace}/{act or tweapps name}
*/
SmplBuf_WChar App_FirmProg::_build_project;

/** @brief	the last chosen project name */
SmplBuf_WChar App_FirmProg::_build_project_prev;

#endif

/** @brief	dir where a firmware file is stored. */
SmplBuf_WChar App_FirmProg::_firmfile_dir;

/** @brief	Name of the application firmware file, to open file. */
SmplBuf_WChar App_FirmProg::_firmfile_name;

/** @brief	Display name of the firmware file. */
SmplBuf_WChar App_FirmProg::_firmfile_disp;

/** @brief	Module type of firmware file. */
TWE::TweProg::E_MOD_TYPE App_FirmProg::_firmfile_modtype;

// pass the Parent node's object to change_app().
App_FirmProg* App_FirmProg::SubScreen::_parent = nullptr;

// color table
static const uint16_t COLTBL_MAIN[8] = {
	BLACK,
	RED,
	color565(31, 191, 31), // GREEN
	YELLOW,
	color565(127, 127, 255), // BLUE,
	color565(255, 0, 142), // MAGENTA,
	CYAN,
	ALMOST_WHITE
};

/**
 * @fn	static int change_app(TWE::APP_MGR& the_app, int n, int prev_app, int exit_id)
 *
 * @brief	Change sub application in App_FirmProg.
 * 			The next application is chosen by the `exit_code'.
 *
 * @param [in,out]	the_app 	The application management class object.
 * @param 		  	n_appsel	The requested app id.
 * @param 		  	prev_app	The previous(exiting) app id.
 * @param 		  	exit_id 	The exit code of the previous(exiting) app.
 *
 * @returns	        app id of switched app.
 */
int App_FirmProg::change_app(TWE::APP_MGR& sub_app, int next_app, int prev_app, int exit_code) {
	if (next_app == Screen_ModIdentify::SCR_ID) {
		sub_app.new_app<Screen_ModIdentify>();
		return next_app;
	}

	switch (prev_app) {
	case Screen_ModIdentify::SCR_ID:
		switch (exit_code) {
		case SubScreen::EXIT_BACK_TO_MENU:
			::the_app.exit(APP_ID);
			break;

		case SubScreen::EXIT_NEXT:
#ifdef ESP32
			sub_app.new_app<Screen_FileBrowse>();
			return Screen_FileBrowse::SCR_ID;
#else
			sub_app.new_app<Screen_OpenMenu>();
			return Screen_OpenMenu::SCR_ID;
#endif

		case SubScreen::EXIT_ERROR:
			sub_app.new_app<Screen_FatalError>();
			return Screen_FatalError::SCR_ID;

		default:
			::the_app.exit(APP_ID);
			break;
		}
		break;

#ifndef ESP32
	case Screen_OpenMenu::SCR_ID:
		switch (exit_code) {
		case SubScreen::EXIT_BACK_TO_MENU:
			::the_app.exit(APP_ID);
			break;

		case SubScreen::EXIT_NEXT:
			sub_app.new_app<Screen_FileBrowse>();
			return Screen_FileBrowse::SCR_ID;

		case SubScreen::EXIT_NEXT2:
			sub_app.new_app<Screen_FileProg>();
			return Screen_FileProg::SCR_ID;

		case SubScreen::EXIT_NEXT3:
			sub_app.new_app<Screen_ActBuild>(next_app);
			return Screen_ActBuild::SCR_ID;

		case SubScreen::EXIT_ERROR:
			sub_app.new_app<Screen_FatalError>();
			return Screen_FatalError::SCR_ID;

		default:
			::the_app.exit(APP_ID);
			break;
		}
		break;

	case Screen_ActBuild::SCR_ID:
		switch (exit_code) {
		case SubScreen::EXIT_BACK_TO_MENU:
			sub_app.new_app<Screen_OpenMenu>();
			return Screen_OpenMenu::SCR_ID;

		case SubScreen::EXIT_NEXT:
			sub_app.new_app<Screen_FileProg>();
			return Screen_FileProg::SCR_ID;
			break;

		default:
			::the_app.exit(APP_ID);
			break;
		}
#endif

	case Screen_FileBrowse::SCR_ID:
		switch (exit_code) {
		case SubScreen::EXIT_BACK_TO_MENU:

#ifdef ESP32
			::the_app.exit(APP_ID);
			break;
#else
			sub_app.new_app<Screen_OpenMenu>();
			return Screen_OpenMenu::SCR_ID;
#endif

		case SubScreen::EXIT_NEXT:
			sub_app.new_app<Screen_FileProg>();
			return Screen_FileProg::SCR_ID;
		

		case SubScreen::EXIT_ERROR:
			sub_app.new_app<Screen_FatalError>();
			return Screen_FatalError::SCR_ID;
			
		default:
			::the_app.exit(APP_ID);
			break;
		}
		break;

	case Screen_FileProg::SCR_ID:
		switch (exit_code) {
		case SubScreen::EXIT_ERROR:
			// in the case of an error, back to FileBrowsing.
			sub_app.new_app<Screen_FileBrowse>();
			return Screen_FileBrowse::SCR_ID;

		case SubScreen::EXIT_NEXT:
			
			// switch to Console.
			switch (sAppData.u8_TWESTG_STAGE_APPWRT_BUILD_NEXT_SCREEN) {
			case 0: ::the_app.exit(APP_ID, int(E_APP_ID::INTERACTIVE)); break; // INTERACTIVE MENU
			case 1: ::the_app.exit(APP_ID, int(E_APP_ID::CONSOLE)); break; // TERMINAL
			case 2:
				::the_app.exit(EXIT_ID_GOTO_FIRM_PROG_LAST_BUILD, (int)E_APP_ID::FIRM_PROG);
#ifndef ESP32
				Serial2.reopen(); // try to re-open the Serial device (in case of re-attaching device)
#endif
				break; // last menu
			}
			::the_sys_console.clear_screen();
			::twe_prog.reset_module();
			break;

		case SubScreen::EXIT_PREV:
			::the_app.exit(EXIT_ID_GOTO_FIRM_PROG_LAST_BUILD, (int)E_APP_ID::FIRM_PROG);
			break;

		case SubScreen::EXIT_BACK_TO_MENU:
		default:
			::the_app.exit(APP_ID);
			break;
		}
		break;

	case Screen_FatalError::SCR_ID:
		::the_app.exit(APP_ID);
		break;
	}

	return 0;
}

void App_FirmProg::setup() {
	// preference
	the_settings_menu.begin(appid_to_slotid(APP_ID));

	// default vars
	if (_firmfile_dir.size() == 0) {
		_firmfile_dir << get_dir_tweapps();
	}
	
	setup_screen();
	_subscr.setup(change_app);
	_subscr.exit(-1, Screen_ModIdentify::SCR_ID);

#ifndef ESP32
	// store dir drop dir into internal dirs
	_dirname_drop.resize(0);
	_dirname_drop << the_file_drop.get_dir();
#endif
}

void App_FirmProg::loop() {
#ifndef ESP32
	// dropped file
	if (the_file_drop.available()) {
		_dirname_drop.resize(0);
		_dirname_drop << the_file_drop.get_dir();
		_b_drop_available = true;
	}
#endif
	_subscr.loop();
}

void App_FirmProg::idenfify_module() {
	the_screen << "\033[2J" << "Identify Module";
	_b_running_tweprog = twe_prog.begin(TweProg::BL_PROTOCOL_GET_MODULE_INFO);
}

void App_FirmProg::setup_screen() {
	// LCD font color
	default_bg_color = color565(sAppData.u32_TWESTG_STAGE_BG_COLOR); // color565(90, 0, 50); 
	default_fg_color = color565(sAppData.u32_TWESTG_STAGE_FG_COLOR);

	// font register (note: to save flash area, don't create too much!)
	TWEFONT::createFontMP10_std(1, 0, 0); // MP10 font

	TWEFONT::createFontShinonome16_mini(10, 0, 0, TWEFONT::U32_OPT_FONT_TATEBAI); // shinonome 16 font (TATE BAIKAKU)
	TWEFONT::createFontShinonome16_mini(11, 1, 0); // shinonome 16 font

#ifndef ESP32
	// load fullset
	TWEFONT::createFontMP12(12, 0, 0, TWEFONT::U32_OPT_FONT_TATEBAI | TWEFONT::U32_OPT_FONT_YOKOBAI); // MP12 font
	TWEFONT::createFontMP12(13, 0, 0); // MP10 font
#else
	TWEFONT::createFontMP12_mini(12, 0, 0, TWEFONT::U32_OPT_FONT_TATEBAI | TWEFONT::U32_OPT_FONT_YOKOBAI); // MP12 font
	TWEFONT::createFontMP12_mini(13, 0, 0); // MP10 font
#endif

	// main screen area
	the_screen.set_font(13);
	the_screen.set_color(default_fg_color, default_bg_color);
	the_screen.set_cursor(0); // 0: no 1: curosr 2: blink cursor
	the_screen.force_refresh();

	// middle area
	the_screen_l.set_font(13);
	the_screen_l.set_color(default_fg_color, BLACK);
	the_screen_l.set_cursor(0); // 0: no 1: curosr 2: blink cursor
	the_screen_l.set_color_table(COLTBL_MAIN); // set color palette
	the_screen_l.force_refresh();

	// bottom area
	the_screen_b.set_font(1);
	the_screen_b.set_color(color565(80, 80, 80), color565(20, 20, 20));
	the_screen_b.set_cursor(0);
	the_screen_b.set_color_table(COLTBL_MAIN); // set color palette
	the_screen_b.force_refresh();

	// bottom area
	the_screen_c.set_font(1);
	the_screen_c.set_color(ORANGE, color565(20, 20, 20));
	the_screen_c.set_cursor(0);
	the_screen_c.force_refresh();

	// top area
	the_screen_t.set_font(11);
	the_screen_t.set_color(default_bg_color, default_fg_color);
	the_screen_t.set_cursor(0);
	the_screen_t.force_refresh();
}

// screen refresh timing (every 32ms)
void App_FirmProg::screen_refresh() {
	static uint32_t u32mills;

	uint32_t u32now = millis();
	if (u32now - u32mills > 32) {
		the_screen.refresh();
		the_screen_l.refresh();
		the_screen_b.refresh();
		the_screen_c.refresh();
		the_screen_t.refresh();

		u32mills = u32now;
	}
}

// change screen font
void App_FirmProg::change_screen_font() {
	static uint8_t idx = 1;
	uint8_t modes[] = { 10, 11, 12, 13 }; // toggle screen 10 .. 13

	the_screen.set_font(modes[idx & 0x3]);
	the_screen.clear_screen();
	the_screen.force_refresh();

	auto font = TWEFONT::queryFont(the_screen.font_id());
	the_screen_b.clear_screen();
	TWE::fPrintf(the_screen_b, "\nFont: %s\n      ID=%d H:%d W:%d W_CHRs:%d",
		font.font_name, font.get_font_code(), font.height, font.width, font.font_wide_count);

	idx++;
}

// change screen color
void App_FirmProg::change_screen_color() {
	static bool i = false;
	i = !i;

	if (i) {
		the_screen.set_color(ORANGE, color565(0x20, 0x20, 0x20));
		the_screen.clear_screen();
	}
	else {
		the_screen.set_color(default_fg_color, default_bg_color);
		the_screen.clear_screen();
	}

	the_screen.force_refresh();
}

#ifndef ESP32
int App_FirmProg::Screen_OpenMenu::_i_selected = -1; // the last selected item
void App_FirmProg::Screen_OpenMenu::setup() {

	the_screen_t.clear_screen();
	the_screen_t << "\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m アプリ書換";

	// NOTE: MUST ADD ORDER OF ID NUMBER OF MENU_??? 
	_listMenu.push_back(L"BINから選択", uint16_t(MENU_REGULAR_APP));
	
	_listMenu.push_back(L"Actビルド&書換", uint16_t(MENU_ACT));
	
	_listMenu.push_back(L"TWELITE APPSビルド&書換", uint16_t(MENU_TWEAPPS));
	
	_listMenu.push_back(L"Actエクストラ", uint16_t(MENU_ACT_EXTRA));

	_listMenu.push_back(L"指定", uint16_t(MENU_DROP_DIR));
	update_dropmenu();

	_listMenu.push_back(L"直前", uint16_t(MENU_LAST));
	update_dropmenu();

	// if constructor option is passed, choose the one
	if (_parent->_n_opt_construct == EXIT_ID_GOTO_FIRM_PROG_LAST_BUILD) {
		_parent->_n_opt_construct = 0; // clear opt
		_i_selected = MENU_LAST;
		// the_keyboard.push(TWECUI::KeyInput::KEY_ENTER);
	}

#if !(defined(ESP32) || defined(MWM5_BUILD_RASPI))
	_listMenu.set_info_area(L"ﾌｫﾙﾀﾞ", L"ｳｪﾌﾞ");
#endif
	_listMenu.attach_term(the_screen);

	_listMenu.set_view(0, _i_selected >= 0 && _i_selected < _listMenu.size() ? _i_selected : -1);
	_listMenu.update_view(true);
	update_navigation();

	_parent->_b_drop_available = true; // force to update drop menu at loop()
}

void App_FirmProg::Screen_OpenMenu::update_navigation() {
	int isel = _listMenu.get_selected_index();
	int iext = isel >= 0 ? _listMenu.get_selected().second[0] : -1;

	switch (iext) {
	case MENU_REGULAR_APP:
	case MENU_ACT:
	case MENU_ACT_EXTRA:
	case MENU_TWEAPPS:
		the_screen_c.clear_screen();
#if defined(ESP32) || defined(MWM5_BUILD_RASPI)
		the_screen_c << "     ↑/長押:MENU          選択/--                ↓/--";
#else
		the_screen_c << "     ↑/長押:MENU          選択/--                ↓/ﾌｫﾙﾀﾞ";
#endif
		break;
	case MENU_DROP_DIR:
	case MENU_LAST:
		the_screen_c.clear_screen();
#if defined(ESP32) || defined(MWM5_BUILD_RASPI)
		the_screen_c << "     ↑/長押:MENU          選択/--                ↓/--";
#else
		the_screen_c << "     ↑/長押:MENU          選択/--                ↓/";
		the_screen_c << (sAppData.u8_TWESTG_STAGE_OPEN_CODE ? "VSCode" : "ﾌｫﾙﾀﾞ");
#endif
		break;
	default:
		the_screen_c.clear_screen();
		the_screen_c << "     ↑/長押:MENU          選択/--                ↓/--";
		break;
	}
}

void App_FirmProg::Screen_OpenMenu::update_dropmenu() {
	{
		auto& l = _listMenu[MENU_DROP_DIR].first;
		
		auto& dirdrop = _parent->_dirname_drop;
		int dirdrop_len = dirdrop.size();

#if defined(ESP32) || defined(MWM5_BUILD_RASPI)
		l = L"---";
#else
		if (dirdrop_len == 0) {
			l = L"指定 [ﾌｫﾙﾀﾞをﾄﾞﾛｯﾌﾟ]";
		}
		else {
			l = L"指定 [";

			if (dirdrop_len > 32) {
				l << std::pair(dirdrop.begin(), dirdrop.begin() + 10);
				l << L"...";
				l << std::pair(dirdrop.end() - 20, dirdrop.end());
				l << L']';
			}
			else {
				l << dirdrop << ']';
			}
		}
#endif
	}

	{
		auto& l = _listMenu[MENU_LAST].first;
		l = L"再書換";
		bool b_menu_none = false;

		switch(_parent->_last_menu_number) {
		case MENU_NONE:
			b_menu_none = true;
			break;
		case MENU_DROP_DIR:
			if (_listMenu[MENU_DROP_DIR].first.length() > 5) {
				l << L" [" << std::pair(_listMenu[MENU_DROP_DIR].first.begin() + 4, _listMenu[MENU_DROP_DIR].first.end() - 1) << L']';
			}
			else {
				l << L" []";
			}
			break;
		case MENU_REGULAR_APP:
			if (_parent->_firmfile_disp.size() == 0) {
				b_menu_none = true;
			}
			else {
				l << L" [" << _parent->_firmfile_disp << L']';
			}
			break;
		case MENU_ACT:
		case MENU_ACT_EXTRA:
		case MENU_TWEAPPS:
			if (_parent->_build_name.size() == 0) {
				b_menu_none = true;
			} else {
				l << L" [" << _parent->_build_name << L']';
			}
			break;
		}

		if (b_menu_none) {
			l << L" [無し]";
		}
	}

	_listMenu.update_view(true);
}

void App_FirmProg::Screen_OpenMenu::loop() {
	if (_parent->_b_drop_available) {
		update_dropmenu();
		_parent->_b_drop_available = false;
	}

	do {
		int c = the_keyboard.read();

		if (_listMenu.size() > 0 && _listMenu.key_event(c)) {
			update_navigation();

			if (auto ibtn = _listMenu.is_info_selected()) {
				int iext = _listMenu.get_selected().second[0];

				if (ibtn & 2) {
					// open info
					switch(iext) {
					case MENU_REGULAR_APP: shell_open_url("https://stage.twelite.info/usage/screens/main_menu/firm_prog/bin"); break;
					case MENU_ACT: shell_open_url("https://stage.twelite.info/usage/screens/main_menu/firm_prog/act_build"); break;
					case MENU_ACT_EXTRA: shell_open_url("https://stage.twelite.info/usage/screens/main_menu/firm_prog/act_extras_build"); break;
					case MENU_TWEAPPS: shell_open_url("https://stage.twelite.info/usage/screens/main_menu/firm_prog/tweapps_build"); break;
					case MENU_DROP_DIR: shell_open_url("https://stage.twelite.info/usage/screens/main_menu/firm_prog/zhi-ding"); break;
					case MENU_LAST: shell_open_url("https://stage.twelite.info/usage/screens/main_menu/firm_prog/zai-shu-huan"); break;
					}
				} else if (ibtn & 1) {
					bool bErrMsg = false;
					// open folder
					switch(iext) {
					case MENU_REGULAR_APP: shell_open_folder(get_dir_tweapps()); break;
					case MENU_ACT: shell_open_folder(the_cwd.get_dir_wks_acts()); break;
					case MENU_ACT_EXTRA: shell_open_folder(the_cwd.get_dir_wks_act_extras()); break;
					case MENU_TWEAPPS: shell_open_folder(the_cwd.get_dir_wks_tweapps()); break;
					case MENU_DROP_DIR: if(_parent->_dirname_drop.length() > 0) shell_open_folder(_parent->_dirname_drop); else bErrMsg = true; break;
					case MENU_LAST: bErrMsg = true; break;
					}

					if (bErrMsg) the_screen_b << crlf << "\033[31mFolder cannot be opened.\033[0m";
				}
			} else
			if (_listMenu.is_selection_completed()) {
				auto& l = _listMenu.get_selected().first;
				int isel = _listMenu.get_selected_index();
				int iext = _listMenu.get_selected().second[0];

				_i_selected = _listMenu.get_selected_index();

				// 直前のメニューがドロップディレクトリの場合、そのメニューを選択
				if (_parent->_last_menu_number == MENU_DROP_DIR && iext == MENU_LAST) {
					iext = MENU_DROP_DIR;
				}

				switch (iext) {
				case MENU_REGULAR_APP:
					//_parent->_firmfile_dir.clear();
					_parent->_firmfile_dir = as_copying(get_dir_tweapps());

					_parent->_last_menu_number = MENU_REGULAR_APP;
					exit(EXIT_NEXT); // move to dir list
					break;

				case MENU_DROP_DIR:
					if (_parent->_dirname_drop.size() > 0) {
						_parent->_last_menu_number = MENU_DROP_DIR;

						// find build dir -> building
						if (TweDir::is_dir(make_full_path(_parent->_dirname_drop, L"BUILD").c_str())) {
							_parent->_build_project_prev.clear();
							_parent->_build_project.clear();
							split_fullpath_into_dir_file(_parent->_dirname_drop
								, _parent->_build_workspace
								, _parent->_build_name);

							exit(EXIT_NEXT3, Screen_ActBuild::OPT_START_BUILD_ACT);
							break;
						}

						// find .BIN files
						{
							TweDir dir;
							auto files = dir.find_files(_parent->_dirname_drop.c_str(), L".BIN");
							if (files.size() > 0) {
								_parent->_firmfile_dir = as_copying(_parent->_dirname_drop); // set browse
								exit(EXIT_NEXT); // move to dir list (firmware write)
								break;
							}
						}

						_parent->_build_workspace = as_copying(_parent->_dirname_drop);
						_parent->_build_project_prev.clear();
						_parent->_build_project.clear();
						_parent->_build_name.clear();
						exit(EXIT_NEXT3, Screen_ActBuild::OPT_START_DIR_LIST_ACT);
					}
					else {
						l = L"選択不可";
						_listMenu.clear_completion(); // cancel complete state
						_listMenu.update_view(true);
					}
					break;

				case MENU_ACT: // ACT BUILD
					_parent->_last_menu_number = MENU_ACT;
					_parent->_build_workspace = as_copying(the_cwd.get_dir_wks_acts());
					_parent->_build_project_prev.clear();
					_parent->_build_project.clear();
					exit(EXIT_NEXT3, Screen_ActBuild::OPT_START_DIR_LIST_ACT);
					break;

				case MENU_ACT_EXTRA: // ACT BUILD
					_parent->_last_menu_number = MENU_ACT_EXTRA;
					_parent->_build_workspace = as_copying(the_cwd.get_dir_wks_act_extras());
					_parent->_build_project_prev.clear();
					_parent->_build_project.clear();
					exit(EXIT_NEXT3, Screen_ActBuild::OPT_START_DIR_LIST_ACT);
					break;

				case MENU_TWEAPPS:
					_parent->_last_menu_number = MENU_TWEAPPS;
					_parent->_build_workspace = as_copying(the_cwd.get_dir_wks_tweapps());
					_parent->_build_project_prev = as_moving(_parent->_build_project);
					_parent->_build_project.clear();
					exit(EXIT_NEXT3, Screen_ActBuild::OPT_START_DIR_LIST_TWEAPPS);
					break;

				case MENU_LAST: // PREVIOUS
					switch (_parent->_last_menu_number) {
					case MENU_DROP_DIR:
					case MENU_REGULAR_APP:
						if (_parent->_firmfile_disp.size() > 0 && _parent->_firmfile_name.size() > 0) {
							exit(EXIT_NEXT2); // move to prog
							return;
						}
						break;
					case MENU_ACT_EXTRA:
					case MENU_ACT:
						if (_parent->_build_name.size() > 0) {
							exit(EXIT_NEXT3, Screen_ActBuild::OPT_START_BUILD_ACT); // last selected act
							return;
						}
						break;
					case MENU_TWEAPPS:
						if (_parent->_build_name.size() > 0) {
							exit(EXIT_NEXT3, Screen_ActBuild::OPT_START_BUILD_TWEAPPS); // last selected tweapps
							return;
						}
						break;
					}
					l = L"選択不可";
					_listMenu.clear_completion(); // cancel complete state
					_listMenu.update_view(true);
					break;

				default:
					exit(EXIT_BACK_TO_MENU);
				}
			}
		} 
		else switch (c) {
		case KeyInput::KEY_ESC:
			exit(EXIT_BACK_TO_MENU);
			return;

		case KeyInput::KEY_BUTTON_A:
			the_keyboard.push(TWECUI::KeyInput::KEY_UP);
			break;

		case KeyInput::KEY_BUTTON_A_LONG:
			exit(EXIT_BACK_TO_MENU);
			return;

		case KeyInput::KEY_BUTTON_B:
			the_keyboard.push(TWECUI::KeyInput::KEY_ENTER);
			break;

		case KeyInput::KEY_BUTTON_B_LONG:
			break;

		case KeyInput::KEY_BUTTON_C:
			the_keyboard.push(TWECUI::KeyInput::KEY_DOWN);
			break;

		case KeyInput::KEY_BUTTON_C_LONG:
			// open last or drop dir, if selected the menu
			{
				switch (_listMenu.get_selected().second[0]) {
				case MENU_REGULAR_APP:
					shell_open_folder(the_cwd.get_dir_tweapps());
					break;

				case MENU_ACT_EXTRA:
				case MENU_ACT:
					shell_open_folder(the_cwd.get_dir_wks_acts());
					break;

				case MENU_TWEAPPS:
					shell_open_folder(the_cwd.get_dir_wks_tweapps());
					break;

				case MENU_DROP_DIR:
					if (TweDir::is_dir(_parent->_dirname_drop.c_str())) {
						shell_open_folder(_parent->_dirname_drop);
					}
				break;

				case MENU_LAST:
				{
					auto&& dir = make_full_path(_parent->_build_workspace, _parent->_build_project, _parent->_build_name);
					if (sAppData.u8_TWESTG_STAGE_OPEN_CODE) {
						shell_open_by_command(dir.c_str(), L"code");
					} else {
						shell_open_folder(dir.c_str());
					}
				}
				break;

				default:
				break;
				}
			}
			break;
		}
	} while (the_keyboard.available());

	// LCD update
	screen_refresh();
}
#endif

void App_FirmProg::Screen_FileBrowse::setup() {
	the_screen << "\033[2J";

	// put a init message
	the_screen_t << "\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m ｱﾌﾟﾘ書換 - ﾌｧｰﾑｳｪｱ選択";

	// button navigation
	the_screen_c.clear_screen();
	
#if defined(ESP32) || defined(MWM5_BUILD_RASPI)
	the_screen_c << "     ↑/長押:MENU          選択/--                ↓/--";
#else
	//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
	the_screen_c << "     ↑/長押:MENU          選択/--                ↓/ﾌｫﾙﾀﾞ";
#endif

	// test for list files
	_listFiles.set_info_area(nullptr);
	_listFiles.clear();

	// list files
	{
		TweDir dir;
		auto files = dir.find_files(_parent->_firmfile_dir.c_str(), L".BIN");

		for(auto&& x : files) {
			// x.first -> sys name
			// x.secound -> display name
			SmplBuf_WChar name_disp(TWE::TWE_FILE_NAME_TYPLEN);
			if (dispFileName(x.second.c_str(), name_disp)) {
				_listFiles.push_back(as_moving(name_disp), as_moving(x.first));
			}
		}
	}

	if (_listFiles.size() == 0) {
		the_screen << "ファームウェアファイルが見つかりません";
		_exit_timer.start(2000);
		_error_status = true;
	}
	else {
		_listFiles.sort_items(true);
		_listFiles.attach_term(the_screen, true);
		_listFiles.set_view();
		if (!_parent->_firmfile_name.empty()) {
			for(int i = 0; i < _listFiles.size(); i++) {
				if (!wcscmp(_parent->_firmfile_name.c_str(), _listFiles[i].second.c_str())) {
					_listFiles.update_selection_absolute(i);
				}
			}
		}
		_listFiles.update_view(true);
	}
}

bool App_FirmProg::Screen_FileBrowse::dispFileName(const wchar_t* name, SmplBuf_WChar& name_disp) {
	bool b_ok = true;

	// prepare upper string.
	SmplBuf_WCharL<128> l_name_upper;
	SmplBuf_WChar& name_upper = l_name_upper.get();
	name_upper << name;
	toUpper(name_upper);

	// model name
	const wchar_t wstr_model[][16] = {
		L"UNDEF",
		L"BLUE",
		L"RED"
	};

	// check if filename has TWELITE MODE name.
	if (wcsstr(name_upper.data(), wstr_model[(int)twe_prog.module_info.type]) == nullptr) { b_ok = false; }

	// generate return string
	if (b_ok) {
		name_disp.resize(0);
		name_disp << name;
		name_disp.resize(name_disp.size() - 4); // remove ".BIN"
	}

	return b_ok;
}

void App_FirmProg::Screen_FileBrowse::loop() {
	// Key/Menu Op
	do  {
		int c = the_keyboard.read();

		if (_listFiles.key_event(c)) {
			if (_listFiles.is_selection_completed()) {
				auto&& x = _listFiles.get_selected();
				
				_parent->_firmfile_disp = x.first;
				_parent->_firmfile_name = x.second; // actual path filename w/o dir
				
				if (_error_status) exit(EXIT_ERROR);
				else exit(EXIT_NEXT);
				return;
			}
		} else
		switch (c) {
		case KeyInput::KEY_ESC:
		case KeyInput::KEY_BUTTON_A_LONG:
			exit(EXIT_BACK_TO_MENU);
			return;

		case KeyInput::KEY_BUTTON_A:
			the_keyboard.push(TWECUI::KeyInput::KEY_UP);
			break;

			exit(EXIT_BACK_TO_MENU);
			return;

		case KeyInput::KEY_BUTTON_B:
			the_keyboard.push(TWECUI::KeyInput::KEY_ENTER);
			break;

		case KeyInput::KEY_BUTTON_B_LONG:
			break;

		case KeyInput::KEY_BUTTON_C:
			the_keyboard.push(TWECUI::KeyInput::KEY_DOWN);
			break;

		case KeyInput::KEY_BUTTON_C_LONG:
			shell_open_folder(_parent->_firmfile_dir);
			break;
		}
	} while (the_keyboard.available());

	// LCD update
	screen_refresh();
}

void App_FirmProg::Screen_FatalError::setup() {
	the_screen_t.clear();
	the_screen_t << "\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m ｱﾌﾟﾘ書換 - エラー";
	
	the_screen.clear_screen();

	the_screen
		<< crlf
		<< L"TWELITE 無線モジュールが認識できません。" << crlf
		<< L"" << crlf
		// L"....+....1a...+....2....+....3.b..+....4....+....5..3"
		<< L"同じエラーが続く場合は、TWELITE STAGEを終了し、USBデ" L"\r\n"
		   L"バイス(MONOSITCK,TWELITE R)を切断し、TWELITEの配線を" L"\r\n"
		   L"再確認します。改めてUSBデバイスの接続とSTAGEアプリを" L"\r\n"
		   L"起動してください。"  L"\r\n"
		;
			
	// button navigation
	the_screen_c.clear_screen();
	//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
	the_screen_c << "     --/長押:MENU         RETRY/--               --/--";

	auto i = _listFiles.get_selected_index();
	if (i != -1) {
		the_screen << _listFiles[i].first;
	}
}

void App_FirmProg::Screen_FatalError::loop() {
	// KEYBOARD
	while (the_keyboard.available()) {
		switch (int c = the_keyboard.read()) {
		case KeyInput::KEY_ESC:
			exit(EXIT_BACK_TO_MENU);
			return;

		case KeyInput::KEY_BUTTON_A:
			exit(EXIT_BACK_TO_MENU);
			return;

		case KeyInput::KEY_BUTTON_A_LONG:
			exit(EXIT_BACK_TO_MENU); 
			return;

		case KeyInput::KEY_ENTER:
		case KeyInput::KEY_BUTTON_B:
#ifndef ESP32
			Serial2.reopen();
#endif
			// ::the_app.exit(EXIT_ID_GOTO_FIRM_PROG_LAST_BUILD, (int)E_APP_ID::FIRM_PROG);
			::the_app.exit(0, (int)E_APP_ID::FIRM_PROG);
			return;

		case KeyInput::KEY_BUTTON_B_LONG:
			break;

		case KeyInput::KEY_BUTTON_C:
			exit(EXIT_BACK_TO_MENU);
			return;

		case KeyInput::KEY_BUTTON_C_LONG:
			break;

		default:
			if (TWECUI::KeyInput::MOUSE_UP::is_type(c)) {
				// press LEFT mouse button to proceed.
				TWECUI::KeyInput::MOUSE_UP ev(c);
				if (auto&& coord = the_screen.get_term_coord_from_screen(ev.get_x(), ev.get_y())) {				
					the_keyboard.push(KeyInput::KEY_ENTER);
				}
			}
		}
	}

	// LCD update
	screen_refresh();
}

void App_FirmProg::Screen_ModIdentify::cb_protocol(
	TweProg::E_ST_TWEBLP cmd,
	int req_or_resp,
	TWE::APIRET evarg,
	TWEUTILS::SmplBuf_Byte& payl,
	void* pobj
) {
	const char strcmd[][16] = {
		"NONE",
		"CONNECT",
		"IDENTIFY_FLASH",
		"SELECT_FLASH",
		"READ_CHIPID",
		"READ_MAC_CUSTOM",
		"FINISH",
		"FINISH_ERROR"
	};

	if (pobj) {
		App_FirmProg::Screen_ModIdentify& obj = *(App_FirmProg::Screen_ModIdentify*)pobj;
		ITerm& scr = obj.the_screen_b;

		if (req_or_resp == TweProg::EVENT_NEW_STATE) {
			scr	<< crlf
				<< strcmd[(int)cmd]
				// << "\033[46;30;1m>\033[0m\033[46;30m"
				<< "\033[7;1m>\033[0m\033[7m"
				;

			for (auto x : payl) {
				scr << printfmt("%02X", x);
			}

			scr << (evarg ? "\033[0m\033[1m""ok""\033[0m" : "\033[41;37m""ng""\033[0m");
		}
		else {
			scr	
				// << "\033[43;30;1m<\033[0m\033[43;30m"
				<< "\033[7;1m<\033[0m\033[7m"
				;

			for (auto x : payl) {
				scr << printfmt("%02X", x);
			}

			scr
				<< (evarg ? "\033[0m\033[1m""ok""\033[0m" : "\033[41;37m""ng""\033[0m");
		}
	}
}


void App_FirmProg::Screen_ModIdentify::setup() {
	the_screen_t << "\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m ｱﾌﾟﾘ書換 - ﾓｼﾞｭｰﾙ認識";
	the_screen.clear_screen();

	twe_prog.add_cb(cb_protocol, (void*)this);
	start_protocol();
}

void App_FirmProg::Screen_ModIdentify::loop() {
	// handle the protocol
	while (_b_protocol) {
		int c = 0;

		if (the_uart_queue.available()) c = the_uart_queue.read();
		else if (_timer.available()) {
			_timer.start();
			c = -1; // periodic time out check
		}
		else break;
			
		if (c >= 0) {
#if 0
			the_screen_b << printfmt("%02X", c);

			static int ct;
			ct++;
			if (!(ct % 16)) the_screen_b << crlf;
#endif
		}

		if (twe_prog.process_input(c)) {
			_b_protocol = false;

			// discard the rest of input
			while (the_uart_queue.available()) the_uart_queue.read();

			// finished or error
			auto stat = twe_prog.get_state();

			the_screen << crlf;
			the_screen << printfmt("Exit Status = %02x", (int)stat);

			if (stat == TweProg::E_ST_TWEBLP::FINISH) {
				const char names_modtype[][16] = {
					"NONE",
					"\033[44mBLUE\033[0m",
					"\033[41mRED\033[0m"
				};

				// if the name of the last file does not match the detected model,
				// reset those information.
				if (_parent->_firmfile_modtype != twe_prog.module_info.type) {
					_parent->_firmfile_disp.resize(0);
					_parent->_firmfile_name.resize(0);
					_parent->_firmfile_modtype = twe_prog.module_info.type;
				}

				// display info
				the_screen_l << printfmt("ｼﾘｱﾙ番号=\033[7m%07X\033[0m TWELITE=%s"
					, twe_prog.module_info.serial_number & 0x0FFFFFFF
					, names_modtype[(int)twe_prog.module_info.type]
				);

				exit(EXIT_NEXT);
			}
			else {
				exit(EXIT_ERROR);
			}
		}
	}

	// KEYBOARD
	if (the_keyboard.available()) {
		int c = the_keyboard.read();

		switch (c) {
		case KeyInput::KEY_ENTER:
			start_protocol();
			break;

		case KeyInput::KEY_BUTTON_A:
			exit(EXIT_BACK_TO_MENU);
			return;

		case KeyInput::KEY_BUTTON_A_LONG:
			break;

		case KeyInput::KEY_BUTTON_B:
			start_protocol();
			break;

		case KeyInput::KEY_BUTTON_B_LONG:
			break;

		case KeyInput::KEY_BUTTON_C:
			break;

		case KeyInput::KEY_BUTTON_C_LONG:
			break;
		}
	}

	// LCD update
	screen_refresh();
}


void App_FirmProg::Screen_ModIdentify::start_protocol() {
	// if the protocol is still on going, don't proceed it.
	if (_b_protocol) return;
	
#ifndef ESP32
	Serial2.reopen();
#endif

	// flush rx buffer at first.
	twe_prog.reset_hold_module(); // hold module (to stop serial message)
	update_serial_keyb_input(true); // update input queue (grab remaining buffer data into the_uart_queue)
	while (the_uart_queue.available()) (void)the_uart_queue.read();
	while (the_keyboard.available()) (void)the_keyboard.read();

	// identify module
	the_screen.clear_screen();
	the_screen << "Get Module Info(tim=" << int((millis() >> 10) & 0xffff) << ')';
	twe_prog.begin(TweProg::BL_PROTOCOL_GET_MODULE_INFO);

	_b_protocol = true;
	_timer.start(32);
}

void App_FirmProg::Screen_FileProg::setup() {
	bool bErrorFileRead = false;

	the_screen << "\033[2J";

	// put a init message
	the_screen_t.clear_screen();
	the_screen_t << "\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m ｱﾌﾟﾘ書換 - 書換中";

	// button navigation
	the_screen_c.clear_screen();
	//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
	the_screen_c << "     ↑/長押:MENU          書換/--                ↓/--";

	// error
	if (_parent->_firmfile_dir.size() == 0 || _parent->_firmfile_name.size() == 0) {
		bErrorFileRead = true;
	}

	// open the file
	if (!bErrorFileRead) {
		SmplBuf_WChar fname(TWE::TWE_FILE_NAME_MAX);

		fname << _parent->_firmfile_dir;
		fname << WCHR_PATH_SEP;
		fname << _parent->_firmfile_name;

		_file.reset(new TweFile(4096));
		_file->open(fname);

		if (!twe_prog.set_firmware_data(_file)) bErrorFileRead = true;
	}

	if (!bErrorFileRead) {
		APP_HNDLR::new_hndlr(&Screen_FileProg::hndlr_start);
	} else {
		the_screen << L"ファイルの読み込みに失敗しました";
		screen_refresh();

		// exiting
		APP_HNDLR::new_hndlr(&Screen_FileProg::hndlr_waitexit, 3000);
	}
}

void App_FirmProg::Screen_FileProg::hndlr_start(event_type ev, arg_type arg) {
	switch (ev) {
	case EV_SETUP:
		// clear twe_prog state (it should be FINISHED, then set NONE)
		twe_prog.clear_state();

		// add callback.
		twe_prog.add_cb(cb_protocol, (void*)this);

		// start firm write
		start_protocol();
		break;

	case EV_LOOP:
		// handle the protocol
		while (_b_protocol) {
			int c = 0;

			// incoming uart data.
			if (the_uart_queue.available()) c = the_uart_queue.read();
			else if (_timer_null.available()) {
				c = -1; // periodic time out check
				_timer_null.start();
			}
			else {
				break;
			}

			if (twe_prog.process_input(c)) {
				_b_protocol = false;

				// discard the rest of input
				while (the_uart_queue.available()) the_uart_queue.read();

				// finished or error
				auto stat = twe_prog.get_state();

				if (stat == TweProg::E_ST_TWEBLP::FINISH) {
					APP_HNDLR::new_hndlr(&Screen_FileProg::hndlr_success);
				}
				else {
					APP_HNDLR::new_hndlr(&Screen_FileProg::hndlr_error);
				}
				return;
			}
		}
		break;

	case EV_EXIT:
		break;
	}
}

void App_FirmProg::Screen_FileProg::hndlr_success(event_type ev, arg_type arg) {
	switch (ev) {
	case EV_SETUP:
		the_screen << crlf;
		the_screen << L"書き込みに成功しました";
		the_screen << printfmt("(%dms)", millis() - _u32_tickstart);
		the_screen << crlf << crlf;
		the_screen << L"\033[7m中ボタン\033[0mまたは[\033[7mEnter\033[0m]で" << crlf;
		if (sAppData.u8_TWESTG_STAGE_APPWRT_BUILD_NEXT_SCREEN == 0) {
			the_screen << L"ﾀｰﾐﾅﾙ(ｲﾝﾀﾗｸﾃｨﾌﾞﾓｰﾄﾞ用)を開きます。";
		} else {
			the_screen << L"ﾀｰﾐﾅﾙを開きます。";
		}

		// button navigation
		the_screen_c.clear_screen();
		if (sAppData.u8_TWESTG_STAGE_APPWRT_BUILD_NEXT_SCREEN == 0) {
			//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
			the_screen_c << "     ↑/長押:MENU     " 
									   "\033[42;30m成功:設定\033[0m"
														   "/--                ↓/--";
		}
		else if (sAppData.u8_TWESTG_STAGE_APPWRT_BUILD_NEXT_SCREEN == 1) {
			//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
			the_screen_c << "     ↑/長押:MENU    " 
									  "\033[42;30m成功:ﾀｰﾐﾅﾙ\033[0m"
														   "/--                ↓/--";
		}
		else {
			//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
			the_screen_c << "     ↑/長押:MENU    " 
									  "\033[42;30m成功:戻る\033[0m"
														  "/--                ↓/--";

		}
		break;

	case EV_LOOP:
		// KEYBOARD
		switch (int c = the_keyboard.read()) {
		case KeyInput::KEY_ESC:
		case KeyInput::KEY_ENTER:
			exit(EXIT_NEXT);
			break;

		case KeyInput::KEY_BUTTON_A:
		case KeyInput::KEY_BUTTON_A_LONG:
			exit(EXIT_BACK_TO_MENU);
			break;

		case KeyInput::KEY_BUTTON_B:
			the_keyboard.push(KeyInput::KEY_ENTER);
			break;

		case KeyInput::KEY_BUTTON_B_LONG:
		case KeyInput::KEY_BUTTON_C:
		case KeyInput::KEY_BUTTON_C_LONG:
			break;

		default:
			if (TWECUI::KeyInput::MOUSE_UP::is_type(c)) {
				// press LEFT mouse button to proceed.
				TWECUI::KeyInput::MOUSE_UP ev(c);
				if (auto && coord = the_screen.get_term_coord_from_screen(ev.get_x(), ev.get_y())) {				
					the_keyboard.push(KeyInput::KEY_ENTER);
				}
			}
		}
		break;

	case EV_EXIT:
		break;
	}
}

void App_FirmProg::Screen_FileProg::hndlr_error(event_type ev, arg_type arg) {
	switch (ev) {
	case EV_SETUP:
		the_screen << crlf << "\033[31;7m" << L"書き込みに失敗しました" << "\033[0m";

		the_screen_c.clear_screen();
		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << "     ↑/長押:MENU        再書換/--                ↓/--";
		break;

	case EV_LOOP:
		// KEYBOARD
		switch (int c = the_keyboard.read()) {
		case KeyInput::KEY_ENTER:
			//if (!_b_protocol) {	start_protocol(); } // try again (NG: may cause hung up.)
			exit(EXIT_PREV); // back to menu w/ last app.
			break;

		case KeyInput::KEY_ESC:
		case KeyInput::KEY_BUTTON_A_LONG:
			exit(EXIT_BACK_TO_MENU);
			break;

		case KeyInput::KEY_BUTTON_B:
			the_keyboard.push(KeyInput::KEY_ENTER);
			break;

		case KeyInput::KEY_BUTTON_B_LONG:
		case KeyInput::KEY_BUTTON_C:
		case KeyInput::KEY_BUTTON_C_LONG:
			break;
		
		default:
			if (TWECUI::KeyInput::MOUSE_UP::is_type(c)) {
				// press LEFT mouse button to proceed.
				TWECUI::KeyInput::MOUSE_UP ev(c);
				if (auto&& coord = the_screen.get_term_coord_from_screen(ev.get_x(), ev.get_y())) {				
					the_keyboard.push(KeyInput::KEY_ENTER);
				}
			}
		}
		break;

	case EV_EXIT:
		break;
	}
}

void App_FirmProg::Screen_FileProg::hndlr_waitexit(event_type ev, arg_type arg) {
	switch (ev) {
	case EV_SETUP:
		_timer_exit.start(3000);
		break;

	case EV_LOOP:
		if (_timer_exit.is_timeout()) {
			exit(EXIT_BACK_TO_MENU);
		}
		break;

	case EV_EXIT:
		break;
	}
}

void App_FirmProg::Screen_FileProg::loop() {
	// loop by APP_HNDLR
	APP_HNDLR::loop();

	// LCD update
	screen_refresh();
}

void App_FirmProg::Screen_FileProg::start_protocol() {
	// if the protocol is still on going, don't proceed it.
	if (_b_protocol) return;

	// flush rx buffer at first.
	while (the_uart_queue.available()) (void)the_uart_queue.read();
	while (the_keyboard.available()) (void)the_keyboard.read();

	// set timer
	_u32_tickstart = millis();

	// set the firmware data (at head)
	twe_prog.rewind_firmware_data();

	// start protocol
	the_screen.clear_screen();
	the_screen	<< crlf << '['
				<< _firmfile_disp << ']';

	twe_prog.begin(TweProg::BL_PROTOCOL_ERASE_AND_WRITE);

	_b_protocol = true;

	_timer_null.start(32);
}

void App_FirmProg::Screen_FileProg::cb_protocol(
	TweProg::E_ST_TWEBLP cmd,
	int req_or_resp,
	TWE::APIRET evarg,
	TWEUTILS::SmplBuf_Byte& payl,
	void* pobj
) {
	const char strcmd[][16] = {
		"NONE",
		"CONNECT",
		"IDENTIFY_FLASH",
		"SELECT_FLASH",
		"READ_CHIPID",
		"READ_MAC_CUSTOM",
		"ERASE_FLASH",
		"WRITE_FLASH",
		"VERIFY_FLASH"
	};
	
	if (pobj) {
		App_FirmProg::Screen_FileProg& obj = *(App_FirmProg::Screen_FileProg*)pobj;
		ITerm& scm = obj.the_screen; // for main
		ITerm& scp = obj.the_screen_b; // for protocol

		if (req_or_resp == TweProg::EVENT_NEW_STATE) {
			if (cmd == TweProg::E_ST_TWEBLP::WRITE_FLASH_FROM_FILE) {
				scm << crlf << crlf << L"\033[Kファームウェアを書き込んでいます..." << crlf;
				scp << crlf;
			} else
			if (cmd == TweProg::E_ST_TWEBLP::VERIFY_FLASH) {
				scm << L"\033[A\033[G\033[K書き込み内容確認(ベリファイ)中..." << crlf;
				scp << crlf;
			} else {
				scp << crlf
					<< strcmd[(int)cmd]
					<< "\033[7;1m>\033[0m\033[7m"
					;

				for (auto x : payl) {
					scp << printfmt("%02X", x);
				}

				scp << (evarg ? "\033[0m\033[1m""ok""\033[0m" : "\033[41;37m""ng""\033[0m");
			}
		}
		else {
			const int barmax = 32;
			if (cmd == TweProg::E_ST_TWEBLP::WRITE_FLASH_FROM_FILE || cmd == TweProg::E_ST_TWEBLP::VERIFY_FLASH) {
				int progres = (evarg.get_value() + barmax / 2) / 32; // 0..1023 -> 0..16

				if (cmd == TweProg::E_ST_TWEBLP::WRITE_FLASH_FROM_FILE)
					scm << "\033[G|\033[44;30m"; // write
				else
					scm << "\033[G|\033[42;30m"; // verify

				for (int i = 0; i < progres && i < barmax; i++)
					scm << ' ';
				scm << "\033[0m";
				for (int i = progres; i < barmax; i++)
					scm << ' ';
				scm << "|";

				int progress_percent = (evarg.get_value() * 100 + 50) >> 10;

				scp << printfmt("\033[GPROGRESS=\033[7m%03d%%\033[0m ", progress_percent);
			}
			else {
				scp << "\033[7;1m<\033[0m\033[7m";

				for (auto x : payl) {
					scp << printfmt("%02X", x);
				}

				scp	<< (evarg ? "\033[0m\033[1m""ok""\033[0m" : "\033[41;37m""ng""\033[0m");
			}
		}
	}
}


#ifndef ESP32
void App_FirmProg::Screen_ActBuild::setup() {
	the_screen << "\033[2J";

	// put a init message
	the_screen_t.clear_screen();
	if (_opt == OPT_START_BUILD_ACT || _opt == OPT_START_DIR_LIST_ACT) {
		the_screen_t << "\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m Act ビルド";
	}
	else {
		the_screen_t << "\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m TWELITE APPS ビルド";
	}

	if (_opt == OPT_START_BUILD_ACT || _opt == OPT_START_BUILD_TWEAPPS) {
		// change to previous dir (start build)
		APP_HNDLR::new_hndlr(&Screen_ActBuild::hndlr_build);
	} else {
		// open the projects dirs
		if (std::filesystem::is_directory(make_full_path(_parent->_build_workspace, _parent->_build_project).c_str())) {
			APP_HNDLR::new_hndlr(&Screen_ActBuild::hndlr_actdir);
		} else {
			the_screen << _parent->_build_workspace << '/' << _parent->_build_project << L"が見つかりません。" << crlf;
			screen_refresh();

			// exiting
			APP_HNDLR::new_hndlr(&Screen_ActBuild::hndlr_error, 3000);
		}
	}
}

void App_FirmProg::Screen_ActBuild::loop() {
	// loop by APP_HNDLR
	APP_HNDLR::loop();

	// LCD update
	screen_refresh();
}

void App_FirmProg::Screen_ActBuild::actdir_update_bottom() {
	the_screen_c.clear_screen();

#if (defined(ESP32) || defined(MWM5_BUILD_RASPI))
	the_screen_c << "     ↑/長押:MENU          選択/--                ↓/--";
#else
	if (_desc.get_url().length()) {
		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
 		the_screen_c << "     ↑/長押:MENU          選択/\033[32m"
			                                            "ｳｪﾌﾞｻｲﾄ"
			                                             "\033[0m           ↓/";
	}
	else {
		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
	 	the_screen_c << "     ↑/長押:MENU          選択/--                ↓/";
	}

	the_screen_c << (sAppData.u8_TWESTG_STAGE_OPEN_CODE ? "VSCode" : "ﾌｫﾙﾀﾞ");
#endif
	the_screen_c.force_refresh();
}

void App_FirmProg::Screen_ActBuild::hndlr_actdir(event_type ev, arg_type arg) {
	switch (ev) {
	case EV_SETUP: {
		actdir_update_bottom();

		TweDir dir;
		int idx = 0, idx_sel = -1;

		dir.open(make_full_path(_parent->_build_workspace, _parent->_build_project).c_str());
			
		if (dir.is_opened()) {
			SmplBuf_WCharL<TWE_FILE_NAME_MAX> l_filename_sys;
			SmplBuf_WChar& filename_sys = l_filename_sys.get();
			SmplBuf_WCharL<TWE_FILE_NAME_MAX> l_filename_disp;
			SmplBuf_WChar& filename_disp = l_filename_disp.get();

			_listFiles.clear();
			
			while (1) {
				l_filename_sys.get().resize(0);
				l_filename_disp.get().resize(0);

				bool b = dir.get_next(filename_sys, filename_disp, TweDir::OPT_LIST_DIRS);
		
				// end of dir entry
				if (!b) break;

				// skip Common and starting with `.'.
				if (filename_sys.size() > 0
					&& (   !_SmplBuf_SCompare(filename_sys, SmplBuf_WChar(L"COMMON"), [](wchar_t z) { return toupper(z); })
						|| !_SmplBuf_SCompare(filename_sys, SmplBuf_WChar(L"SOURCE"), [](wchar_t z) { return toupper(z); })
					    || filename_sys[0] == L'.'
					   )
				   ) {
					continue;
				}

				
				// check the file name and add into the list.
				_listFiles.push_back(filename_disp, filename_sys);
				idx++;
			}
		}
		else {
			// if dir is not opened.
			_listFiles.clear();
		}

		if (_listFiles.size() == 0) {
			the_screen.clear_screen();
			the_screen << crlf  << L" Act/ｱﾌﾟﾘｿｰｽが見つかりません。";
			screen_refresh();

			// exiting
			APP_HNDLR::new_hndlr(&Screen_ActBuild::hndlr_error, 3000);
		}
		else {
			_listFiles.sort_items(true); // sort without case
			_listFiles.attach_term(the_screen, true);
#if !(defined(ESP32) || defined(MWM5_BUILD_RASPI))			
			_listFiles.set_info_area(sAppData.u8_TWESTG_STAGE_OPEN_CODE ? L"VSCode" : L"ﾌｫﾙﾀﾞ", L"ｳｪﾌﾞ");
#endif
			_listFiles.set_view();

			// select last build item.
			if (!_build_project_prev.empty()) {
				for (int i = 0; i < _listFiles.size(); i++) {
					if (!wcscmp(_listFiles[i].first.c_str(), _build_project_prev.c_str())) {
						_listFiles.update_selection_absolute(i);
						break;
					}
				}
			}

			if (!_build_name.empty()) {
				bool b_found = false;
				for (int i = 0; i < _listFiles.size(); i++) {
					if (!wcscmp(_listFiles[i].first.c_str(), _build_name.c_str())) {
						_listFiles.update_selection_absolute(i);
						b_found = true;
						break;
					}
				}

				if (!b_found) _build_name.clear(); // if not found, clear it.
			}

			//_listFiles.update_view(true);
		}
	} break;

	case EV_LOOP:
		// KEYBOARD
		do {
			int c = the_keyboard.read();

			if (_listFiles.key_event(c)) {				
				int isel = _listFiles.get_selected_index();
				if (isel >= 0) {
					// try to open the desc file
					_desc.load(make_full_path(
						_parent->_build_workspace, _parent->_build_project,
						_listFiles.get_selected().first, L"000desc.txt").c_str());

					// clear desc area
					the_screen_b.clear_screen();
					the_screen_l << "\033[2;1H\033[K";

					if (_desc) {
						the_screen_l << "\033[2;1H\033[30;42m\033[K " << _desc.get_title() << "\033[0m";
						the_screen_b << "\033[32m" << _desc.get_desc() << "\033[0m";
					}
				}

				// update bottom bar.
				actdir_update_bottom();

				// selection completed
				if (auto x = _listFiles.is_info_selected()) {
					if (x & 0x01) the_keyboard.push(TWECUI::KeyInput::KEY_BUTTON_C_LONG);
					if (x & 0x02) the_keyboard.push(TWECUI::KeyInput::KEY_BUTTON_B_LONG);
					continue;
				} else
				if (_listFiles.is_selection_completed()) {
					if (TweDir::is_dir(make_full_path(
								  _parent->_build_workspace, _parent->_build_project
								, _listFiles.get_selected().first, L"build").c_str())) {
						// if build dir is found, we can build,
						_parent->_build_name = as_copying(_listFiles.get_selected().first); // copy assign (no need to add const explicitely)
						APP_HNDLR::new_hndlr(&Screen_ActBuild::hndlr_build);
					}
					else {
						// otherwise list selected dir again. (for TweApps)
						_parent->_build_project = as_copying(_listFiles.get_selected().first); // copy assign (no need to add const explicitely)
						APP_HNDLR::new_hndlr(&Screen_ActBuild::hndlr_actdir); // open again
					}
				}
			}
			else {
				switch (c) {
				case KeyInput::KEY_ESC:
				case KeyInput::KEY_BUTTON_A_LONG:
					if (_parent->_build_project.empty()) {
						exit(EXIT_BACK_TO_MENU);
					}
					else {
						_parent->_build_project_prev = as_moving(_parent->_build_project);
						_parent->_build_project.clear();
						APP_HNDLR::new_hndlr(&Screen_ActBuild::hndlr_actdir); // open again
					}
					return;

				case KeyInput::KEY_BUTTON_A:
					the_keyboard.push(TWECUI::KeyInput::KEY_UP);
					break;

				case KeyInput::KEY_BUTTON_B:
					the_keyboard.push(TWECUI::KeyInput::KEY_ENTER);
					break;

				case KeyInput::KEY_BUTTON_B_LONG:
					if (_desc.get_url().length() > 0) {
						shell_open_url(_desc.get_url());
						SmplBuf_ByteSL<1024> url;
					}
					break;

				case KeyInput::KEY_BUTTON_C:
					the_keyboard.push(TWECUI::KeyInput::KEY_DOWN);
					break;

				case KeyInput::KEY_BUTTON_C_LONG:
					if (_listFiles.get_selected_index() >= 0) {
						auto&& dir = make_full_path(_parent->_build_workspace, _parent->_build_project, _listFiles.get_selected().first).c_str();
						if (sAppData.u8_TWESTG_STAGE_OPEN_CODE) {
							shell_open_by_command(dir, L"code");
						} else {
							shell_open_folder(dir);
						}
					}
					break;

				default:
					break;
				} // switch
			} // if() else
		} while (the_keyboard.available());
		break;

	case EV_EXIT:
		break;
	} // switch
}

void App_FirmProg::Screen_ActBuild::hndlr_build(event_type ev, arg_type arg) {
	switch (ev) {
	// pre-condition
	// - sdk dir is confirmed.
	// - _parent->_build_name is set.
	case EV_SETUP: {
		the_screen_c.clear_screen();
		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
#if (defined(ESP32) || defined(MWM5_BUILD_RASPI))
		the_screen_c << "     --/長押:MENU         ﾋﾞﾙﾄﾞ/--                --/--";
#else
		the_screen_c << "     --/長押:MENU         ﾋﾞﾙﾄﾞ/--            ｴﾗｰﾛｸﾞ/";
		the_screen_c << (sAppData.u8_TWESTG_STAGE_OPEN_CODE ? "VSCode" : "ﾌｫﾙﾀﾞ");
#endif

		the_screen_c.force_refresh();

		_act_dir = make_full_path(_parent->_build_workspace, _parent->_build_project, _parent->_build_name, L"build");
		the_cwd.change_dir(_act_dir);

		// set parallel jobs count.
		int ct_cpu = sAppData.u8_TWESTG_STAGE_APPWRT_BUILD_MAKE_JOGS;
		if (ct_cpu == 0) {
			ct_cpu = TWESYS::Get_CPU_COUNT(); // Physical CPU count (or Logical/2 if failed to get Physical CPU count)
			if (ct_cpu >= 4) ct_cpu = ct_cpu - 1; // free one core not to make system too busy.
		}

		SmplBuf_ByteSL<1024> cmdstr;

#if defined(_MSC_VER) || defined(__MINGW32__) 
		#define MAKE_CMD_TERM "\""
		if (TweDir::is_dir(make_full_path(the_cwd.get_dir_sdk(), L"..\\Tools").c_str())) {
			// Tools dir is located ../{MWSDK_DIR}
			cmdstr << the_cwd.get_dir_sdk() << char(WCHR_PATH_SEP)
				<< "..\\Tools\\MinGW\\msys\\1.0\\bin\\bash -c \"/usr/bin/make";
		}
		else {
			// Tools dir is located {MWSDK_DIR}/Tools
			cmdstr << the_cwd.get_dir_sdk() << char(WCHR_PATH_SEP)
				<< "Tools\\MinGW\\msys\\1.0\\bin\\bash -c \"/usr/bin/make";
		}
#else
		#define MAKE_CMD_TERM ""
		cmdstr << "make";
#endif

		// set cpu count for parallel building
		if (ct_cpu > 1) cmdstr << " -j" << printfmt("%d", ct_cpu); // parallel jobs

		// don't use APPDEP (ALWAYS do full build.)
		cmdstr << " USE_APPDEPS=0"; 

		// set MODULE type (BLUE/RED)
		switch (_parent->_firmfile_modtype) {
		case TweProg::E_MOD_TYPE::TWELITE_BLUE: cmdstr << " TWELITE=BLUE"; break;
		case TweProg::E_MOD_TYPE::TWELITE_RED: cmdstr << " TWELITE=RED"; break;
		default: break;
		}

		if(1) { // make clean
			SmplBuf_ByteSL<1024> cleancmd;
			cleancmd = as_copying(cmdstr);
			cleancmd << " USE_APPDEPS=0 clean" MAKE_CMD_TERM;
			int i = system((const char*)cleancmd.c_str()); (void)i;// echo cmd string
		}

		// if LTO is disabled, set DISABLE_LTO=1 (affective only on Windows, others cannot set LTO by default)
		if (sAppData.u8_TWESTG_STAGE_APPWRT_FORCE_DISABLE_LTO) {
			cmdstr << " DISABLE_LTO=1"; // disabling LTO
		}

		// add redirect
		cmdstr << " 2>" << WSTR_BUILD_ERROR_LOG <<  MAKE_CMD_TERM;
		_pipe = TweCmdPipe((const char*)cmdstr.c_str());

		if(_pipe) {
			the_screen.clear_screen();
			the_screen_b.clear_screen();
			the_screen << L"compiling(コンパイル中)";

			// generate regex to capture compile message
			_re_target = std::regex(R"(!!!TARGET=([0-9a-zA-Z_\-]+\.[bB][iI][nN]))");
			//_re_gcc = std::regex(R"(bin/ba-elf-(gcc|g\+\+)(\.exe)?[ \t])");
			_re_gcc = std::regex(R"(^Compiling)"); // makefile outputs this message for each c/cpp file.
			_re_link = std::regex(R"(-Wl,--gc-sections)");
			// _re_cfile = std::regex(R"(/([a-zA-Z0-9_\-]+)\.([cC]|[cC][pP][pP])[ \t\r\n\:$])");
			_re_cfile = std::regex(R"(/([a-zA-Z0-9_\-]+)\.([cC]|[cC][pP][pP]) \.\.\.)");

		} else {
			the_screen << crlf << "\033[7m" << L"ビルドが開始できません" << "\033[0m";
			_timer_exit.start(3000);
		}

	} break;

	case EV_LOOP:
		if (_timer_exit.is_enabled()) {
			if (_timer_exit.is_timeout()) {
				APP_HNDLR::new_hndlr(&Screen_ActBuild::hndlr_actdir);
			}

			if (the_keyboard.available()) {
				int c = the_keyboard.read();

				switch (c) {
				case KeyInput::KEY_BUTTON_A_LONG:
				case KeyInput::KEY_ESC:
					APP_HNDLR::new_hndlr(&Screen_ActBuild::hndlr_actdir);
					return;

				case KeyInput::KEY_BUTTON_B:
				case KeyInput::KEY_ENTER:
					APP_HNDLR::new_hndlr(&Screen_ActBuild::hndlr_build);
					return;

				case KeyInput::KEY_BUTTON_C:
					// open builderr.log as shell function
					shell_open_default(WSTR_BUILD_ERROR_LOG);
					return;
				case KeyInput::KEY_BUTTON_C_LONG:
					// open source dir.
					if (sAppData.u8_TWESTG_STAGE_OPEN_CODE) {
						shell_open_by_command(L"..", L"code");
					} else {
						shell_open_folder(L"..");
					}
					return;
				}
			}
			// if timer is started, no further process
			break;
		}

		if (_pipe.available()) {
			SmplBuf_Byte buff;
			
			if (_pipe.readline(buff)) {
				// remove endl.
				remove_endl(buff);

				if (buff.size() > 0) {
					const char* p = (const char*)buff.begin().raw_ptr();
					const char* e = (const char*)buff.end().raw_ptr();
				
					std::cmatch m_gcc;

					if (std::regex_search(p, m_gcc, _re_gcc)) {
						std::cmatch m_file;
						if (std::regex_search(p, m_file, _re_cfile)) {
							// found gcc or g++ c/c++ file.
							the_screen << '.';
							the_screen_b
								<< crlf
								<< m_gcc[1].str().c_str()
								<< " "
								<< m_file[1].str().c_str() << "." << m_file[2].str().c_str();
						}
						else if (std::regex_search(p, _re_link)) {
							the_screen
								<< crlf
								<< L"linking(リンク中)";
						}
					}

					std::cmatch m_target;
					if (std::regex_search(p, m_target, _re_target)) {
						_act_build_file.resize(0);
						_act_build_file << m_target[1].str().c_str();
					}

					// output to the system console
					the_sys_console << buff.c_str() << crlf;
				}
			}
		} else { // success call
			// display error logs
			try {
				std::ifstream ifs(STR_BUILD_ERROR_LOG);
				std::string buff;
				
				auto _re_gcc_error = std::regex(R"(([a-zA-Z0-9_\-]+)\.([cC]|[cC][pP][pP]|[hH]|[hH][pP][pP])\:([0-9]+)\:([0-9]+)\:[ \t](fatal error|error):[ \t](.+))");
				auto _re_gcc_error0 = std::regex(R"(error\:)");
				auto _re_gcc_warning0 = std::regex(R"(warning\:)");
				auto _re_gcc_ld_undef_ref = std::regex(R"((\: undefined reference to )(.+))");
				auto _re_gcc_ld_multi_def = std::regex(R"((\: multiple definition of )(.+))");

				while (getline(ifs, buff)) {
					bool b_match_err_message = false;
					bool b_match_warning_message = false;

					// chop it.
					remove_endl(buff);
					
					// match object
					std::smatch m_err;

					// parse lines
					if (std::regex_search(buff, _re_gcc_error0)) {
						b_match_err_message = true;

						if (std::regex_search(buff, m_err, _re_gcc_error)) {
							SmplBuf_ByteSL<64> msgerr; // limit to 64chars
							msgerr << m_err[6].str().c_str();

							// compile error
							the_screen
								<< crlf << ">> "
								<< TermAttr(TERM_COLOR_BG_BLACK | TERM_COLOR_FG_RED)
								<< m_err[1].str().c_str() << '.' << m_err[2].str().c_str()
								<< TermAttr(TERM_ATTR_OFF)
								<< ':' << m_err[3].str().c_str()
								<< " " << msgerr;

							the_screen_b
								<< crlf << ">> "
								<< TermAttr(TERM_COLOR_BG_RED | TERM_COLOR_FG_BLACK)
								<< "ERROR:"
								<< TermAttr(TERM_COLOR_BG_BLACK | TERM_COLOR_FG_RED)
								<< m_err[1].str().c_str() << '.' << m_err[2].str().c_str()
								<< TermAttr(TERM_ATTR_OFF)
								<< ':' << m_err[3].str().c_str()
								<< " " << msgerr;
						}
					} else
					if (std::regex_search(buff, m_err, _re_gcc_ld_undef_ref)) {
						the_screen
							<< crlf << ">> "
							<< TermAttr(TERM_COLOR_BG_BLACK | TERM_COLOR_FG_YELLOW)
							<< "ﾘﾝｶ:未定義:"
							<< TermAttr(TERM_ATTR_OFF)
							<< m_err[2].str().c_str();
						b_match_err_message = true;
					} else
					if (std::regex_search(buff, m_err, _re_gcc_ld_multi_def)) {
						the_screen
							<< crlf << ">> "
							<< TermAttr(TERM_COLOR_BG_BLACK | TERM_COLOR_FG_YELLOW)
							<< "ﾘﾝｶ:複数定義:"
							<< TermAttr(TERM_ATTR_OFF)
							<< m_err[2].str().c_str();
						b_match_err_message = true;
					} else
					if (std::regex_search(buff, _re_gcc_warning0)) {
						b_match_warning_message = true;
					}

					if (b_match_err_message) the_sys_console << "\033[31;47m"; // RED/WHITE
					if (b_match_warning_message) the_sys_console << "\033[33;40m"; // YELLOW/BLACK
					the_sys_console << buff.c_str();
					if (b_match_err_message || b_match_warning_message) the_sys_console << "\033[0m";
					the_sys_console << crlf;
				}
			}
			catch (...) { ; }
			
			// check exit code
			if (_pipe.exit_code() == 0) {
				_parent->_firmfile_dir.resize(0);
				_parent->_firmfile_dir << _act_dir;
				
				_parent->_firmfile_name = as_copying(_act_build_file);
				_parent->_firmfile_disp = as_copying(_act_build_file);
				exit(EXIT_NEXT); // switch to the firm write.
			}
			else {
				the_screen << crlf << "\033[7m" << L"ビルド中にエラーを検出しました。" << "\033[0m";
				_timer_exit.start(60000);
			}
		}
		break;

	case EV_EXIT:
		the_cwd.change_dir(the_cwd.get_dir_exe());
		_timer_exit.stop();
		_pipe.close();
		break;
	}
}

void App_FirmProg::Screen_ActBuild::hndlr_error(event_type ev, arg_type arg) {
	switch (ev) {
	case EV_SETUP:
		_timer_exit.start(3000);
		break;

	case EV_LOOP:
		if (_timer_exit.is_timeout()) {
			exit(EXIT_BACK_TO_MENU);
		}
		if (the_keyboard.available()) {
			int c = the_keyboard.read();
			switch (c) {
			case KeyInput::KEY_ESC:
			case KeyInput::KEY_BUTTON_B:
			case KeyInput::KEY_BUTTON_A_LONG:
				exit(EXIT_BACK_TO_MENU);
				return;
			}
		}
		//the_screen << "\033[G" << printfmt("--%03d--", (_timer_exit.get_timeout() - _timer_exit.ms_to_fire()) / 10);
		break;

	case EV_EXIT:
		break;
	}
}

#endif