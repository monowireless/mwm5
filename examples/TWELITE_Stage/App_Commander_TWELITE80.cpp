/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_Commander.hpp"

using APP_BASE = App_Commander;

struct  APP_BASE::SCR_TWELITE80 : public APP_HANDLR_DC {
	static const int CLS_ID = APP_BASE::PAGE_ID::PAGE_TWELITE80;
	int get_class_id() { return CLS_ID; }

	APP_BASE& _app;
	TWE_WidSet_Buttons _btns;

	// constructor
	SCR_TWELITE80(APP_BASE& app) : _app(app), _btns(*this, app.the_screen)
		, _b_dirty(1), _id(-1)
		, _di_state{ 1,1,0,0 }
		, _di_sel{ 1,1,0,0 }
		, _pwm{ 8,11,11,11 }
		, APP_HANDLR_DC(CLS_ID) {}

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
	int8_t _di_state[4]; // 0,1
	int8_t _di_sel[4]; // 0,1
	int8_t _pwm[4]; // 0..10:0-100%, 11:disabled

public:
	// id
	void id_up(int id, uint32_t opt = 0) {
		_incr(_id, -1, 8);
		generate_command();
	}
	void id_dn(int id, uint32_t opt = 0) {
		_decr(_id, -1, 8);
		generate_command();
	}

	// di
	void di_state(int id, uint32_t opt = 0) {
		_incr(_di_state[opt], 0, 1);
		generate_command();
	}

	// di selection bitmap
	void di_sel(int id, uint32_t opt = 0) {
		_incr(_di_sel[opt], 0, 1);
		generate_command();
	}

	// pwm
	void pwm(int id, uint32_t opt = 0) {
		_incr(_pwm[opt], 0, 11);
		generate_command();
	}

	// transmit with current settings
	void fire(int id, uint32_t opt = 0) {
		generate_command();
	}

	// open web site
#ifndef ESP32
	void web(int id, uint32_t opt = 0) {
		shell_open_url(L"https://mono-wireless.com/jp/products/TWE-APPS/App_Twelite/step3-80.html");
	}
#endif

	// screen updator
	void update_screen();

	// parser command and transmit
	void generate_command(bool turn_on = true);

