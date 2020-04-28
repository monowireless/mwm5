/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_console.hpp"
#include "twe_printf.hpp"

#ifdef ESP32
// #include "twe_sys.hpp"
#include "esp32_common.h"

using namespace TWE;
using namespace TWETERM;

// version variables
extern const unsigned char MWM5_APP_VERSION_MAIN  __attribute__((weak));
extern const unsigned char MWM5_APP_VERSION_SUB  __attribute__((weak));
extern const unsigned char MWM5_APP_VERSION_VAR  __attribute__((weak));
extern const char* MWM5_APP_NAME  __attribute__((weak));

const unsigned char MWM5_APP_VERSION_MAIN = 0;
const unsigned char MWM5_APP_VERSION_SUB = 0;
const unsigned char MWM5_APP_VERSION_VAR = 0;
const char* MWM5_APP_NAME = "MWM5";

// do nothing so far (for printf.h)
extern "C" void _putchar(char character) {
	Serial.write(character);
}

// the instance
TWE::TWE_PutChar_Serial<HardwareSerial> TWE::WrtCon(Serial);
TWE::IStreamOutWrapper TWE::WrtTWE(new TWE::TWE_PutChar_Serial<HardwareSerial>(Serial2)); // default WrtTWE object is Serial2
// TWE::TWE_PutChar_Serial<HardwareSerial> TWE::WrtTWE(Serial2);

class TWETerm_EspConsole : public TWETERM::ITerm {
public:
	TWETerm_EspConsole() : ITerm(80, 24) {}
	void setup() {}
	void refresh();
};

/* serial functions */
void TWETerm_EspConsole::refresh() {
	if (!visible()) return;

	if (u32Dirty) {
		const int B = 256;
		char fmt[B];

		if (u32Dirty == U32DIRTY_FULL) {
			WrtCon << "\033[2J\033[H"; // clear all screen and HOME
		}
		for (int i = 0; i <= max_line; i++) {
			if ((1UL << i) & u32Dirty) {
				WrtCon << printfmt("\033[%d;%dH", i + 1, 1);

				int j = calc_line_index(i);
				memset(fmt, 0x20, sizeof(fmt));

				GChar* p = astr_screen[j].begin().raw_ptr();
				GChar* e = astr_screen[j].end().raw_ptr();
				int ifmt = 0;
				while (p < e) {
					if (*p < 0x80) {
						fmt[ifmt++] = (char)*p;
					}
					else {
						fmt[ifmt++] = (*p & 0xFF00) >> 8;
						fmt[ifmt++] = *p & 0xFF;
					}
					p++;
				}
				fmt[ifmt] = 0;

				WrtCon << fmt;
			}
		}

		//sprintf_s(fmt, "\033[%d;%dH123456789 223456789 323456789 423456789 523456789 623456789 723456789 823456789X", max_line + 2, 1); // move cursor
		//cputs(fmt);

		int c_vis = column_idx_to_vis(cursor_c, calc_line_index(cursor_l));
		WrtCon << printfmt("\033[%d;%dH", cursor_l + 1, c_vis + 1); // move cursor
	}
	u32Dirty = 0UL;
}

// global instance
TWETerm_EspConsole con_screen;
ITerm& TWETERM::the_sys_console = con_screen;


#endif