/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

#include <cstring>
#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_console.hpp"

using namespace TWE;
using namespace TWETERM;

const uint8_t TWETERM::U8CMDTBL[] = {
	'A', E_ESCSEQ_CURSOR_UP,
	'B', E_ESCSEQ_CURSOR_DOWN,
	'C', E_ESCSEQ_CURSOR_FWD,
	'D', E_ESCSEQ_CURSOR_BWD,
	'H', E_ESCSEQ_CURSOR_POSITION,
	'f', E_ESCSEQ_CURSOR_POSITION,
	'J', E_ESCSEQ_ERASE_DISPLAY,
	'K', E_ESCSEQ_ERASE_LINE,
	'G', E_ESCSEQ_CURSOR_POSITION_COLUMN,
	'm', E_ESCSEQ_CURSOR_ATTR,
	'h', E_ESCSEQ_SCREEN_MODE,
	0, 0 // terminator
};

/// <summary>
/// initialize the buffers
/// </summary>
void ITerm::_init_buff() {
	// init screen buff
	astr_screen.redim(max_line + 1);
	int off = 0;
	for (int i = 0; i <= max_line; i++) {
		astr_screen[i].attach(_raw_buffer + off, 0, max_col + 1);
		off += (max_col + 1);
	}
}

// add a byte to the terminal
ITerm& ITerm::write (wchar_t c) {
	int16_t i;
	bool bHandled = false;

	uint8_t u8cur_init = (uint8_t)cursor_l;

	// WRAP
	{ // no WRAP for single line
		i = calc_line_index(cursor_l);
		int c_vis = column_idx_to_vis(cursor_c, i);

		if (wrapchar >= 0 && is_printable(c)) { // && c_vis >= max_col) {
			// wrap at the right edge
			if (cursor_c >= max_col) cursor_c = max_col;
			astr_screen[i][cursor_c] = GChar(0xA6, 0x09); // RED COLOR

			if (max_line > 1) {
				// new line
				cursor_c = 0;
				cursor_l = cursor_l + 1;
				u32Dirty |= (1UL << cursor_l);
				if (cursor_l > max_line) {
					cursor_l = max_line;
					u32Dirty |= (1UL << cursor_l);
					newline();
				}

				// recalc line buff idx
				i = calc_line_index(cursor_l);

				// put a last byte on the line head
				astr_screen[i].append(GChar(wrapchar, escseq_attr));
				astr_screen[i].append(GChar(c, escseq_attr));
				cursor_c = 2;
			}

			// exit here
			u32Dirty = U32DIRTY_FULL;
			wrapchar = -1;
			return (*this);
		}
	}

	// clear wrapchar.
	wrapchar = -1;

	// ESC SEQUENCE
	if (escseq.is_sequence()) {
		// is in the sequence (already got ESC in advance)

		if (escseq << (uint8_t)c) {
			// complete the sequence

			uint8_t op;
			uint8_t opchr;
			uint8_t nVal;
			uint8_t* pVal;
			op = escseq.get_command(opchr, nVal, &pVal);
			uint8_t val1 = pVal[0];
			uint8_t val2 = pVal[1];
			int16_t col_v;

			switch (op) {
			case E_ESCSEQ_CURSOR_UP:
				col_v = column_idx_to_vis(cursor_c, calc_line_index(cursor_l));
				cursor_l -= val1 ? val1 : 1;
				if (cursor_l < 0) cursor_l = 0;
				u32Dirty |= (1UL << cursor_l);
				cursor_c = column_vis_to_idx(col_v, calc_line_index(cursor_l));
				u8OptRefresh |= U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK;
				break;

			case E_ESCSEQ_CURSOR_DOWN:
				col_v = column_idx_to_vis(cursor_c, calc_line_index(cursor_l));
				cursor_l += val1 ? val1 : 1;
				if (cursor_l > max_line) cursor_l = max_line;
				cursor_c = column_vis_to_idx(col_v, calc_line_index(cursor_l));
				u32Dirty |= (1UL << cursor_l);
				u8OptRefresh |= U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK;

				break;

			case E_ESCSEQ_CURSOR_FWD:
				cursor_c += val1 ? val1 : 1;
				if (cursor_c > max_col) cursor_c = max_col;
				u32Dirty |= (1UL << cursor_l);
				u8OptRefresh |= U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK;
				break;

			case E_ESCSEQ_CURSOR_BWD:
				cursor_c -= val1 ? val1 : 1;
				if (cursor_c < 0) cursor_c = 0;
				u32Dirty |= (1UL << cursor_l);
				u8OptRefresh |= U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK;
				break;

			case E_ESCSEQ_CURSOR_POSITION:
				cursor_l = val1 ? val1 - 1 : 0;
				col_v = val2 ? val2 - 1 : 0;
				if (cursor_l < 0) cursor_l = 0;
				if (col_v < 0) col_v = 0;

				if (cursor_l > max_line) cursor_l = max_line;
				if (col_v > max_col) col_v = max_col;
				cursor_c = column_vis_to_idx(col_v, calc_line_index(cursor_l));

				u32Dirty |= (1UL << cursor_l);
				u8OptRefresh |= U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK;

				break;

			case E_ESCSEQ_CURSOR_POSITION_COLUMN:
				col_v = val1 ? val1 - 1 : 0;
				if (col_v < 0) col_v = 0;
				if (col_v > max_col) col_v = max_col;
				cursor_c = column_vis_to_idx(col_v, calc_line_index(cursor_l));

				u32Dirty |= (1UL << cursor_l);
				u8OptRefresh |= U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK;

				break;

			case E_ESCSEQ_ERASE_DISPLAY:
				if (val1 == 2) {
					// ONLY IMPLEMENT 2
					home(); // DOS spec
					clear();
					force_refresh();
				}
				break;

			case E_ESCSEQ_ERASE_LINE:
			{
				int i = calc_line_index(cursor_l);
				u32Dirty |= (1UL << cursor_l);
				u8OptRefresh |= U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK;

				// fill with spaces
				switch (val1) {
				case 0: // clear from cursor to line end
					astr_screen[i][cursor_c] = GChar(' ', escseq_attr);
					astr_screen[i].redim(cursor_c + 1);
					break;
				case 1: // clear head to cursor
					for (int j = 0; j <= cursor_c; j++) astr_screen[i][j] = ' ';
					break;
				case 2:
					for (int j = 0; j <= cursor_c; j++) astr_screen[i][j] = ' ';
					astr_screen[i].redim(cursor_c + 1);
					break;
				}
			}

			case E_ESCSEQ_CURSOR_ATTR:
				// not supported, just set internally
				if (val1 == 0) { // NOTE: ESC[m may clear all attribulte as ESC[0m.
					escseq_attr = 0;
				}
				else {
					for (i = 0; i < nVal; i++) {
						if (pVal[i] == 1) {
							escseq_attr |= E_ESCSEQ_BOLD_MASK;
						}
						else  if (pVal[i] == 7) {
							escseq_attr |= E_ESCSEQ_REVERSE_MASK;
						}
						else if (pVal[i] >= 30 && pVal[i] <= 37) {
							escseq_attr = (escseq_attr & 0xFFF0) | (8 + (pVal[i] - 30)); // set fg color
						}
						else if (pVal[i] >= 40 && pVal[i] <= 47) {
							escseq_attr = (escseq_attr & 0xFF0F) | ((8 + (pVal[i] - 40)) << 4); // set bg color
						}
					}
				}
				break;

			case E_ESCSEQ_SCREEN_MODE:
				u8OptRefresh |= U8OPT_REFRESH_WITH_SCREEN_MODE;
				screen_mode = val1;
				force_refresh();
				break;

			case E_ESCSEQ_UNKNOWN_COMMAND:
				break;
			default:
				break;
			}

			escseq.s_init(); // init here
		}
		if (u32Dirty) {
			u32Dirty |= (1UL << u8cur_init);
		}
		bHandled = true;
	}

	// SPECIAL CHARS
	if (c == '\r') {
		// carrige return
		cursor_c = 0;
		u32Dirty |= (1UL) << cursor_l;
		bHandled = true;
	}
	else if (c == 0x08) {
		// BS (cursor back)
		cursor_c = cursor_c - 1;

		if (cursor_c < 0) {
			cursor_c = 0;
		}
		else {
			// ' astr_screen(i).Remove(astr_screen(i).Length - 1, 1) // remove tail char
		}
		u32Dirty |= (1UL) << cursor_l;
		bHandled = true;
	}
	else if (c == 0x1B) {
		// will handle esc sequence
		escseq.s_init();
		escseq << (uint8_t)c;
		bHandled = true;
	}
	else if (c == '\n') {
		cursor_c = 0;
		u32Dirty |= (1UL) << cursor_l;
		cursor_l = cursor_l + 1;
		u32Dirty |= (1UL) << cursor_l;

		if (cursor_l > max_line) {
			cursor_l = max_line;
			newline();
			u32Dirty = U32DIRTY_FULL;
		}

		bHandled = true;
	}

	// calc buffer index of the line.
	uint8_t L = calc_line_index(cursor_l);

	// append a char
	if (!bHandled && is_printable(c)) { // printable
		// fill with space to the cursor position, when the cursor exceeds line buffer end.
		while (astr_screen[L].length() <= cursor_c) {
			astr_screen[L].append(GChar(' ', 0));
		}
		
		int c_vis = column_idx_to_vis(cursor_c, L);
		bool bSingleChar = TWEUTILS::Unicode_isSingleWidth(c);

		if (bSingleChar || (!bSingleChar && c_vis + 1 <= max_col)) {
			if (cursor_c <= max_col) {
				// put a char at the cursor position
				astr_screen[L][cursor_c] = GChar(c, escseq_attr);
				cursor_c = cursor_c + 1;
				u32Dirty |= (1UL) << cursor_l;

				// on the right end of colomn.
				c_vis = column_idx_to_vis(cursor_c, L);

				if (c_vis > max_col) {
					wrapchar = c;
					cursor_c--;
				}
			}
			else {
				// exceptional state (set cursor to the end of buffer)
				cursor_c = max_col - 1;
			}
		}
		else {
			wrapchar = c;
		}
	}

	return (*this); // returns self
}

