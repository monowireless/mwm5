/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_Commander.hpp"
using APP_BASE = App_Commander;

struct  APP_BASE::SCR_NOTICE1 : public APP_HANDLR_DC {
	static const int CLS_ID = APP_BASE::PAGE_ID::PAGE_NOTICE01;
	int get_class_id() { return CLS_ID; }

	APP_BASE& _app;
	TWE_WidSet_Buttons _btns;

	// constructor
	SCR_NOTICE1(APP_BASE& app) : _app(app), _btns(*this, app.the_screen)
		, _b_dirty(1), _id(1), _pat(0), _col(0), _bri(15), _dur(4), APP_HANDLR_DC(CLS_ID) {}

private:
	int8_t _incr(int8_t& x, int8_t x_min, int8_t x_max) {
		_b_dirty = 1;

		if (x >= x_max) x = x_min;
		else x++;

		return x;
	}
	int8_t _decr(int8_t& x, int8_t x_min, int8_t x_max) {
		_b_dirty = 1;

		if (x <= x_min) x = x_max;
		else x--;

		return x;
	}

	int8_t _b_dirty;
	int8_t _id;
	int8_t _col;
	int8_t _pat;
	int8_t _bri;
	int8_t _dur;

public:
	void on_setup() {
		_b_dirty = 1;
	}

	// id
	void id_up(int id, uint32_t opt = 0) { _incr(_id, 1, 8); generate_command(); }
	void id_dn(int id, uint32_t opt = 0) { _decr(_id, 1, 8); generate_command(); }

	// col
	void col_up(int id, uint32_t opt = 0) { _incr(_col, 0, 7); generate_command(); }
	void col_dn(int id, uint32_t opt = 0) { _decr(_col, 0, 7); generate_command(); }

	// ptn
	void pat_up(int id, uint32_t opt = 0) { _incr(_pat, 0, 3); generate_command(); }
	void pat_dn(int id, uint32_t opt = 0) { _decr(_pat, 0, 3); generate_command(); }

	// ptn
	void bri_up(int id, uint32_t opt = 0) { _incr(_bri, 0, 15); generate_command(); }
	void bri_dn(int id, uint32_t opt = 0) { _decr(_bri, 0, 15); generate_command(); }

	// dur
	void dur_up(int id, uint32_t opt = 0) { _incr(_dur, 0, 7); generate_command(); }
	void dur_dn(int id, uint32_t opt = 0) { _decr(_dur, 0, 7); generate_command(); }
	int get_dur_s() {
		const int DUR[] = { 10, 20, 30, 45, 60, 90, 120, 300 };
		return DUR[_dur];
	}

	// buttons
	void turn_on(int id, uint32_t opt = 0) { generate_command(); }
	void turn_off(int id, uint32_t opt = 0) { generate_command(false); }

#ifndef ESP32
	void web(int id, uint32_t opt = 0) {
		shell_open_url(L"https://wings.twelite.info/how-to-use/parent-mode/transemit-command/0x90-command");
	}
#endif

	// screen updator
	void update_screen();

	// parser command and transmit
	void generate_command(bool turn_on = true);

