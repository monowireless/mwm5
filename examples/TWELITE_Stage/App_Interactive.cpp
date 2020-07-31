/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_Interactive.hpp"

 // *** Entering Config Mode ***
 // !INF EXIT INTERACTIVE MODE.

void App_Interactive::setup() {
	// preference
	the_settings_menu.begin(appid_to_slotid(APP_ID));

#ifdef ESP32
	if (!sAppData.u8_TWESTG_STAGE_INTRCT_USE_SETPIN) {
		_g_twemodctl_esp32_no_set_control = 1;
	}
#endif

	// init the TWE M5 support
	setup_screen(); // initialize TWE M5 support.

	// put a init message
	the_screen_t << "\033[G\033[1mTWELITE\033[0m®\033[1mSTAGE\033[0m ｲﾝﾀﾗｸﾃｨﾌﾞﾓｰﾄﾞ\033[0m";

	// set new handler
	APP_HNDLR::new_hndlr(&App_Interactive::hndlr_init_screen);
}

void App_Interactive::loop() { 
	// run loop of the handler
	APP_HNDLR::loop();


	// LCD update
	screen_refresh();
}

void App_Interactive::hndlr_init_screen(event_type ev, arg_type arg) {
	switch (ev) {
	case EV_SETUP:
		the_screen.clear();
		the_screen_msg.visible(true);
		the_screen_msg << L"TWELITEﾓｼﾞｭｰﾙを\033[4mｲﾝﾀﾗｸﾃｨﾌﾞﾓｰﾄﾞ\033[0mにします。" << crlf << crlf;
		the_screen_msg
			//  "....+....1....+....2....+....3....+....4"
			<< L"※TWELITEは一旦ﾘｾｯﾄされ、ｲﾝﾀﾗｸﾃｨﾌﾞﾓｰﾄﾞに" L"\r\n"
			<< L" 移行します。ｲﾝﾀﾗｸﾃｨﾌﾞﾓｰﾄﾞ未対応の場合は" L"\r\n"
			<< L" 通常の出力が表示されます。"              L"\r\n"
			<< L"(画面が崩れた場合は[\033[7mEnter\033[0m]ｷｰで再描画)" L"\r\n"
			;


#ifdef ESP32
		// in case GPIO cannot be used (faces keypad is connected)
		if (_g_twemodctl_esp32_no_set_control) {			
			the_screen_msg
				<< printfmt("\033[%d;1H", the_screen_msg.get_rows() - 2)
				<< "\033[31;40m\033[K"
				//  "....+....1....+....2....+....3....+....4"
				<< L"  SETﾋﾟﾝ制御が必要なｱﾌﾟﾘ(例:PAL子機)は"
				<< crlf
				<< "\033[31;40m\033[K"
				//  "        DI1ボタンを押しながら"
				<< L"        \033[1;31;47mDI1ボタン\033[0m\033[31;40mを押しながら\033[0m"
				;
		}
#endif

		the_screen_msg << crlf
			<< printfmt("\033[%d;1H", the_screen_msg.get_rows())
			//  "....+....1....+....2....+....3....+....4"
			//         中ボタンまたは[Enter]で進む
			<< L"      \033[7m中ボタン\033[0mまたは[\033[7mEnter\033[0m]で進む";

		the_screen_msg.force_refresh();

		// button navigation
		//              "0....+....1....+....2....+....3....+....4....+....5....+...."
		the_screen_c.clear_screen();
		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << "     --/長押:MENU     ｲﾝﾀﾗｸﾃｨﾌﾞﾓｰﾄﾞ遷移/--        --/-- ";
		the_screen_c.force_refresh();

		break;

	case EV_LOOP:
		// the keyboard
		while (the_keyboard.available()) {
			int c = the_keyboard.read();

			switch (c) {
			case KeyInput::KEY_BS:
			case KeyInput::KEY_ESC:
			case KeyInput::KEY_BUTTON_A_LONG:
				the_app.exit(APP_ID);
				break;
			case KeyInput::KEY_ENTER:
			case KeyInput::KEY_BUTTON_A:
			case KeyInput::KEY_BUTTON_B:
			case KeyInput::KEY_BUTTON_B_LONG:
			case KeyInput::KEY_BUTTON_C:
			case KeyInput::KEY_BUTTON_C_LONG:
				APP_HNDLR::new_hndlr(&App_Interactive::hndlr_main_screen);
				break;

			default:

				break;
			}
		}
		break;

	case EV_EXIT:
		the_screen_msg.visible(false);
		break;
	}
}

