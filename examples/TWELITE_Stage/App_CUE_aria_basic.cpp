/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_CUE.hpp"

struct App_CUE::SCR_ARIA_BASIC : public APP_HANDLR_DC {
	static const int CLS_ID = App_CUE::PAGE_ID::PAGE_ARIA_BASIC;
	int get_class_id() { return CLS_ID; }

	App_CUE& _app;
	TWE_WidSet_Buttons _btns;
	int _pkt_rcv_ct;

	// object references to the App_ARIA
	ITerm& the_screen;
	ITerm& the_screen_b;
	IParser& parse_ascii;

	// Aria viewer log
	static const int _siz = 9;
	uint8_t _startid = 0;
	spTwePacket _pal[_siz];

	SCR_ARIA_BASIC(App_CUE& app) : _app(app), _btns(*this, app.the_screen), _pkt_rcv_ct(0)
		, the_screen(app.the_screen), the_screen_b(app.the_screen_b), parse_ascii(app.parse_ascii)
		, APP_HANDLR_DC(CLS_ID) {}
	~SCR_ARIA_BASIC() {}

	void Btn_Press(int id, uint32_t opt = 0) {
		_app._tabs.select(opt);
	}

	void show_message() {
		auto& t = _app.the_screen;

		//     "0....+....1....+....2....+....3....+....4....+....5..
		the_screen(0, 6)
			<< "  -- TWELITE ARIAからのﾒｯｾｰｼﾞを解釈・表示します --" << crlf
			;
	}