	// setup
	void setup() {
		auto& dc = *this;

		// layout screen.
		_app.screen_layout_apps();

		// set title bar
		_app.set_title_bar(PAGE_ID::PAGE_TWELITE80);

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
			auto& t = _app.the_screen; t.clear_screen();

			// put base screen text
			t.clear_screen();

			//   "0....+....1....+....2....+....3....+....4....+....5...
			t << crlf;
			t << MLSLW(
				L"             [標準ｱﾌﾟﾘ 0x80ｺﾏﾝﾄﾞ]",
				L"             [Std App  0x80 cmd]") << crlf;     // L=1
			t << crlf;
			t << MLSLW(
				L"    宛先ID       (I)▽        △(i)",
				L"    DestID       (I)▽        △(i)") << crlf; // L=3
			t << crlf;
			//   "0....+....1....+....2....+....3....+....4....+....5...
			t << "    DI1(1) 〇   DI2(2) 〇   DI3(3) 〇   DI4(4) 〇 " << crlf; // L=5
			t << "    SEL(q) Ｙ   SEL(w) Ｙ   SEL(r) Ｙ   SEL(t) Ｙ " << crlf; // L=6
			t << crlf; // L=7
			t << "    PWM1(a) |0123456789|    PWM2(s) |0123456789|" << crlf; // L=8
			t << "    PWM3(d) |0123456789|    PWM4(f) |0123456789|" << crlf; // L=9
			t << crlf; // L=10

			t << MLSLW(
				L"                                        送信(t)",
				L"                                        Trns(t)");

			// add buttons
			dc._btns.clear();

			int idx = 0;
			idx = dc._btns.add(20 - 3, 3, L"(I)▽", &SCR_TWELITE80::id_dn, &dc);
			idx = dc._btns.add(30, 3, L"△(i)", &SCR_TWELITE80::id_up, &dc);

			idx = dc._btns.add(4, 5, L"DI1(1)", &SCR_TWELITE80::di_state, &dc, 0);
			dc._btns[idx].set_additional_hot_area({ 4, 5, 9, 1 }); // set additional hot area.
			idx = dc._btns.add(16, 5, L"DI2(2)", &SCR_TWELITE80::di_state, &dc, 1);
			dc._btns[idx].set_additional_hot_area({ 16, 5, 9, 1 });
			idx = dc._btns.add(28, 5, L"DI3(3)", &SCR_TWELITE80::di_state, &dc, 2);
			dc._btns[idx].set_additional_hot_area({ 28, 5, 9, 1 });
			idx = dc._btns.add(40, 5, L"DI4(4)", &SCR_TWELITE80::di_state, &dc, 3);
			dc._btns[idx].set_additional_hot_area({ 40, 5, 9, 1 });

			idx = dc._btns.add(4, 6, L"SEL(q)", &SCR_TWELITE80::di_sel, &dc, 0);
			dc._btns[idx].set_additional_hot_area({ 4, 6, 9, 1 });
			idx = dc._btns.add(16, 6, L"SEL(w)", &SCR_TWELITE80::di_sel, &dc, 1);
			dc._btns[idx].set_additional_hot_area({ 16, 6, 9, 1 });
			idx = dc._btns.add(28, 6, L"SEL(r)", &SCR_TWELITE80::di_sel, &dc, 2);
			dc._btns[idx].set_additional_hot_area({ 28, 6, 9, 1 });
			idx = dc._btns.add(40, 6, L"SEL(t)", &SCR_TWELITE80::di_sel, &dc, 3);
			dc._btns[idx].set_additional_hot_area({ 40, 6, 9, 1 });

			idx = dc._btns.add(4, 8, L"PWM1(a)", &SCR_TWELITE80::pwm, &dc, 0);
			dc._btns[idx].set_additional_hot_area({ 4, 8, 19, 1 });
			idx = dc._btns.add(28, 8, L"PWM2(s)", &SCR_TWELITE80::pwm, &dc, 1);
			dc._btns[idx].set_additional_hot_area({ 28, 8, 19, 1 });
			idx = dc._btns.add(4, 9, L"PWM3(d)", &SCR_TWELITE80::pwm, &dc, 2);
			dc._btns[idx].set_additional_hot_area({ 4, 9, 19, 1 });
			idx = dc._btns.add(28, 9, L"PWM4(f)", &SCR_TWELITE80::pwm, &dc, 3);
			dc._btns[idx].set_additional_hot_area({ 28, 9, 19, 1 });

			idx = dc._btns.add(40, 11, MLSLW(L"送信(SPACE)", L"Trns(SPACE)"), &SCR_TWELITE80::fire, &dc, 0);

#ifndef ESP32
			idx = dc._btns.add(45, 0, MLSLW(L"ﾍﾙﾌﾟ(h)", L"HELP(h)"), &SCR_TWELITE80::web, &dc, 0);
#endif

			dc._btns.update_view();
		}
	}

	// the loop()
	void loop() {
		auto& dc = *this;

		// handle events for buttons array
		dc._btns.check_events();

		// other events (keyboard)
		do {
			int c = the_keyboard.read();

			switch (c) {
			case 'i': dc.id_up(0); break;
			case 'I': dc.id_dn(0); break;
			case '1': dc.di_state(0, 0); break;
			case '2': dc.di_state(0, 1); break;
			case '3': dc.di_state(0, 2); break;
			case '4': dc.di_state(0, 3); break;
			case 'q': dc.di_sel(0, 0); break;
			case 'w': dc.di_sel(0, 1); break;
			case 'e': dc.di_sel(0, 2); break;
			case 'r': dc.di_sel(0, 3); break;
			case 'a': dc.pwm(0, 0); break;
			case 's': dc.pwm(0, 1); break;
			case 'd': dc.pwm(0, 2); break;
			case 'f': dc.pwm(0, 3); break;
			case ' ':
			case KeyInput::KEY_BUTTON_B:
				dc.fire(0);
				break;
#ifndef ESP32
			case 'h': dc.web(0, 0); break;
#endif

			default: break;
			}

		} while (the_keyboard.available());

		// update screen text (numbers, etc)
		dc.update_screen();
	}

	// the on close
	void on_close() {
		;
	}
};