void App_Interactive::hndlr_main_screen(event_type ev, arg_type arg) {
	int ct = 0; // uart input count

	switch (ev) {
	case EV_SETUP:
		the_screen.set_cursor(2);
		the_screen.visible(true);
		the_screen.clear_screen();
		the_screen.force_refresh();

		// button navigation
		the_screen_c.clear_screen();
		//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
		the_screen_c << "        --/長押:MENU     ズーム/--          ﾌｧｰﾑ書換/-- ";
		the_screen_c.force_refresh();

		// reserve struct
		_sp_intr.reset(new INTR);
		break;

	case EV_LOOP:
		// check uart input
		while (the_uart_queue.available()) {
			INTR& intr = *_sp_intr;

			int c = the_uart_queue.read();
			monitor_uart(c);

			// pass them to M5 (normal packet analysis)
			if (c >= 0) {
				the_screen << char_t(c);
				ct++;
			}
		}
		if (ct == 0) {
			monitor_uart(KeyInput::KEY_VOID); // timeout check
		}

		// the keyboard
		while (the_keyboard.available()) {
			int c = the_keyboard.read();

			c = virt_scrctl(c);

			switch (c) {
			case KeyInput::KEY_BUTTON_A_LONG:
			case KeyInput::KEY_ESC:
				if (!(_sp_intr && _sp_intr->b_now_input)) {
					// if input prompt is confirmed, just pass ESC to the UART,
					// otherwise exit this screen.
					the_app.exit(APP_ID);
					c = -1;
				}
				break;

			case KeyInput::KEY_BUTTON_B:
				// zoom
				change_screen_font();
				break;

			case KeyInput::KEY_BUTTON_C:
				// move to firm prom menu
				the_app.exit(EXIT_ID_GOTO_FIRM_PROG_LAST_BUILD, (int)E_APP_ID::FIRM_PROG);
				break;

			case KeyInput::KEY_BUTTON_A:
			case KeyInput::KEY_BUTTON_B_LONG:
			case KeyInput::KEY_BUTTON_C_LONG:
				break;

			case KeyInput::KEY_UP: WrtTWE << "\033[A"; c = -1; break;
			case KeyInput::KEY_DOWN: WrtTWE << "\033[B"; c = -1; break;
			case KeyInput::KEY_LEFT: WrtTWE << "\033[D"; c = -1; break;
			case KeyInput::KEY_RIGHT: WrtTWE << "\033[C"; c = -1; break;

			default:
				break;
			}

			if (c >= 0 && c <= 0x7F) {
				WrtTWE << char_t(c);
			}
		}

		if (ct) {
			// if changed in screen, set the virt screen with cursor visible.
			virt_scrctl(0);
		}

		break;

	case EV_EXIT: // on exit reset module
		twe_prog.setpin(false);
		twe_prog.reset_module();
		break;
	}
}

// color table
static const uint16_t COLTBL_MAIN[8] = {
	BLACK,
	RED,
	GREEN,
	YELLOW,
	color565(127, 127, 255), // BLUE,
	color565(255, 0, 142), // MAGENTA,
	CYAN,
	ALMOST_WHITE
};

void App_Interactive::setup_screen() {
	// LCD font color
	default_bg_color = color565(sAppData.u32_TWESTG_STAGE_BG_COLOR); // color565(90, 0, 50); 
	default_fg_color = color565(sAppData.u32_TWESTG_STAGE_FG_COLOR);

	// font register (note: to save flash area, don't create too much!)
	TWEFONT::createFontMP10(1, 0, 0); // MP10 font
	TWEFONT::createFontMP10(10, 0, 0); // MP10 font
	TWEFONT::createFontShinonome16(11, 0, 0); // shinonome 16 font
	TWEFONT::createFontShinonome16(12, 1, 0); // shinonome 16 font

	// main screen area
	the_screen.set_color(default_fg_color, default_bg_color);
	the_screen.set_color_table(COLTBL_MAIN);
	the_screen.set_cursor(2); // 0: no 1: curosr 2: blink cursor
	the_screen.set_wraptext(false); // no wrap text mode
	bZoom = true; change_screen_font(); // bZoom is toggled from true to false.
	the_screen.visible(false);

	// bottom area
	the_screen_c.set_font(1);
	the_screen_c.set_color(ORANGE, color565(20, 20, 20));
	the_screen_c.set_cursor(0);
	the_screen_c.force_refresh();

	// top area
	the_screen_t.set_font(12);
	the_screen_t.set_color(default_bg_color, default_fg_color);
	the_screen_t.set_cursor(0);
	the_screen_t.force_refresh();

	// msg area
	the_screen_msg.set_font(12);
	the_screen_msg.set_color(default_fg_color, default_bg_color);
	the_screen_msg.set_cursor(0);
	the_screen_msg.visible(false); // don't show at the initial
}

