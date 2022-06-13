/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_CUE.hpp"

struct App_CUE::SCR_CUE_BASIC : public APP_HANDLR_DC {
	static const int CLS_ID = App_CUE::PAGE_ID::PAGE_BASIC;
	int get_class_id() { return CLS_ID; }

	App_CUE& _app;
	TWE_WidSet_Buttons _btns;
	int _pkt_rcv_ct;

	// object references to the App_CUE
	ITerm& the_screen;
	ITerm& the_screen_b;
	IParser& parse_ascii;

	SCR_CUE_BASIC(App_CUE& app) : _app(app), _btns(*this, app.the_screen), _pkt_rcv_ct(0)
			, the_screen(app.the_screen), the_screen_b(app.the_screen_b), parse_ascii(app.parse_ascii)
			, APP_HANDLR_DC(CLS_ID) {}
	~SCR_CUE_BASIC() {}

	void Btn_Press(int id, uint32_t opt = 0) {
		_app._tabs.select(opt);
	}

	void show_message() {
		auto& t = _app.the_screen;

		//     "0....+....1....+....2....+....3....+....4....+....5..
		the_screen(0,6)
			<< "  -- TWELITE CUEからのﾒｯｾｰｼﾞを解釈・表示します --" << crlf
			;
	}

	void print_event(PalEvent &ev, int line) {
		the_screen.move_cursor(0, line);
		the_screen << printfmt("■ イベント (ID=0x%02x SRC=0x%02x) = ", ev.u8event_id, ev.u8event_source);

		int l = line + 1;
		int c = 16;
		TermAttr TB(TERM_COLOR_FG_BLACK | TERM_COLOR_BG_WHITE);
		TermAttr TR(TERM_COLOR_FG_RED | TERM_COLOR_BG_WHITE);
		TermAttr TC(TERM_ATTR_OFF);

		switch (ev.e_event_accel) {
			case E_EVENT_ACCEL::DICE_1:
			case E_EVENT_ACCEL::DICE_2:
			case E_EVENT_ACCEL::DICE_3:
			case E_EVENT_ACCEL::DICE_4:
			case E_EVENT_ACCEL::DICE_5:
			case E_EVENT_ACCEL::DICE_6:
			{
				the_screen << printfmt("ダイス") << crlf;


				switch (ev.u8event_id) {
				case 1:
					the_screen(c, l + 0) << TR << "　　　" << TC;
					the_screen(c, l + 1) << TR << "　●　" << TC;
					the_screen(c, l + 2) << TR << "　　　" << TC;
					break;
				case 2:
					the_screen(c, l + 0) << TB << "●　　" << TC << crlf;
					the_screen(c, l + 1) << TB << "　　　" << TC << crlf;
					the_screen(c, l + 2) << TB << "　　●" << TC << crlf;
					break;
				case 3:
					the_screen(c, l + 0) << TB << "●　　" << TC << crlf;
					the_screen(c, l + 1) << TB << "　●　" << TC << crlf;
					the_screen(c, l + 2) << TB << "　　●" << TC << crlf;
					break;
				case 4:
					the_screen(c, l + 0) << TB << "●　●" << TC << crlf;
					the_screen(c, l + 1) << TB << "　　　" << TC << crlf;
					the_screen(c, l + 2) << TB << "●　●" << TC << crlf;
					break;
				case 5:
					the_screen(c, l + 0) << TB << "●　●" << TC << crlf;
					the_screen(c, l + 1) << TB << "　●　" << TC << crlf;
					the_screen(c, l + 2) << TB << "●　●" << TC << crlf;
					break;
				case 6:
					the_screen(c, l + 0) << TB << "●　●" << TC << crlf;
					the_screen(c, l + 1) << TB << "●　●" << TC << crlf;
					the_screen(c, l + 2) << TB << "●　●" << TC << crlf;
					break;
				}

			}
			break;

			case E_EVENT_ACCEL::SHAKE:
			{
				the_screen << printfmt("シェイク") << crlf;

				the_screen(c, l + 0) << TB << "    _____    " << TC << " SHAKE!";
				the_screen(c, l + 1) << TB << "(( ⇔___/| ))" << TC;
				the_screen(c, l + 2) << TB << "   |____|/   " << TC;
			}
			break;

			case E_EVENT_ACCEL::MOVE:
			{
				the_screen << printfmt("ムーブ") << crlf;

				the_screen(c, l + 0) << TB << "    _____    " << TC << " ACCEL! ";
				the_screen(c, l + 1) << TB << " ⊃★___/|   " << TC;
				the_screen(c, l + 2) << TB << "   |____|/ ミ" << TC;
			}
			break;

			default:
			{
				the_screen << printfmt("その他") << crlf;
			}
			break;
		}
	}

