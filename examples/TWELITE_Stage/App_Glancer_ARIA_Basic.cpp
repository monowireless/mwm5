/* Copyright (C) 2020-2022 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "App_Glancer.hpp"

using APP_BASE = App_Glancer; //! alias to parent screen class

/** 
 * Screen ARIA BASIC, quick glance of pakects from TWELITE ARIA.
 * This is TAB implementation under APP_BASE class.
 */
struct APP_BASE::SCR_ARIA_BASIC : public APP_HANDLR_DC {
	static const int CLS_ID = APP_BASE::PAGE_ID::PAGE_SCR_ARIA;
	int get_class_id() { return CLS_ID; }

	APP_BASE& _app;
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

	SCR_ARIA_BASIC(APP_BASE& app) : _app(app), _btns(*this, app.the_screen), _pkt_rcv_ct(0)
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
	
	template <typename T>
	void print_data(const int index, TWEFMT::TwePacketPal& pal_log, T& sns) {
		uint32_t t = pal_log.common.tick + 50;
		t = t % (10000 * 1000); // loop at 9999.9sec
		the_screen << printfmt("｜ %4d.%1d｜%3d", t / 1000, (t / 100) % 10, pal_log.u8addr_src);

		// auto&& sns = pal_log.get_TweARIA();
		if (sns.has_vcc()) {
			the_screen << printfmt("｜  %4d ", sns.get_vcc_i16mV());
			if (index == _startid) the_screen_b << ":VCC=" << int(sns.get_vcc_i16mV());
		}
		else {
			the_screen << "｜         ";
		}

		// temp
		if (sns.has_temp()) {
			int16_t temp = sns.get_temp_i16_100xC();
			the_screen << printfmt("｜ %3d.%02d", temp / 100, (unsigned)temp % 100);
			if (index == _startid) the_screen_b << ":TMP=" << int(temp);
		}
		else {
			the_screen << "｜         ";
		}

		if (sns.has_humidity()) {
			uint16_t hum = sns.get_humidity_u16_100xPC();
			the_screen << printfmt("｜ %3d.%02d", hum / 100, hum % 100);
			if (index == _startid) the_screen_b << ":HUM=" << int(hum);
		}
		else {
			the_screen << "｜         ";
		}

		//the_screen << crlf;
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

			// 1. identify the packet type
			auto pkt = newTwePacket(parse_ascii.get_payload());
			the_screen_b << ":Typ=" << int(identify_packet_type(pkt));

			if (identify_packet_type(pkt) == E_PKT::PKT_PAL) {
				auto&& pal = refTwePacketPal(pkt);
				//_pal[_startid] = refTwePacketPal(pkt);
				PalEvent ev;

				// put information
				the_screen_b
					<< printfmt(":Lq=%d:Ad=%08X", pal.u8lqi, pal.u32addr_src)
					<< ":PAL=" << printfmt("%d", pal.u8palpcb)
					<< ":ID=" << printfmt("%d", pal.u8addr_src)
					<< crlf
					;

				if (pal.has_PalEvent()) {
					ev = pal.get_PalEvent();
					if (ev) the_screen_b << ":EVENT:ID=" << int(ev.u8event_id);
				}

				bool hasAmbDat = false;

				switch (pal.get_PalDataType()) {
				case E_PAL_DATA_TYPE::EX_ARIA_STD:
				{					
					the_screen_b << ":PAL_AMB";
					_pal[_startid] = pkt;
					hasAmbDat = true;
				} // case E_PAL_DATA_TYPE::EX_ARIA_STD
				break;

				case E_PAL_DATA_TYPE::AMB_STD:
				{
					the_screen_b << ":PAL_AMB";
					_pal[_startid] = pkt;
					hasAmbDat = true;
				}
				break;

				default: break;
				} // switch

				if (hasAmbDat) {
					// extended header
					the_screen_b << ":EX(" << int(pal.u8data_type) << "," << int(pal.u8_data_cause) << "," << int(pal.u8_data_cause) << ")";
					// event data
					if (ev) the_screen_b << ":EVENT=" << int(ev.u8event_id);

					// update main screen
					the_screen.clear_screen();
					the_screen.move_cursor(0, 0);
					the_screen << printfmt("■ パケット (#%04d, 種別:%02d)", _pkt_rcv_ct, identify_packet_type(_pal[_startid])) << crlf;
					the_screen << printfmt("  ID=%02d AD=0x%08X LQ=%03d SQ=%04d", pal.u8addr_src, pal.u32addr_src, pal.u8lqi, pal.u16seq);

					// list
					the_screen.move_cursor(0, 2);
					the_screen
						//   "0....+....1....+....2....+....3....+....4....+....5..
						<< L"┌-------┬---┬-------┬-------┬-------┬------┐" << crlf
						<< L"｜時間(s)｜ ID｜VCC(mV)｜温度(C)｜湿度(%)｜ 磁石 ｜" << crlf
						<< L"├-------┼---┼-------┼-------┼-------┼------┤" << crlf
						;

					for (int i = 0; i < _siz; i++) {
						int index = _startid - i;
						//int index = i;
						if (index < 0) index += _siz;
						auto&& pal_log = refTwePacketPal(_pal[index]);

						// additional mag data
						if (pal_log.get_PalDataType() == E_PAL_DATA_TYPE::EX_ARIA_STD) {
							auto sns = pal_log.get_TweARIA();
							print_data<TweARIA>(index, pal_log, sns);
							// mag
							if (sns.has_mag()) {
								TermAttr TB(TERM_COLOR_BG_BLUE | TERM_COLOR_FG_WHITE);
								TermAttr TR(TERM_COLOR_BG_RED | TERM_COLOR_FG_WHITE);
								TermAttr TC(TERM_ATTR_OFF);

								if (index == _startid) {
									the_screen_b << ":MAG=";
									the_screen_b << "(";
								}
								switch (sns.get_mag_stat_u8() & 0x7F) {
								case 0: the_screen << L"｜ なし"; if (index == _startid) the_screen_b << "NO MANGET"; break;
								case 1: the_screen << "｜" << TR << L"[Ｎ極]" << TC << "｜"; if (index == _startid) the_screen_b << "N POLE"; break;
								case 2: the_screen << "｜" << TB << L"[Ｓ極]" << TC << "｜"; if (index == _startid) the_screen_b << "S POLE"; break;
								}
								if (index == _startid) the_screen_b << ")";
							}
							else {
								the_screen << "｜ ---- ｜";
							}

							the_screen << crlf;
						}
						else if (pal_log.get_PalDataType() == E_PAL_DATA_TYPE::AMB_STD) {
							auto sns = pal_log.get_PalAmb();
							print_data<PalAmb>(index, pal_log, sns);

							the_screen << "｜ ---- ｜";
							the_screen << crlf;
						}

					}
					the_screen << L"└-------┴---┴-------┴-------┴-------┴------┘" << crlf;

					// if has data, save it into _pal[]
					_startid++;
					if (_startid >= _siz) {
						_startid = 0;
					}
				}

			}
		}
	}

	void setup() {
		the_screen.clear_screen();
		the_screen_b.clear_screen();
		_app.set_title_bar(PAGE_ID::PAGE_SCR_ARIA);
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
void APP_BASE::hndr_SCR_ARIA_BASIC(event_type ev, arg_type arg) { hndr<SCR_ARIA_BASIC>(ev, arg); }