// screen refresh timing (every 32ms)
void App_Interactive::screen_refresh() {
	static uint32_t u32mills;

	uint32_t u32now = millis();
	if (u32now - u32mills > 32) {
		//if (bZoom) virt_scrctl(0); // in zoom mode, follow cursor position

		if (_ct_refresh == 0) {
			the_screen.refresh();
			the_screen_msg.refresh();
			the_screen_c.refresh();
			the_screen_t.refresh();
		}
		else {
			_ct_refresh--;

			if (!_ct_refresh) the_screen.force_refresh();
		}
		
		u32mills = u32now;
	}
}

// change screen font
void App_Interactive::change_screen_font() {
	// toggle the state
	bZoom = !bZoom;

	signed rows = the_screen.get_rows();
	signed cols = the_screen.get_cols();

	std::unique_ptr<GChar[]> scrbuf(new GChar[scr_max_cols * scr_max_cols]);

	the_screen.get_screen_buf(scrbuf.get());

	if (bZoom) {
		the_screen.set_draw_area({scr_x, scr_y, scr_wz, scr_hz});
		the_screen.set_view_pos(0, 0);
		the_screen.set_view_size(scr_w, scr_h);
		the_screen.set_font(11, scr_max_cols, scr_max_rows);
	}
	else {
		the_screen.set_draw_area({ scr_x, scr_y, scr_w, scr_h });
		the_screen.set_view_pos(0, 0);
		the_screen.set_view_size(scr_w, scr_h);
		the_screen.set_font(10, scr_max_cols, scr_max_rows);
	}

	the_screen.set_screen_buf(scrbuf.get(), cols, rows);
	virt_scrctl(0, true); // set visible area including cursor position (and refresh screen)
}

// change screen color
void App_Interactive::change_screen_color() {
	static bool i = false;
	i = !i;

	if (i) {
		the_screen.set_color(ORANGE, color565(0x20, 0x20, 0x20));
	}
	else {
		the_screen.set_color(default_fg_color, default_bg_color);
	}

	the_screen.force_refresh();
}

