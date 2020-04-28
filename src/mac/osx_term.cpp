/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#ifdef __APPLE__
#include <iostream>
// #include <stdio.h>
#include <curses.h>

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_sercmd.hpp"
#include "twe_console.hpp"
#include "twe_printf.hpp"
#include "twe_csettings.hpp"

#include "twe_sys.hpp"

#include "osx_term.hpp"


using namespace TWETERM;

static bool _b_active_curses = false;

TWE::IStreamOut& TWE_PutChar_CONIO::operator ()(char_t c) {
#ifdef USE_CURSES
	if (_b_active_curses) addch(c);
#else
	putchar(c);
#endif
	return *this;
}

int TWE_GetChar_CONIO::get_a_byte() {
#ifdef USE_CURSES
	return (_b_active_curses) ? wgetch(stdscr) : -1;
#else
	return -1;
#endif
}

/* serial functions */
int TWETERM_iPutC(int c) {
#ifdef USE_CURSES
	return (_b_active_curses) ? waddch(stdscr, c) : -1;
#else
	putchar(c);
	return c;
#endif
}
int TWETERM_iGetC() {
#ifdef USE_CURSES
	return  (_b_active_curses) ? wgetch(stdscr) : -1;
#else
	return -1;
#endif
}

TWETerm_MacConsole::TWETerm_MacConsole(uint8_t u8c, uint8_t u8l) 
		: ITerm(u8c, u8l) {
}

TWETerm_MacConsole::~TWETerm_MacConsole() {
#ifdef USE_CURSES
	if (_b_active_curses) endwin();
#endif
}

void TWETerm_MacConsole::setup() {
#ifdef USE_CURSES
	initscr(); 
	noecho();
	nodelay(stdscr, TRUE);
	cbreak();
	raw(); // if raw mode, ctrl-c passes to the app.
	_b_active_curses = true;
#endif
}

void TWETerm_MacConsole::close_term() {
#ifdef USE_CURSES
	if (_b_active_curses) {
		endwin();
		_b_active_curses = false;
	}
#endif
}

void TWETerm_MacConsole::refresh() {
#ifdef USE_CURSES
	if (!visible()) {
		if (_b_active_curses) {
			// def_shell_mode(); // this call does not take back scroll-back buffer(???)
			endwin();
			_b_active_curses = false;

			std::cout << "\033[2J\033[H";
			std::cout.flush();
		}
		return;
	} else {
		if (!_b_active_curses) {
			// def_prog_mode();
			setup(); // reopen curses
		}
	}
	
	if (u32Dirty) {
		const int B = 256;
		char fmt[B];

		if (u32Dirty == U32DIRTY_FULL) {
			wclear(stdscr);
		}
		for (int i = 0; i <= max_line; i++) {
			if ((1UL << i) & u32Dirty) {
				wmove(stdscr, i, 0);

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

				waddstr(stdscr, fmt);
			}
		}

		int c_vis = column_idx_to_vis(cursor_c, calc_line_index(cursor_l));
		wmove(stdscr, cursor_l, c_vis);
	}
	wrefresh(stdscr);

	u32Dirty = 0UL;
#endif
}

#endif // __APPLE__