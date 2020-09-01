/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include <algorithm>
#include <iterator>

#include "App_SelectPort.hpp"
#include "serial_ftdi.hpp"

void App_SelectPort::setup() {
	// preference
	the_settings_menu.begin(0); // load default preference
	
	// init the TWE M5 support
	setup_screen(); // initialize TWE M5 support.

	// put a init message
	the_screen_t << "\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m ｼﾘｱﾙﾎﾟｰﾄ選択";

	// add items
	SerialFtdi::list_devices();
	if (SerialFtdi::ser_count == 0) {
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
		the_screen_c << "     --/長押:--            ﾒﾆｭｰ/--                --/--";

		the_screen << "\033[2J\033[H"
			<< crlf
			// L"....+....1....+....2....+....3....+....4" // 16dots 40cols
			<< L"   * シリアルポートが見つかりません *   " << crlf << crlf
			<< L"    \033[7mボタン\033[0mまたは\033[7m[Enter]\033[0mで、" << crlf
			<< L"    メニューに移動します。";
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
		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << "     ↑/長押:--            決定/--                ↓/--";

		if (sAppData.au8_TWESTG_STAGE_FTDI_ADDR[1] <= 0x20) {
			int c = sAppData.au8_TWESTG_STAGE_FTDI_ADDR[0];
			int n = c - '0';

			// if single char is specified, select the port of the listed number.
			if (n >= 1 && n <= SerialFtdi::ser_count) {
				b_selected = true;

				if (_n_arg == APP_ID) { // only if _n_arg is provided as APP_ID
					the_keyboard.push(c);
				}
			}
		}

		for (int i = 0; i < SerialFtdi::ser_count; i++) {
			int n = _listPorts.push_back(SerialFtdi::ser_devname[i]);
			auto item_p = _listPorts.get(n);
			auto& item = item_p.first;

			item << L" (";
			item << SerialFtdi::ser_desc[i];
			item << L")";

			// if ser# of FTDI device is specicied, check with it.
			if (!strncmp(SerialFtdi::ser_devname[i]
				, (const char*)sAppData.au8_TWESTG_STAGE_FTDI_ADDR
				, 8)) {
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
			int c = the_keyboard.read(); //note: even c==-1, it must be passed to list object to timeout check.

			if (_listPorts.size() > 0 && _listPorts.key_event(c)) {
				if (_listPorts.is_selection_completed()) {
					int i_sel = _listPorts.get_selected_index();

					// find ser# string from head to space. "XXXXX (DESC)"
					SmplBuf_ByteL<32> devname;
					auto& wstr = _listPorts[i_sel].first;
					auto pos_space = std::find(wstr.begin(), wstr.end(), L' '); // find the first blank space.
					if (pos_space != wstr.end()) {
						// convert wchar_t to char
						auto p = wstr.begin();
						auto e = pos_space;
						while (p != e) { devname.push_back(uint8_t(*p)); ++p; }
						
						if (Serial2.is_opened()) Serial2.close();
						Serial2.open((const char*)devname.c_str()); // open the device

						// start exit timer
						the_app.exit(APP_ID);

						return; // wait for exit timer expiration.
					}
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

	// font register (note: to save flash area, don't create too much!)
	TWEFONT::createFontMP10_std(1, 0, 0); // MP10 font

	TWEFONT::createFontShinonome16_mini(10, 0, 0); // shinonome 16 font
	TWEFONT::createFontShinonome16_mini(11, 1, 0); // shinonome 16 font
	TWEFONT::createFontMP12_mini(12, 0, 0, TWEFONT::U32_OPT_FONT_TATEBAI | TWEFONT::U32_OPT_FONT_YOKOBAI); // MP10 font
	TWEFONT::createFontMP12_mini(13, 0, 0); // MP10 font

	// main screen area
	the_screen.set_font(10);
	the_screen.set_color(default_fg_color, default_bg_color);
	the_screen.set_cursor(0); // 0: no 1: curosr 2: blink cursor
	the_screen.force_refresh();

	// bottom area
	the_screen_b.set_font(1);
	the_screen_b.set_color(color565(80, 80, 80), color565(20, 20, 20));
	the_screen_b.set_cursor(0);
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