// check serial input.
void App_Interactive::monitor_uart(KeyInput::keyinput_type c) {
	if (_sp_intr) {
		INTR& intr = *_sp_intr;
		bool bBreak = false;

		// check line break
		if (c == 0x0d || c == 0x0a) {
			intr.que_uart.push(uint8_t(c));
			bBreak = true;
		} else
		if (c == KeyInput::KEY_VOID) {
			if (intr.timeout_uart.available()) {
				intr.timeout_uart.start(100);
				bBreak = true;
			}
		} else
		if (c >= 0x00 && c <= 0xFF) {
			// timer set
			intr.timeout_uart.start(100);
			
			intr.que_uart.push(c & 0xFF);
		}
		
		if (bBreak || (intr.stat == E_STAT::NONE && c == KeyInput::KEY_VOID)) {
			bool b_next = false;

			SmplBuf_ByteL<INTR::UART_LINE_CHAR_MAX + 1> l_buff; // string type, with buffer space for NUL char.

			while (intr.que_uart.available()) {
				int c = intr.que_uart.read();
				if ((c == 0x0d || c == 0x0a) && intr.b_now_input) {
					intr.b_now_input = 0;
				}
				if (c >= 0x20 && c <= 0x7F) { // put only readable chars
					l_buff.push_back(toupper(c & 0xFF));
				}
			}

			// check state
			switch (intr.stat) {
			case E_STAT::NONE:
				intr.timeout.start(1000);
				the_screen.set_color(color565(80, 80, 80), color565(20, 20, 20));
				the_screen.clear_screen();
				the_screen.force_refresh();

				// interactive mode
				twe_prog.setpin(true); // SET=LOW (for App_Tag/PAL end device)
				// set new state
				intr.stat = E_STAT::RESET_WITH_SET; // next
				intr.re.comp("ENTERING.CONFIG.MODE"); // expected pattern (for App_Tag/PAL end device)
				//intr.re.comp("ENTERING.CONFIG.MODE"); // expected pattern (for App_Tag/PAL end device)
				twe_prog.reset_module(); // module reset				
				break;

			case E_STAT::RESET_WITH_SET:
				// starting state
				if (intr.timeout.is_timeout()) {
					// not AppTag/PAL end device, so try to press + + +
					WrtTWE << '+';
					delay(400);
					WrtTWE << '+';
					delay(400);
					WrtTWE << '+';
					b_next = true;
				}

				// match pattern
				if (intr.re.exec((const char*)l_buff.get().c_str())) {
					b_next = true;
				}

				// exit condition (detect or not)
				if (b_next) {
					intr.timeout.start(100);
					intr.re.comp("!INF.[ER][XE][IS][TE]");
					intr.re2.comp("INPUT.*:");

					intr.stat = E_STAT::DETECT_ENTERING; // next (Entering Interactive mode)

					the_screen.set_color(color565(sAppData.u32_TWESTG_STAGE_FG_COLOR), color565(sAppData.u32_TWESTG_STAGE_BG_COLOR));
					the_screen.force_refresh();
				}
				break;

			case E_STAT::DETECT_ENTERING:
				if (intr.timeout.is_timeout()) {
					twe_prog.setpin(false);
				}
				if (intr.re.exec((const char*)l_buff.get().c_str())) {
					intr.timeout.start(1000);
					intr.stat = E_STAT::DETECT_EXIT_MESSAGE;
				}
				if (l_buff.size() > 0) {
					if (intr.re2.exec((const char*)l_buff.get().c_str())) {
						intr.b_now_input = 1;
					}
				}
				break;
				
			case E_STAT::DETECT_EXIT_MESSAGE:
				if (intr.timeout.is_timeout()) {
					the_app.exit(APP_ID);
				}
				break;

			default:
				the_app.exit(APP_ID);
			}
		}

	}
}

int App_Interactive::virt_scrctl(int c, bool reset) {
	if (bZoom) {
		static uint8_t ix;
		static uint8_t iy;
		bool bForceRedraw = false;
		uint8_t ix2 = ix, iy2 = iy;

		if (reset) {
			ix = ix2 = 0;
			iy = iy2 = 0;
			bForceRedraw = true;
		}

		// view change by keyboard operation
		switch (c) {
		case KeyInput::KEY_LEFT: ix2 = (ix - 1) & 1; c = -1;  break;
		case KeyInput::KEY_RIGHT: ix2 = (ix + 1) & 1; c = -1; break;
		case KeyInput::KEY_UP: iy2 = (iy - 1) & 1; c = -1;  break;
		case KeyInput::KEY_DOWN: iy2 = (iy + 1) & 1; c = -1; break;
		}
		
		// follow to the cursor position
		if (c != -1) {
			// set to cursor pos
			int16_t c_x, c_y;

			// current viewing screen area
			int16_t y1 = iy ? (scr_hz - scr_h) : 0;
			int16_t y2 = y1 + scr_h - 1;
			int16_t x1 = ix ? (scr_wz - scr_w) : 0;
			int16_t x2 = x1 + scr_w - 1;

			// get cursor position as relative coordinate.
			the_screen.get_cursor_pos_relative(c_x, c_y);

			// check if cursor is in the current viewing area.
			if (c_x < x1 || c_x > x2 - 16) ix2 = (ix + 1) & 1;
			if (c_y < y1 || c_y > y2) iy2 = (iy + 1) & 1;
		}

		// do redraw
		if (bForceRedraw || ix != ix2 || iy != iy2) {
			ix = ix2;
			iy = iy2;

			the_screen.set_view_pos(ix ? (scr_wz - scr_w) : 0, iy ? (scr_hz - scr_h) : 0);
			_ct_refresh = 2;
		}
	}
	else {
		if (reset)
			_ct_refresh = 2;
	}

	return c;
}