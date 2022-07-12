/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include <algorithm>
#include <iterator>

#include "App_SelectPort.hpp"
#include "serial_ftdi.hpp"

template<>
const wchar_t* App_SelectPort::APP_DESC<App_SelectPort>::TITLE_LONG[] = {
	L"シリアルポートの選択",
	L"Select SERIAL port"
};

template<>
const wchar_t* App_SelectPort::APP_DESC<App_SelectPort>::LAUNCH_MSG[] =
//....+....1....+....2....+....3....+....4| // 16dots 40cols
{
L"シリアルポートを選択します。""\r\n"
L"※ Alt(Cmd)+0..9での操作も可能です。""\r\n"
,
L"Select SERIAL PORT""\r\n"
L"※ It can be seleceted by Alt(Cmd)+0..9 operation as well.""\r\n"
};

void App_SelectPort::setup() {
	// preference
	the_settings_menu.begin(0); // load default preference
	
	// init the TWE M5 support
	setup_screen(); // initialize TWE M5 support.

	// put a init message
	the_screen_t << MLSLW(L"\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m ｼﾘｱﾙﾎﾟｰﾄ選択",
						  L"\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m Select SERIAL port");

#if defined(_MSC_VER) || defined(__MINGW32__)
	TermAttr TB(TERM_COLOR_FG_BLACK | TERM_COLOR_BG_WHITE);
	TermAttr TC(TERM_ATTR_OFF);
	the_screen_b << TB << MLSLW(L" c ｷｰ: 反転項目のCOM番号を調べる", L" c key: look up COM port # of selected item.") << TC;
#endif

	// add items
	Serial2.list_devices();
	if (Serial2.ser_count == 0) {
		APP_HNDLR::new_hndlr(&App_SelectPort::hndlr_not_found);
	} else {
		APP_HNDLR::new_hndlr(&App_SelectPort::hndlr_list);
	}
}

void App_SelectPort::loop() {
	APP_HNDLR::loop();
	screen_refresh();
}

void App_SelectPort::hndlr_not_found(event_type ev, arg_type arg) {
	switch (ev) {
	case EV_SETUP:
		// button navigation
		the_screen_c.clear_screen();
		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << MLSLW(L"     --/長押:--            ﾒﾆｭｰ/--                --/--",
							  L"     --/Long:--            MENU/--                --/--");

		switch (g_lang) {
		case TWE::LANG_JP:
			the_screen << "\033[2J\033[H"
				<< crlf
				// L"....+....1....+....2....+....3....+....4" // 16dots 40cols
				<< L"   * シリアルポートが見つかりません *   " << crlf << crlf
				<< L"    \033[7mボタン\033[0mまたは\033[7m[Enter]\033[0mで、" << crlf
				<< L"    メニューに移動します。";
			break;
		case TWE::LANG_EN:
			the_screen << "\033[2J\033[H"
				<< crlf
				// L"....+....1....+....2....+....3....+....4" // 16dots 40cols
				<< L"   * No SERIAL port found *   " << crlf << crlf
				<< L"   Move bake to MENU by" << crlf
				<< L"    \033[7mButton\033[0mor\033[7m[Enter]\033[0m."
				;
			break;
		}
		break;

	case EV_LOOP:
		while (the_keyboard.available()) {
			int c = the_keyboard.read();
			switch (c) {
			case KeyInput::KEY_ENTER:
			case KeyInput::KEY_ESC:
			case KeyInput::KEY_BUTTON_A:
			case KeyInput::KEY_BUTTON_B:
			case KeyInput::KEY_BUTTON_C:
				the_app.exit(0);
				break;
			default:
				if (TWECUI::KeyInput::MOUSE_UP::is_type(c)) {
					// press LEFT mouse button to proceed.
					TWECUI::KeyInput::MOUSE_UP ev(c);
					if (auto&& coord = the_screen.get_term_coord_from_screen(ev.get_x(), ev.get_y())) {
						the_app.exit(0);
					}
				}
				break;
			}
		}
		break;

	case EV_EXIT:
		break;
	}
}

