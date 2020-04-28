/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

#if defined(_MSC_VER) || defined(__MINGW32__)

#include <stdio.h>
#include <conio.h>
#include <windows.h>

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_sercmd.hpp"
#include "twe_console.hpp"
#include "twe_printf.hpp"
#include "twe_csettings.hpp"

#include "msc_term.hpp"

using namespace TWETERM;

/* serial functions */
int TWETERM_iPutC(int c) { return _putch(c); }
int TWETERM_iGetC() { if (_kbhit()) return _getch(); else return -1; }

void TWETerm_WinConsole::refresh() {
	if (!visible()) {
		setup(); // always set ESC SEQ MODE.

		if (_prog_mode) {
			_prog_mode = false;
			_cputs("\033[2J\033[H"); // clear all screen and HOME
		}
		return;
	}
	else {
		if (!_prog_mode) {
			setup();
			_prog_mode = true;
			_cputs("\033[2J\033[H"); // clear all screen and HOME
		}
	}

	if (u32Dirty) {
		const int B = 256;
		char fmt[B];

		if (u32Dirty == U32DIRTY_FULL) {
			_cputs("\033[2J\033[H"); // clear all screen and HOME
		}
		for (int i = 0; i <= max_line; i++) {
			if ((1UL << i) & u32Dirty) {
				sprintf_s(fmt, "\033[%d;%dH", i + 1, 1); // move cursor
				_cputs(fmt);

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

				_cputs(fmt);
			}
		}

		//sprintf_s(fmt, "\033[%d;%dH123456789 223456789 323456789 423456789 523456789 623456789 723456789 823456789X", max_line + 2, 1); // move cursor
		//cputs(fmt);

		int c_vis = column_idx_to_vis(cursor_c, calc_line_index(cursor_l));
		sprintf_s(fmt, "\033[%d;%dH", cursor_l + 1, c_vis + 1); // move cursor
		_cputs(fmt);
	}
	u32Dirty = 0UL;
}

/// <summary>
/// reinit_state win console
/// </summary>
void TWETERM_vInitVSCON(TWE_tsFILE* fp, TWE::IStreamOut *winconsole, TWE::IStreamIn *winkeyb) {
	C_TWE_printf_support::s_init(fp, winconsole, winkeyb);
}


#endif // _MSC_VER