	// the setup
	void setup() {
		auto& dc = *this;
		
		// layout screen.
		_app.screen_layout_apps();

		// set title bar
		_app.set_title_bar(PAGE_ID::PAGE_NOTICE01);

		// set nav bar
		{
			auto& t = _app.the_screen_c; t.clear_screen();
			//    "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
			t << MLSLW(L"    --/長押:戻る             --/--                --/ﾘｾｯﾄ",
				       L"    --/Long:BACK             --/--                --/RST");
		}

		// initialize some
		_b_dirty = 1;

		// control array
		{
			auto& t = _app.the_screen;

			// put base screen text
			t.clear_screen();

			if (g_lang == TWE::LANG_JP) {
				//   "0....+....1....+....2....+....3....+....4....+....5...
				t << crlf;
				t << "             [NOTICE PAL LED 点灯制御]" << crlf;     // L=1
				t << crlf;
				t << "    子機ID       (I)▽        △(i)" << crlf; // L=3
				t << crlf;
				t << "    色           (C)▽        △(c)" << crlf; // L=5
				t << crlf;
				t << "    明るさ       (B)▽        △(b)" << crlf; // L=7
				t << crlf;
				t << "    点灯点滅     (P)▽        △(p)" << crlf; // L=9
				t << crlf;
				t << "    点灯時間     (D)▽        △(d)" << crlf; // L=11
				t << crlf;
				t << "                    消灯(x)   点灯(SPACE)"; // L=13 (消灯,点灯)
			}
			else {
				//   "0....+....1....+....2....+....3....+....4....+....5...
				t << crlf;
				t << "             [NOTICE PAL LED ON/OFF CONTROL]" << crlf;     // L=1
				t << crlf;
				t << "    LD(Chld)     (I)▽        △(i)" << crlf; // L=3
				t << crlf;
				t << "    Color        (C)▽        △(c)" << crlf; // L=5
				t << crlf;
				t << "    Brightness   (B)▽        △(b)" << crlf; // L=7
				t << crlf;
				t << "    Blink        (P)▽        △(p)" << crlf; // L=9
				t << crlf;
				t << "    Light Dur    (D)▽        △(d)" << crlf; // L=11
				t << crlf;
				t << "                    消灯(x)   点灯(SPACE)"; // L=13 (消灯,点灯)
			}

			// add buttons
			dc._btns.clear();

			dc._btns.add(20 - 3, 3, L"(I)▽", &SCR_NOTICE1::id_dn, &dc);
			dc._btns.add(30, 3, L"△(i)", &SCR_NOTICE1::id_up, &dc);

			dc._btns.add(20 - 3, 5, L"(C)▽", &SCR_NOTICE1::col_dn, &dc);
			dc._btns.add(30, 5, L"△(c)", &SCR_NOTICE1::col_up, &dc);

			dc._btns.add(20 - 3, 7, L"(B)▽", &SCR_NOTICE1::bri_dn, &dc);
			dc._btns.add(30, 7, L"△(b)", &SCR_NOTICE1::bri_up, &dc);

			dc._btns.add(20 - 3, 9, L"(P)▽", &SCR_NOTICE1::pat_dn, &dc);
			dc._btns.add(30, 9, L"△(p)", &SCR_NOTICE1::pat_up, &dc);

			dc._btns.add(20 - 3, 11, L"(D)▽", &SCR_NOTICE1::dur_dn, &dc);
			dc._btns.add(30, 11, L"△(d)", &SCR_NOTICE1::dur_up, &dc);

			dc._btns.add(20, 13, MLSLW(L"消灯(x)", L" Off(x)"), &SCR_NOTICE1::turn_off, &dc);
			dc._btns.add(30, 13, MLSLW(L"点灯(SPACE)", L"  On(SPACE)"), &SCR_NOTICE1::turn_on, &dc);
#ifndef ESP32
			dc._btns.add(45, 0, MLSLW(L"ﾍﾙﾌﾟ(h)", L"HELP(h)"), &SCR_NOTICE1::web, &dc, 0);
#endif
			dc._btns.update_view();
		}
	}

	// the loop
	void loop() {
		auto& dc = *this;

		// handle events for buttons array
		dc._btns.check_events();

		// other events (keyboard)
		do {
			int c = the_keyboard.read();

			switch (c) {
			case ' ': dc.turn_on(0); break;
			case 'x': dc.turn_off(0); break;
			case 'i': dc.id_up(0); break;
			case 'I': dc.id_dn(0); break;
			case 'c': dc.col_up(0); break;
			case 'C': dc.col_dn(0); break;
			case 'b': dc.bri_up(0); break;
			case 'B': dc.bri_dn(0); break;
			case 'p': dc.pat_up(0); break;
			case 'P': dc.pat_dn(0); break;
			case 'd': dc.dur_up(0); break;
			case 'D': dc.dur_dn(0); break;
#ifndef ESP32
			case 'h': dc.web(0, 0); break;
#endif
			case KeyInput::KEY_BUTTON_B:
				dc.turn_on(0);
				break;

			default: break;
			}

		} while (the_keyboard.available());

		// update screen text (numbers, etc)
		dc.update_screen();
	}