void App_SelectPort::hndlr_list(event_type ev, arg_type arg) {
	bool b_selected = false;

	switch (ev) {
	case EV_SETUP:
		// button navigation
		the_screen_c.clear_screen();
		//e_screen_c <<        "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << MLSLW(L"     ↑/長押:--            決定/--                ↓/--",
							  L"     ↑/Long:--          SELECT/--                ↓/--");

		if (sAppData.au8_TWESTG_STAGE_FTDI_ADDR[1] <= 0x20) {
			int c = sAppData.au8_TWESTG_STAGE_FTDI_ADDR[0];
			int n = c - '0';

			// if single char is specified, select the port of the listed number.
			if (n >= 1 && n <= Serial2.ser_count) {
				b_selected = true;

				if (_n_arg == APP_ID) { // only if _n_arg is provided as APP_ID
					the_keyboard.push(c);
				}
			}
		}

		for (int i = 0; i < Serial2.ser_count; i++) {
			int n = _listPorts.push_back(""); // insert a new entry.

			// get entry object
			auto item_p = _listPorts.get(n);
			auto& item = item_p.first;
			auto& item_second = item_p.second;

			// set display name
			item.clear();
			item << Serial2.ser_desc[i];

			item << ' ';
			item << '(';
			item << Serial2.ser_devname[i];
			item << ')';
			
			// set device serial as second data.
			item_second.clear();
			item_second << Serial2.ser_devname[i];


			// if ser# of FTDI device is specified over settings, check with it.
			if (sAppData.au8_TWESTG_STAGE_FTDI_ADDR[0] != 0
				&& !strncmp(Serial2.ser_devname[i], (const char*)sAppData.au8_TWESTG_STAGE_FTDI_ADDR, 8)) {
				b_selected = true;

				if (_n_arg == APP_ID) { // only if _n_arg is provided as APP_ID
					the_keyboard.push('0' + i + 1);
				}
			}
		}

		_listPorts.attach_term(the_screen);
		_listPorts.set_view();
		_listPorts.update_view(true);

		break;

	case EV_LOOP:
		do {
			int c = the_keyboard.read(); //note: even c==-1, it must be passed to list object for timeout check.

#if defined(_MSC_VER) || defined(__MINGW32__)
			if (c == 'c') {
				// check com port number
				int i_sel = _listPorts.get_selected_index();

				if (i_sel >= 0) {
					TermAttr TB(TERM_COLOR_FG_BLACK | TERM_COLOR_BG_YELLOW);
					TermAttr TC(TERM_ATTR_OFF);

					// find ser# string from head to space. "XXXXX (DESC)"
					SmplBuf_ByteSL<32> devname; // create char_t buffer.
					devname << _listPorts[i_sel].second;

					if (Serial2.is_opened()) Serial2.close();
					Serial2.open((const char*)devname.c_str()); // open the device

					// get extra information (COM number, the port must be opened)
					const wchar_t* msg = Serial2.query_extra_device_info();
					if (msg == nullptr || msg[0] == 0) {
						msg = MLSLW(L"不明", L"UNK");
					}

					the_screen_b.clear_line(1);
					the_screen_b(1, 1)
						<< TermAttr(TERM_COLOR_FG_BLACK | TERM_COLOR_BG_YELLOW)
						<< _listPorts[i_sel].first << L" -> " << msg << L"."
						<< TermAttr(TERM_ATTR_OFF);

					if (Serial2.is_opened()) Serial2.close();
				}
			} else
#endif
			if (_listPorts.size() > 0 && _listPorts.key_event(c)) {
				// pass key input to list widget and check select completion.
				if (_listPorts.is_selection_completed()) {
					int i_sel = _listPorts.get_selected_index();

					// find ser# string from head to space. "XXXXX (DESC)"
					SmplBuf_ByteSL<32> devname;
					devname << _listPorts[i_sel].second;

					if (Serial2.is_opened()) Serial2.close();
					Serial2.open((const char*)devname.c_str()); // open the device

					// start exit timer
					the_app.exit(APP_ID);

					return; // wait for exit timer expiration.
				}
			}
			else
			switch (c) {
			case KeyInput::KEY_ESC:
				the_app.exit(0);
				return;

			case KeyInput::KEY_BUTTON_A:
				the_keyboard.push(TWECUI::KeyInput::KEY_UP);
				break;

			case KeyInput::KEY_BUTTON_A_LONG:
				the_app.exit(0);
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
				break;
			}
		} while (the_keyboard.available());
		break;
	
	case EV_EXIT:
		break;
	}
}

void App_SelectPort::setup_screen() {
	// LCD font color
	default_bg_color = BLACK; // color565(90, 0, 50); 
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
	TWEFONT::createFontMP12(13, 0, 0);

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
void App_SelectPort::screen_refresh(bool bforce) {
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