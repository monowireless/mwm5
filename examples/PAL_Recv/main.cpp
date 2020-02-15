/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

#include <mwm5.h>
#include "common.hpp"

#include <Arduino.h>
#include <M5Stack.h>

/**
 * @struct	pkt_data
 *
 * @brief	PAL data management by ID and display them.
 *
 */
struct pkt_data_and_view {
	static const int _siz = 32; // entry max
	spTwePacket _pal[_siz]; // store Packet Data

	int _page;      // start from 0
	int _lines;     // lines to display entry
	int _max_entry; // max ID (1...max)
	bool _bwide;    // if screen col >= 38, set true
	bool _bdirty;   // if update all is required

	ITerm& _trm;           // screen for main content
	ITerm& _trm_status;    // status screen
	const char* _fmt_status; // status message template

	pkt_data_and_view(ITerm& trm, ITerm& trm_status) : _trm(trm), _trm_status(trm_status) {}

	void init_screen(const char *fmt_status) {
		_fmt_status = fmt_status;
		reinit_screen();
	}

	/**
	 * @fn	void reinit_screen()
	 *
	 * @brief	Reinitializes the screen, in case the screen size has been changed.
	 *
	 */
	void reinit_screen() {
		int scrw = _trm.get_width();
		int scrh = _trm.get_height();

		_page = 0;
		_lines = scrh;
		_bwide = (scrw >= 38);

		_bdirty = true;

		_trm << "\033[2J\033[H"; // clear screen (by ESC seq)
		update_status();
	}

	/**
	 * @fn	bool add_entry(spTwePacketPal pal)
	 *
	 * @brief	Adds an entry of PAL packet.
	 *
	 * @param	pal PAL object to be added.
	 *
	 * @returns	True if it succeeds, false if it fails.
	 */
	bool add_entry(spTwePacket spobj) {
		if (identify_packet_type(spobj) == E_PKT::PKT_PAL) {
			auto&& pal = refTwePacketPal(spobj);
			
			if (pal.u8addr_src >= 1 && pal.u8addr_src <= _siz) {
				_pal[pal.u8addr_src - 1] = spobj;
				if (_max_entry < pal.u8addr_src) {
					_max_entry = pal.u8addr_src;
					_bdirty = true;
				}

				return true;
			}
		}

		return false;
	}

	// show next page
	void next_page() {
		_page++;

		if (_page * _lines + 1 > _max_entry) {
			_page = 0; // switch to the top
		}
	}

	// show prev page
	void prev_page() {
		_page--;

		if (_page < 0) {
			_page = _max_entry / _lines;
		}
	}

	// set page
	void set_page(int entry) {
		if (entry > 0 && entry < _max_entry) {
			_page = entry / _lines;
		}
	}

	/**
	 * @fn	void update_status()
	 *
	 * @brief	Updates the status string.
	 *
	 */
	void update_status() {
		int max_page = (_max_entry - 1) / _lines;
		char buff[16];

		snPrintf(buff, sizeof(buff), "[%d/%d]", _page + 1, max_page + 1);

		_trm_status.clear_screen();
		_trm_status << printfmt(_fmt_status, buff);
	}

	/**
	 * @fn	void update_term()
	 *
	 * @brief	Redraw whole screen.
	 *
	 */
	void update_term() {
		update_term(spTwePacketPal(), true);
	}