	// the closing
	void on_close() {
		;
	}
};

void APP_BASE::SCR_NOTICE1::generate_command(bool turn_on) {
	IParser& p = _app.parse_ascii;
	auto&& a = p.get_payload();
	a.clear();

	a << uint8_t(_id);
	a << uint8_t(0x90);

	if (!turn_on) {
		a << uint8_t(0x01); // num of paramters

		a << uint8_t(0x01); // command id
		a << uint8_t(_col);
		a << uint8_t(_pat);
		a << uint8_t(0); // turn off
	}
	else {
		// normal
		a << uint8_t(0x02); // num of paramters

		a << uint8_t(0x01); // command id
		a << uint8_t(_col);
		a << uint8_t(_pat);
		a << uint8_t(_bri); // turn off

		a << uint8_t(0x02); // command id
		a << uint8_t(0xFF); // port (0xFF -> all ports)
		a << uint8_t((get_dur_s() & 0xFF00) >> 8);
		a << uint8_t((get_dur_s() & 0xFF));
	}

	_app.the_screen_b << '[' << int(millis()) << "] " << _app.parse_ascii;
	//the_sys_console << _app.parse_ascii;

	// To TWE Serial port
	TWE::WrtTWE << _app.parse_ascii;
}

void APP_BASE::SCR_NOTICE1::update_screen() {
	ITerm& t = this->_app.the_screen;

	if (_b_dirty) {
		// id
		t.move_cursor(23, 3);
		t << printfmt("%6d", _id);

		// color
		t.move_cursor(23, 5);
		const wchar_t COLS[][TWE::LANG_CT][16] = {
			{ L"\033[7;31m0:赤\033[0m", L"\033[7;31m0:Rd\033[0m" },
			{ L"\033[7;32m1:緑\033[0m", L"\033[7;32m1:Gr\033[0m" },
			{ L"\033[7;34m2:青\033[0m", L"\033[7;34m2:Bl\033[0m" },
			{ L"\033[7;33m3:黄\033[0m", L"\033[7;33m3:Ye\033[0m" },
			{ L"\033[7;35m4:紫\033[0m", L"\033[7;35m4:Pu\033[0m" },
			{ L"\033[7;36m5:水\033[0m", L"\033[7;36m5:Cy\033[0m" },
			{ L"\033[7;37m6:白\033[0m", L"\033[7;37m6:Wh\033[0m" },
			{ L"\033[7;37m7:白\033[0m", L"\033[7;37m7:Wh\033[0m" },
		};
		t << ' ' << COLS[_col][g_lang];

		// 明るさ
		t.move_cursor(23, 7);
		t << printfmt("%3d/15", _bri);

		// pat
		t.move_cursor(23, 9);
		const wchar_t PAT[][TWE::LANG_CT][16] = {
			{ L" 点灯 ", L" Con't" },
			{ L" 点滅1", L" Blnk1" },
			{ L" 点滅2", L" Blnk2" },
			{ L" 点滅3", L" Blnk3" },
		};
		t << PAT[_pat][g_lang];

		// dur
		t.move_cursor(23, 11);
		t << printfmt("%4d", get_dur_s()) << MLSLW(L"秒", L"s.");

		_b_dirty = 0;
	}
}

/**
 * create an instance of hander for SCR_NOTICE.
 */
void APP_BASE::hndr_SCR_NOTICE1(event_type ev, arg_type arg) { hndr<SCR_NOTICE1>(ev, arg); }