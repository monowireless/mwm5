/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#ifdef __linux
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_sercmd.hpp"
#include "twe_console.hpp"
#include "twe_printf.hpp"
#include "twe_csettings.hpp"

#include "linux_term.hpp"

using namespace TWETERM;

bool TWETerm_LinuxConsole::_b_term_controlled = false;
static struct termios CookedTermIos; // cooked mode
static struct termios RawTermIos; // raw mode

TWE::IStreamOut& TWE_PutChar_CONIO::operator ()(char_t c) {
	putchar(c);
	return *this;
}

int TWE_GetChar_CONIO::get_a_byte() {
	return (TWETerm_LinuxConsole::is_term_controlled()) ? getchar() : -1;
}

/* serial functions */
int TWETERM_iPutC(int c) {
	putchar(c);
	return c;
}

int TWETERM_iGetC() {
	return (TWETerm_LinuxConsole::is_term_controlled()) ? getchar() : -1;
}

TWETerm_LinuxConsole::TWETerm_LinuxConsole(uint8_t u8c, uint8_t u8l) : ITerm(u8c, u8l) {
	if (!_b_term_controlled) {
		// save the intial state of terminal
		tcgetattr(STDIN_FILENO, &CookedTermIos);

		// create RAW mode terminal
		RawTermIos = CookedTermIos;
		cfmakeraw(&RawTermIos);
		RawTermIos.c_oflag |= OPOST;

		// set stdin as RAW mode 
		tcsetattr(STDIN_FILENO, TCSANOW, &RawTermIos);
		
		// for nonblocking getchar()
		fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

		_b_term_controlled = true;
	}
}

TWETerm_LinuxConsole::~TWETerm_LinuxConsole() {
	close_term();
}

void TWETerm_LinuxConsole::setup() {
	return;
}

void TWETerm_LinuxConsole::close_term() {
	// set back the terminal
	if (_b_term_controlled) {
        tcsetattr(STDIN_FILENO, 0, &CookedTermIos);
		_b_term_controlled = false;
	}
}

void TWETerm_LinuxConsole::refresh() {
	if (!visible()) {
		return;
	}
	
	if (u32Dirty) {
		const int B = 1024;
		char fmt[B];

		if (u32Dirty == U32DIRTY_FULL) {
			fputs("\033[2J\033[H", stdout);
		}
		for (int i = 0; i <= max_line; i++) {
			if ((1UL << i) & u32Dirty) {
				snprintf(fmt, sizeof(fmt), "\033[%d;%dH", i + 1, 1); // move cursor
				fputs(fmt, stdout);
				
				int j = calc_line_index(i);
				memset(fmt, 0x20, sizeof(fmt));

				GChar* p = astr_screen[j].begin().raw_ptr();
				GChar* e = astr_screen[j].end().raw_ptr();
				int ifmt = 0;

				if (e - p >= sizeof(fmt)) {
					e = p + sizeof(fmt) - 1; // reserve 1 byte for NUL terminator
				}
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

				fputs(fmt, stdout);
			}
		}

		int c_vis = column_idx_to_vis(cursor_c, calc_line_index(cursor_l));
		snprintf(fmt, sizeof(fmt), "\033[%d;%dH", cursor_l + 1, c_vis + 1); // move cursor
		fputs(fmt, stdout);
	}
	fflush(stdout);

	u32Dirty = 0UL;
}

void TWETERM_vInitVSCON(TWE_tsFILE* fp, TWE::IStreamOut *winconsole, TWE::IStreamIn *winkeyb) {
	C_TWE_printf_support::s_init(fp, winconsole, winkeyb);
}

#endif // _linux