	/**
	 * @fn	void update_term(spTwePacketPal pal_upd, bool update_all)
	 *
	 * @brief	Updates the term
	 *
	 * @param	pal_upd   	The pal data to be updated.
	 * @param	update_all	True to update all lines, other than pal_upd.
	 */
	void update_term(spTwePacket pal_upd, bool update_all) {
		int idx_start = _page * _lines + 1;
		int idx_end = idx_start + _lines;

		// check the index
		int idx = -1;
		if (pal_upd) {
			switch (identify_packet_type(pal_upd)) {
			case E_PKT::PKT_PAL: idx = refTwePacketPal(pal_upd).u8addr_src; break;
			default: break;
			}
		}

		// when max entry count has been changed, this flag is set.
		if (_bdirty) {
			update_all = true;
			_bdirty = false;
		}

		if (update_all) {
			// update all
			_trm << "\033[2J\033[H"; // clear screen (by ESC seq)
		}
		else {
			if (!(idx >= idx_start && idx < idx_end)) {
				return; // out of scope (not update)
			}
		}

		// displays each sensor data.
		for (int i = idx_start; i < idx_end && i <= _siz; i++) {
			auto&& spobj = _pal[i - 1];

			// show IDs at head.
			if (update_all || (spobj && spobj == pal_upd)) {
				_trm << printfmt("\033[%d;1H\033[K", i - idx_start + 1) // move cursor and clear the line
					<< printfmt("%2d:", i); // ID:
			}

			// skip when the corresponding ID is not updated.
			if (!spobj || ((update_all == false) && spobj != pal_upd)) continue; // not updated

			// display sensor data
			if (identify_packet_type(spobj) == E_PKT::PKT_PAL) {
				auto&& pal = refTwePacketPal(spobj);

				switch (pal.u8palpcb) {
				case E_PAL_PCB::MAG:
				{
					// generate pal board specific data structure.
					PalMag mag = pal.get_PalMag();

					_trm << TermAttr(TERM_COLOR_BG_BLACK | TERM_COLOR_FG_CYAN);
					_trm << "開閉";
					_trm << TermAttr(TERM_ATTR_OFF);

					_trm << ':';

					if (mag.u8MagStat == 0) {
						_trm << TermAttr(TERM_COLOR_FG_RED | TERM_BOLD);
						_trm << "OPEN(開)";
						_trm << TermAttr(TERM_ATTR_OFF);
					}
					else {
						_trm << "CLOSE(閉)";
					}
				} break;

				case E_PAL_PCB::AMB:
				{
					// generate pal board specific data structure.
					PalAmb amb = pal.get_PalAmb();

					_trm << TermAttr(TERM_COLOR_BG_BLACK | TERM_COLOR_FG_GREEN);
					_trm << printfmt("環境", pal.u8addr_src);
					_trm << TermAttr(TERM_ATTR_OFF);
					_trm << ":";

					_trm << TermAttr(TERM_COLOR_FG_RED | TERM_BOLD);
					_trm << printfmt(_bwide ? "温度=%02.1f℃" : "%02.1fC", (double)amb.i16Temp / 100.0);
					_trm << TermAttr(TERM_ATTR_OFF);

					_trm << ' ';

					_trm << TermAttr(TERM_COLOR_FG_BLUE | TERM_BOLD);
					_trm << printfmt(_bwide ? "湿度=%02d%%" : "%02d%%", (amb.u16Humd + 50) / 100);
					_trm << TermAttr(TERM_ATTR_OFF);

					_trm << ' ';

					_trm << TermAttr(TERM_COLOR_FG_YELLOW | TERM_BOLD);
					_trm << printfmt(_bwide ? "照度=%4d" : "L%4d", amb.u32Lumi > 9999 ? 9999 : amb.u32Lumi);
					_trm << TermAttr(TERM_ATTR_OFF);
				} break;

				case E_PAL_PCB::MOT:
				{
					// generate pal board specific data structure.
					PalMot mot = pal.get_PalMot();

					_trm << TermAttr(TERM_COLOR_BG_BLACK | TERM_COLOR_FG_GREEN);
					_trm << printfmt("加速", pal.u8addr_src);
					_trm << TermAttr(TERM_ATTR_OFF);
					_trm << ":";

					if (mot.u8samples > 0) {
						_trm << printfmt(_bwide ? "X=%5d Y=%5d Z=%5d" : "%5d,%5d,%5d", mot.i16X[0], mot.i16Y[0], mot.i16Z[0]);
					}
					else {
						_trm << "n/a.";
					}
				} break;

				default:
					_trm << "n.a";
				}
			}

			_trm << "\033[4G"; // move cursor at column 2.
		}

		// update status line
		update_status();
	}
};

pkt_data_and_view pkt_data(the_screen, the_screen_t);

// put a byte to parse
void parse_a_byte(char_t u8b) {
	// parse.
	parse_ascii << u8b;

	// if complete parsing
	if (parse_ascii) {
		// output as parser format
		// the_screen_b << parse_ascii;
		the_screen_b.clear_screen();
		static int ct;
		the_screen_b << "PKT(" << ct++ << ')';

		// EEPROM TEST
		auto&& p = parse_ascii.get_payload();

		// 1. identify the packet type
		auto&& pkt = newTwePacket(parse_ascii.get_payload());
		the_screen_b << ":Typ=" << int(identify_packet_type(pkt));
		
		if (identify_packet_type(pkt) == E_PKT::PKT_PAL) {
			auto&& pal = refTwePacketPal(pkt);

			// put information
			the_screen_b
				<< printfmt(":Lq=%d:Ad=%08X", pal.u8lqi, pal.u32addr_src)
				<< ":PAL=" << int(pal.u8palpcb)
				<< ":ID=" << int(pal.u8addr_src)
				<< ":Dat=" << int(pal.u8sensors)
				;

			// store data into `pal_data'
			if (pkt_data.add_entry(pkt)) {
				// update screen.
				pkt_data.update_term(pkt, false);
			}
		}
	}
}

