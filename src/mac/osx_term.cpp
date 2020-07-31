/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#ifdef __APPLE__
#include <iostream>
// #include <stdio.h>
#ifdef USE_CURSES
#include <curses.h>
#else
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_sercmd.hpp"
#include "twe_console.hpp"
#include "twe_printf.hpp"
#include "twe_csettings.hpp"

#include "twe_sys.hpp"

#include "osx_term.hpp"


using namespace TWETERM;

#ifdef USE_CURSES
static bool _b_active_curses = false;
#else
static bool _b_term_controlled = false;
static struct termios CookedTermIos; // cooked mode
static struct termios RawTermIos; // raw mode
#endif

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
	return _b_term_controlled ? getchar() : -1;
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
	return _b_term_controlled ? getchar() : -1;
#endif
}

TWETerm_MacConsole::TWETerm_MacConsole(uint8_t u8c, uint8_t u8l) 
		: ITerm(u8c, u8l), _builtin_term(false) {
#ifdef USE_CURSES
#else
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
#endif
}

TWETerm_MacConsole::~TWETerm_MacConsole() {
#ifdef USE_CURSES
	if (_b_active_curses) endwin();
#else
	close_term();
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
#else
	// set back the terminal
	if (_b_term_controlled) {
        tcsetattr(STDIN_FILENO, 0, &CookedTermIos);
		_b_term_controlled = false;
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

			printf("\033[2J\033[H");
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
#else
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
#endif
}

#endif // __APPLE__