// output to stream
void ITerm::operator >> (IStreamOut& fo) {

	for (int l = 0; l <= max_line; l++) {
		int i = calc_line_index(l);

		for (int j = 0; j < astr_screen[i].length(); j++) {
			fo << (char_t)astr_screen[i][j];
		}

		if (l != max_line)
			fo << TWE::Endl;
	}
}

// resize screen (e.g. due to font change)
void ITerm::resize_screen(uint8_t u8c, uint8_t u8l) {
	// check the max
	if (u8c > max_term_col) {
		u8c = max_term_col;
	}

	// check _raw_buffer over (affects max lines)
	if (u8c * u8l > _raw_buffer_max) {
		u8l = _raw_buffer_max / u8c;
	}
	if (u8l > astr_screen.length_max()) {
		u8l = (uint8_t)astr_screen.length_max();
	}
	if (u8l > max_term_line) {
		u8l = max_term_line;
	}

	max_col = u8c - 1;
	max_line = u8l - 1;
	end_l = max_line;

	// init screen buff again
	_init_buff();

	// redraw!
	u32Dirty = U32DIRTY_FULL;
}

bool TWETERM::EscSeq::operator<<(uint8_t c) {
	bool bAgain = false;
	do {
		bAgain = false;

		switch (_s) {
		case E_ESCSEQ_VOID:
			if (c == '\033') {
				// waiting ESC
				_s = E_ESCSEQ_STATE_HAS_ESC;
				_s_save = E_ESCSEQ_VOID;

				_i = 1;
				_b[0] = c;
				
				_vals_n = 0;
				memset(_vals, 0, sizeof(_vals));
				_vals[0] = 0;
				_cmdchr = 0;
			}
			break;

		case E_ESCSEQ_STATE_HAS_ESC:
			// expect '['
			if (c == '[') {
				_s = E_ESCSEQ_STATE_HAS_BRACKET;
				_i = 2;
				_b[1] = c;
			}
			else {
				_s = E_ESCSEQ_VOID;
			}
			break;

		case E_ESCSEQ_STATE_HAS_BRACKET:
			if (c >= '0' && c <= '9') {
				start_value(c);
			}
			else if (c == ';') {
				// _s = E_ESCSEQ_STATE_HAS_SEMICOLON;
			}
			else if (c == '=') {
				_s = E_ESCSEQ_STATE_HAS_EQUAL;
			}
			else {
				// should command check
				_cmdchr = c;
				_s = _u8GetCmd(_cmdchr);
			}
			break;

		case E_ESCSEQ_STATE_HAS_EQUAL:
			if (c >= '0' && c <= '9') {
				start_value(c);
			}
			else if (c == ';') {
				// _s = E_ESCSEQ_STATE_HAS_SEMICOLON;
			}
			else if (_vals_n > 0 && c == 'h') {
				_cmdchr = c;
				_s = E_ESCSEQ_SCREEN_MODE;
			}
			else {
				_s = E_ESCSEQ_UNKNOWN_COMMAND;
			}
			break;

		case E_ESCSEQ_STATE_READNUM:
			if (c >= '0' && c <= '9') {
				int i =  _vals[_vals_n - 1] * 10 + (c - '0');
				if (i <= 255) {
					_vals[_vals_n - 1] = i;
				}
			}
			else {
				// finish read number, back to state
				bAgain = true;
				_s = _s_save;
				_s_save = E_ESCSEQ_VOID;
			}
			break;
		}
	} while (bAgain);

	return is_complete();
}