void APP_BASE::SCR_TWELITE80::generate_command(bool turn_on) {
	IParser& p = _app.parse_ascii;
	auto&& a = p.get_payload();
	a.clear();

	uint8_t u8di = 0;
	for (int i = 0; i < 4; i++) u8di |= (_di_state[i] << i);

	uint8_t u8di_sel = 0;
	for (int i = 0; i < 4; i++) u8di_sel |= (_di_sel[i] << i);

	uint16_t u16pwm[4];
	for (int i = 0; i < 4; i++) {
		if (_pwm[i] == 11) {
			u16pwm[i] = 0xFFFF;
		}
		else {
			u16pwm[i] = _pwm[i] * 1024 / 10;
		}
	}

	// generating data
	a << uint8_t(_id == -1 ? 0x78 : _id);
	a << uint8_t(0x80);
	a << uint8_t(0x01);
	a << u8di;
	a << u8di_sel;

	for (int i = 0; i < 4; i++) {
		a << (u16pwm[i] >> 8);
		a << (u16pwm[i] & 0xFF);
	}

	_app.the_screen_b << '[' << int(millis()) << "] " << _app.parse_ascii;
	//the_sys_console << _app.parse_ascii;

	// To TWE Serial port
	TWE::WrtTWE << _app.parse_ascii;
}

void APP_BASE::SCR_TWELITE80::update_screen() {
	ITerm& t = this->_app.the_screen;

	if (_b_dirty) {
		// id
		t.move_cursor(23, 3);
		t << "      "; // clean up with blank
		t.move_cursor(23, 3);
		switch (_id) {
		case -1: t << MLSLW(L"全子機", L"Chld:*"); break;
		case 0: t << MLSLW(L"親機:0", L"Prnt:0"); break;
		default: t << MLSLW(L"子機:", L"Chld:") << printfmt("%d", _id);
		}

		// pwm
		const uint8_t COORD_PWM[4][2] = {
			{13,8}, {37,8}, {13,9}, {37,9}
		};

		for (int i = 0; i < 4; i++) {
			t.move_cursor(11 + i * 12, 5);

			if (_di_state[i]) {
				t << printfmt("\033[%dm■\033[0m", 31 + i);
			}
			else {
				t << "□";
			}
			
			t.move_cursor(11 + i * 12, 6);
			t << (_di_sel[i] ? "１" : "０");

			t.move_cursor(COORD_PWM[i][0], COORD_PWM[i][1]);
			if (_pwm[i] == 11) {
				t << "   N.A.   ";
			}
			else {
				char buff[16];
				TWE_snprintf(buff, 16, "   %3d%%      ", _pwm[i] * 10);
#if 1
				// bar starts from the left
				for (int j = 0; j < 10; j++) {
					if (_pwm[i] <= j) {
						t << char_t(buff[j]);
					}
					else {
						t << "\033[33;7m" << char_t(buff[j]) << "\033[0m";
					}
				}
#else
				// bar starts from the right
				for (int j = 10; j > 0; j--) {
					if (_pwm[i] >= j) {
						t << "\033[33;7m" << char_t(buff[10 - j]) << "\033[0m";
					}
					else {
						t << char_t(buff[10 - j]);
					}
				}
#endif
			}
		}
		
		_b_dirty = 0;
	}
}


/**
 * create an instance of hander for SCR_TWELITE80.
 */
void APP_BASE::hndr_SCR_TWELITE80(event_type ev, arg_type arg) { hndr<SCR_TWELITE80>(ev, arg); }