	// put a byte to parse
	void parse_a_byte(char_t u8b) {
		// parse.
		parse_ascii << u8b;

		// if complete parsing
		if (parse_ascii) {
			// output as parser format
			the_screen_b.clear_screen();
			the_screen_b << "PKT(" << _pkt_rcv_ct++ << ')';

			the_screen.clear_screen();

			// payload
			auto&& p = parse_ascii.get_payload();

			// 1. identify the packet type
			auto&& pkt = newTwePacket(parse_ascii.get_payload());
			the_screen_b << ":Typ=" << int(identify_packet_type(pkt));

			the_screen.move_cursor(0, 0);
			the_screen << printfmt("■ パケット (#%04d, 種別:%02d)", _pkt_rcv_ct, identify_packet_type(pkt)) << crlf;
			
			if (identify_packet_type(pkt) == E_PKT::PKT_PAL) {
				auto&& pal = refTwePacketPal(pkt);
				PalEvent ev;

				the_screen << printfmt("  ID=%02d AD=0x%08X LQ=%03d SQ=%04d", pal.u8addr_src, pal.u32addr_src, pal.u8lqi, pal.u16seq);

				// acquire event data.
				if (pal.has_PalEvent()) {
					ev = pal.get_PalEvent();
					print_event(ev, 3); // takes 4lines
				}

				// put information
				the_screen_b
					<< printfmt(":Lq=%d:Ad=%08X", pal.u8lqi, pal.u32addr_src)
					<< ":PAL=" << printfmt("%d", pal.u8palpcb)
					<< ":ID=" << printfmt("%d", pal.u8addr_src)
					<< crlf
					;

				switch (pal.get_PalDataType()) {
					case E_PAL_DATA_TYPE::EVENT_ONLY:
					{
						if (ev) the_screen_b << ":EVENT:ID=" << int(ev.u8event_id);
					}
					break;

					case E_PAL_DATA_TYPE::EX_CUE_STD:
					{
						// generate TWELITE CUE standard data
						TweCUE cue = pal.get_TweCUE();

						the_screen_b << ":CUE";

						// extended header
						the_screen_b << ":EX(" << int(pal.u8data_type) << "," << int(pal.u8_data_cause) << "," << int(pal.u8_data_cause) << ")";

						// event data
						if (ev) the_screen_b << ":EVENT=" << int(ev.u8event_id);

						// volt
						{
							int ct = 0;

							the_screen.move_cursor(0, 8);
							the_screen << L"■ 電圧";
							if (cue.has_vcc()) {
								ct++;
								the_screen_b << ":VCC=" << int(cue.get_vcc_i16mV());
								the_screen << printfmt(" VCC=%04dmV", cue.get_vcc_i16mV());
							}

#if 0
							// adc1
							if (cue.has_adc1()) {
								ct++;
								the_screen_b << ":AD1=" << int(cue.get_adc1_i16mV());
								the_screen << printfmt(" AD1=%04dmV", cue.get_adc1_i16mV());
							}
#endif

							if (ct == 0) the_screen << L" データなし";
						}


						// mag
						the_screen.move_cursor(0, 10);
						the_screen << L"■ 磁石 ";
						if (cue.has_mag()) {
							TermAttr TB(TERM_COLOR_BG_BLUE | TERM_COLOR_FG_WHITE);
							TermAttr TR(TERM_COLOR_BG_RED | TERM_COLOR_FG_WHITE);
							TermAttr TC(TERM_ATTR_OFF);

							the_screen_b << ":MAG=";
							the_screen_b << "(";
							switch (cue.get_mag_stat_u8() & 0x7F) {
							case 0: the_screen_b << "NO MANGET"; the_screen << L"検出無し"; break;
							case 1: the_screen_b << "N POLE"; the_screen << TR << L"[Ｎ極]" << TC; break;
							case 2: the_screen_b << "S POLE"; the_screen << TB << L"[Ｓ極]" << TC; break;
							}
							the_screen_b << ")";
						}
						else {
							the_screen << L" データなし";
						}

						// mot
						the_screen.move_cursor(0, 12);
						the_screen << L"■ 加速度 ";
						if (cue.has_accel()) {
							the_screen << printfmt("(ｻﾝﾌﾟﾙ=%02d, ﾚｰﾄID=%02d)", cue.get_accel_count_u8(), cue.u8sample_rate_code) << crlf;
							the_screen_b << "MOT";
							the_screen_b << ":SAMPLES=" << int(cue.get_accel_count_u8());
							the_screen_b << ":SR=" << int(cue.u8sample_rate_code);

							int ave_x = 0, ave_y = 0, ave_z = 0;
							if (cue.get_accel_count_u8() >= 8) {
								for (int i = 0; i < 8; i++) {
									ave_x += cue.get_accel_X_i16mG(i);
									ave_y += cue.get_accel_Y_i16mG(i);
									ave_z += cue.get_accel_Z_i16mG(i);
								}

								ave_x >>= 3;
								ave_y >>= 3;
								ave_z >>= 3;

								the_screen_b << ":" << "(" << ave_x;
								the_screen_b << "," << ave_y;
								the_screen_b << "," << ave_z;
								the_screen_b << ")";

								the_screen << printfmt("   X=%04dmG", ave_x);
								the_screen << printfmt(", Y=%04dmG", ave_y);
								the_screen << printfmt(", Z=%04dmG", ave_z);
							}
						}
						else {
							the_screen << L" データなし";
						}
					}
					break;

					default: break;
				}

			}
		}
	}

	void setup() {
		the_screen.clear_screen();
		the_screen_b.clear_screen();
		_app.set_title_bar(PAGE_ID::PAGE_BASIC);
		_app.set_nav_bar();
		show_message();
	}

	void loop() {
		_btns.check_events();

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

		// read the uart queue
		do {
			int c = the_uart_queue.read();

			if (c >= 0) parse_a_byte(c);

		} while (the_uart_queue.available());
	}

	void on_close() {
		; // do nothing
	}
};

// generate handler instance (SCR_XXX needs to have setup(), loop(), on_close() methods)
void App_CUE::hndr_SCR_CUE_BASIC(event_type ev, arg_type arg) { hndr<SCR_CUE_BASIC>(ev, arg); }