	// put a byte to parse
	void parse_a_byte(char_t u8b) {
		// parse.
		parse_ascii << u8b;

		// if complete parsing
		if (parse_ascii) {
			// output as parser format
			the_screen_b.clear_screen();
			the_screen_b << "PKT(" << ++_pkt_rcv_ct << ')';

			the_screen.clear_screen();

			// 1. identify the packet type
			_pal[_startid] = newTwePacket(parse_ascii.get_payload());
			the_screen_b << ":Typ=" << int(identify_packet_type(_pal[_startid]));

			the_screen.move_cursor(0, 0);
			the_screen << printfmt("■ パケット (#%04d, 種別:%02d)", _pkt_rcv_ct, identify_packet_type(_pal[_startid])) << crlf;

			if (identify_packet_type(_pal[_startid]) == E_PKT::PKT_PAL) {
				auto&& pal = refTwePacketPal(_pal[_startid]);
				//_pal[_startid] = refTwePacketPal(pkt);
				PalEvent ev;

				the_screen << printfmt("  ID=%02d AD=0x%08X LQ=%03d SQ=%04d", pal.u8addr_src, pal.u32addr_src, pal.u8lqi, pal.u16seq);

				// put information
				the_screen_b
					<< printfmt(":Lq=%d:Ad=%08X", pal.u8lqi, pal.u32addr_src)
					<< ":PAL=" << int(pal.u8palpcb)
					<< ":ID=" << int(pal.u8addr_src)
					<< crlf
					;

				switch (pal.get_PalDataType()) {
				case E_PAL_DATA_TYPE::EVENT_ONLY:
				{
					if (ev) the_screen_b << ":EVENT:ID=" << int(ev.u8event_id);
				}
				break;

				case E_PAL_DATA_TYPE::EX_ARIA_STD:
				{					
					the_screen_b << ":ARIA";
					// extended header
					the_screen_b << ":EX(" << int(pal.u8data_type) << "," << int(pal.u8_data_cause) << "," << int(pal.u8_data_cause) << ")";
					// event data
					if (ev) the_screen_b << ":EVENT=" << int(ev.u8event_id);

					the_screen.move_cursor(0, 3);
					the_screen <<
						//     "0....+....1....+....2....+....3....+....4....+....5..
						"｜時間(s)｜ ID｜VCC(mV)｜温度(C)｜湿度(%)｜ 磁石 ｜" << crlf <<
						"├-------┼---┼-------┼-------┼-------┼------┤" << crlf;

					{
						for (int i = 0; i < _siz; i++) {
							int index = _startid - i;
							//int index = i;
							if(index<0) index += _siz;
							auto&& pal_log = refTwePacketPal(_pal[index]);

							if(pal_log.get_PalDataType() == E_PAL_DATA_TYPE::EX_ARIA_STD){
								//the_screen <<
									//     "0....+....1....+....2....+....3....+....4....+....5..
								//	"├-------┼---┼--------┼-------┼-------┼------┤" << crlf;
								uint32_t t = pal_log.common.tick + 50;
								t = t % (10000 * 1000); // loop at 9999.9sec
								the_screen << printfmt("｜ %4d.%1d｜%3d", t / 1000, (t/100)%10, pal_log.u8addr_src);

								auto&& aria = pal_log.get_TweARIA();
								if (aria.has_vcc()) {
									the_screen << printfmt("｜  %4d ", aria.get_vcc_i16mV());
									if (index == _startid) the_screen_b << ":VCC=" << int(aria.get_vcc_i16mV());
								}
								else {
									the_screen << "｜         ";
								}

								// temp
								if (aria.has_temp()) {
									int16_t temp = aria.get_temp_i16_100xC();
									the_screen << printfmt("｜ %3d.%02d", temp/100 , (unsigned)temp%100);
									if(index == _startid) the_screen_b << ":TMP=" << int(temp);
								}
								else {
									the_screen << "｜         ";
								}

								if (aria.has_humidity()) {
									uint16_t hum = aria.get_humidity_u16_100xPC();
									the_screen << printfmt("｜ %3d.%02d", hum/100, hum%100 );
									if(index == _startid) the_screen_b << ":HUM=" << int(hum);
								}
								else {
									the_screen << "｜         ";
								}

								// mag
								if (aria.has_mag()) {
									TermAttr TB(TERM_COLOR_BG_BLUE | TERM_COLOR_FG_WHITE);
									TermAttr TR(TERM_COLOR_BG_RED | TERM_COLOR_FG_WHITE);
									TermAttr TC(TERM_ATTR_OFF);

									if (index == _startid){
										the_screen_b << ":MAG=";
										the_screen_b << "(";
									}
									switch (aria.get_mag_stat_u8() & 0x7F) {
										case 0: the_screen << L"｜ ---- ｜"; if (index == _startid) the_screen_b << "NO MANGET"; break;
										case 1: the_screen << "｜" << TR << L"[Ｎ極]" << TC << "｜"; if (index == _startid) the_screen_b << "N POLE"; break;
										case 2: the_screen << "｜" << TB << L"[Ｓ極]" << TC << "｜"; if (index == _startid) the_screen_b << "S POLE"; break;
									}
									if (index == _startid) the_screen_b << ")";
								}
								else {
									the_screen << "｜         ";
								}
								the_screen << crlf;
							}
						}
					}
					_startid++;
					if (_startid >= _siz) {
						_startid = 0;
					}

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
						if (cue.has_vcc()) {
							the_screen_b << ":VCC=" << int(cue.get_vcc_i16mV());
						}

#if 0
						// adc1
						if (cue.has_adc1()) {
							the_screen_b << ":AD1=" << int(cue.get_adc1_i16mV());
						}
#endif
					}


					// mag
					if (cue.has_mag()) {
						TermAttr TB(TERM_COLOR_BG_BLUE | TERM_COLOR_FG_WHITE);
						TermAttr TR(TERM_COLOR_BG_RED | TERM_COLOR_FG_WHITE);
						TermAttr TC(TERM_ATTR_OFF);

						the_screen_b << ":MAG=";
						the_screen_b << "(";
						switch (cue.get_mag_stat_u8() & 0x7F) {
						case 0: the_screen_b << "NO MANGET"; break;
						case 1: the_screen_b << "N POLE"; break;
						case 2: the_screen_b << "S POLE"; break;
						}
						the_screen_b << ")";
					}

					// mot
					if (cue.has_accel()) {
						the_screen_b << "MOT";
						the_screen_b << ":SAMPLES=" << int(cue.get_accel_count_u8());
						the_screen_b << ":SR=" << int(cue.u8sample_rate_code);
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
};

// Screen def: opening
void App_CUE::hndr_aria_basic(event_type ev, arg_type arg) {
	// renew object
	auto&& dc = APP_HNDLR::use<SCR_ARIA_BASIC>();

	switch (ev) {
	case EV_SETUP:
		the_screen.clear_screen();
		the_screen_b.clear_screen();
		set_title_bar(PAGE_ID::PAGE_BASIC);
		set_nav_bar();
		dc.show_message();
		break;

	case EV_LOOP:
		dc._btns.check_events();

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

			if (c >= 0) dc.parse_a_byte(c);

		} while (the_uart_queue.available());
		break;

	case EV_EXIT:
		break;
	}
}