// process input
void process_input() {
	int c;
	
	// from TWE
	while(-1 != (c = TWEM5::the_input_uart.get_a_byte())) {
		// pass them to M5 (normal packet analysis)
		parse_a_byte(char_t(c));
	}
}

// check serial input.
void check_for_serial() {
	// UART0 : default UART port (M5stack console)
	while (Serial.available()) {
		int c = Serial.read();

		// put input byte into queue.
		the_input_uart.push_a_byte(c);
	}

	// UART2 : connected to TWE
	while (Serial2.available()) {
		int c = Serial2.read();

		// put input byte into queue.
		the_input_uart.push_a_byte(c);

		// Serial.write(c); // redirect to Serial (for debug)
	}
}

// the setup()
void setup() {
	// init lcd, serial, not init sd card
	M5.begin(true, false, true, false); // bool LCDEnable, bool SDEnable, bool SerialEnable,bool I2CEnable
	M5.Power.begin(); // power control enabled

	// this procedure should follow after M5.begin();
	Serial2.setRxBufferSize(512);
	Serial2.begin(115200, SERIAL_8N1, 16, 17);

	// init the TWE M5 support
	setup_screen(); // initialize TWE M5 support.

	// put a init message
	const char* fmt_title = "\033[1mTWELITE PAL Receiver\033[0m : %s";
	the_screen_t << printfmt(fmt_title, "---"); // accepts UTF-8 codes
	pkt_data.init_screen(fmt_title);

	// button navigation
	//              "0....+....1....+....2....+....3....+....4....+....5....+...."
	the_screen_c << "  前ﾍﾟｰｼﾞ/長押:電源切     ﾌｫﾝﾄ/色変更       次ﾍﾟｰｼﾞ/ﾃｽﾄdat";
}

// the main loop.
void loop() {
	M5.update();

	// check for serial input
	check_for_serial();

	// process input data
	process_input();

	// LCD update
	check_for_refresh();

	// button operation
	if (M5.BtnA.wasReleased()) {
		// previous page
		pkt_data.prev_page();
		pkt_data.update_term();
	}
	else if (M5.BtnB.wasReleased()) {
		// chenge font
		change_screen_font();

		pkt_data.reinit_screen();
		pkt_data.update_term();
	}
	else if (M5.BtnC.wasReleased()) {
		// next page
		pkt_data.next_page();
		pkt_data.update_term();
	}
	else if (M5.BtnA.wasReleasefor(700)) {
		// power off
		M5.powerOFF();
	}
	else if (M5.BtnB.wasReleasefor(700)) {
		// change colour
		change_screen_color();
		pkt_data.update_term();
	}
	else if (M5.BtnC.wasReleasefor(700)) {
		// test data
		const char* paldata[] = { 
			":800000008D0011810EE29A01808103113008020CE411300102048A00000001006163\r\n",
			":800000006F0010810EE29A01808103113008020CE411300102052C0000000101A59A\r\n",
			":80000000D50079810EE29A07808312113008020CE41130010203CC15040006FFF0FF2003F015040106FFE8FF28040015040206FFE0FF3003F815040306FFF0FF2003F015040406FFE8FF2803F015040506FFE0FF2803F815040606FFE8FF20040015040706FFE0FF20041015040806FFE8FF28040815040906FFF0FF28040015040A06FFF0FF4003F815040B06FFE8FF20040815040C06FFF8FF2003F015040D06FFE0FF28042015040E06FFE0FF2003F815040F06FFE8FF2004103D28\r\n" };
		const char* msgs[] =    {
			"ID=1 MAG OPEN",
			"ID=1 MAG CLOSE",
			"ID=7 MOT" };

		static int s_idx = 0;

		s_idx++;
		if (s_idx >= 3) s_idx = 0;

		pkt_data.set_page(1);

		const char* p = paldata[s_idx];
		while(*p != 0) {
			parse_a_byte(char_t(*p));
			p++;
		}

		pkt_data.update_term();

		the_screen_b.clear_screen();
		the_screen_b << "TEST DATA: " << msgs[s_idx];
	}
}