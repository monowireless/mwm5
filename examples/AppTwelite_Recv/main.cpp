/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <mwm5.h>
#include <M5Stack.h>

#include "screen.hpp"

/** @brief	The parse ASCII format */
AsciiParser parse_ascii(256);

/** @brief	store the last packet information. */
spTwePacket spLastPacket;

/**
 * @fn	void update_screen(bool b_redraw = false)
 *
 * @brief	Updates the screen described by b_redraw
 *
 * @param	b_redraw	(Optional) True to redraw whole text.
 */
void update_screen(bool b_redraw = false) {
	if (b_redraw) the_screen.clear_screen();
	ITerm& trm = the_screen; // set alias

	trm << "\033[H"; // set cursor home

	bool b = b_redraw;
	bool c = false;
	if (spLastPacket) c = true;

	if (b || c) {
		if (b) trm << "\033[1;1H" 
			"\033[1;30;45mﾀｲﾑCT\033[0m" "  "
			"\033[1;30;45mId#\033[0m" " "
			"\033[1;30;45mｼﾘｱﾙ番号\033[0m";
		if (b) trm << "\033[3;1H"
			"\033[1;30;45m D1 \033[0m"
			" \033[1;30;45m D2 \033[0m"
			" \033[1;30;45m D3 \033[0m"
			" \033[1;30;45m D4 \033[0m";
		if (b) trm << "\033[5;1H"
			"\033[1;30;45m A1 \033[0m"
			" \033[1;30;45m A2 \033[0m"
			" \033[1;30;45m A3 \033[0m"
			" \033[1;30;45m A4 \033[0m";
	}

	if (c) {
		TwePacketTwelite& x = refTwePacketTwelite(spLastPacket);
		if (c) trm << "\033[2;1H" << printfmt("%5d  %3d %8X", x.u16timestamp, x.u8addr_src, x.u32addr_src);
		if (c) trm << "\033[4;1H " 
			<< (x.DI1 ? "\033[31m" "●" : "\033[35m" "〇") << "\033[0m   "
			<< (x.DI2 ? "\033[32m" "●" : "\033[35m" "〇") << "\033[0m   "
			<< (x.DI3 ? "\033[33m" "●" : "\033[35m" "〇") << "\033[0m   "
			<< (x.DI4 ? "\033[34m" "●" : "\033[35m" "〇") << "\033[0m";
		//                            01234567890123456789
		if (c) {
			trm << "\033[6;1H";

			uint16_t adcvals[] = { x.u16Adc1, x.u16Adc2, x.u16Adc3, x.u16Adc4 };

			bool bfirst = true;
			for (uint16_t v : adcvals) {
				if (bfirst) bfirst = false;
				else trm << " ";

				if (v < 500) {
					trm << printfmt("\033[1;34m%4d\033[0m", v); // blue
				}
				else if (v < 1000) {
					trm << printfmt("\033[1;32m%4d\033[0m", v); // green
				}
				else if (v < 1500) {
					trm << printfmt("\033[1;33m%4d\033[0m", v); // yellow
				}
				else if (v < 5000) {
					trm << printfmt("\033[1;31m%4d\033[0m", v); // red
				}
				else {
					// no value
					trm << "----";
				}
			}
		}
	}
}

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

		if (identify_packet_type(pkt) == E_PKT::PKT_TWELITE) {
			auto&& x = refTwePacketTwelite(pkt);

			// put information
			the_screen_b
				<< printfmt(":Lq=%d:Ad=%08X", x.u8lqi, x.u32addr_src)
				<< printfmt(":ID=%02X", x.u8addr_src)
				<< printfmt(":DI=%04b", x.DI_mask)
				;

			spLastPacket = pkt;

			update_screen();
		}
	}
}

// process input
void process_input() {  
	int c;

	// from TWE
	while (-1 != (c = the_uart_queue.read())) {
		// pass them to M5 (normal packet analysis)
		parse_a_byte(char_t(c));
	}
}

// check serial input.
void check_for_serial() {
#ifndef ESP32
	while (the_sys_keyboard.available()) {
		int c = the_sys_keyboard.get_a_byte();
		if (c >= 0) {
			// sys console key input -> TWELITE via Serial2
			WrtTWE << char_t(c);
		}
	}
#endif

	// UART2 : connected to TWE
	while (Serial2.available()) {
		int c = Serial2.read();

		// put input byte into queue.
		the_uart_queue.push(c);

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

	// allocate buffer in the_uart_queue
	the_uart_queue.setup(512);

	// init the TWE M5 support
	setup_screen(); // initialize TWE M5 support.

	// put a init message
	the_screen_t << "\033[1m TWELITE\033[0m®\033[1m標準アプリ\033[0m (ｺﾏﾝﾄﾞ0x81)";

	// button navigation
	//e_screen_c << "....+....1a...+....2....+....3.b..+....4....+....5..c.+....6...."; // 10dots 64cols
	the_screen_c << "     --/長押:電源切        ﾌｫﾝﾄ/色変更            --/ﾃｽﾄdat";

	// init screen
	update_screen(true);
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
	}
	else if (M5.BtnB.wasReleased()) {
		// chenge font
		change_screen_font();
		update_screen(true);
	}
	else if (M5.BtnC.wasReleased()) {
		// next page
	}
	else if (M5.BtnA.wasReleasefor(700)) {
		// power off
		M5.powerOFF();
	}
	else if (M5.BtnB.wasReleasefor(700)) {
		// change colour
		change_screen_color();
		update_screen(true);
	}
	else if (M5.BtnC.wasReleasefor(700)) {
		// test data
		const char* pktdata[] = {
			":7881150175810000380026C9000C04220000FFFFFFFFFFA7\r\n",
			":788115017581000038002785000C05220000FFFFFFFFFFE9\r\n",
			":78811501758100003800284F000C02230000FFFFFFFFFF20\r\n" };
		const char* msgs[] = {
			"DATA1",
			"DATA2",
			"DATA3" };

		static int s_idx = 0;

		s_idx++;
		if (s_idx >= 3) s_idx = 0;

		const char* p = pktdata[s_idx];
		while (*p != 0) {
			parse_a_byte(char_t(*p));
			p++;
		}

		the_screen_b.clear_screen();
		the_screen_b << "TEST DATA: " << msgs[s_idx];
	}
}