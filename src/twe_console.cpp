/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

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
	// clear raw buff
	for (int i = 0; i < _raw_buffer_max; i++) {
		_raw_buffer[i].attr() = 0;
		_raw_buffer[i].chr() = 0;
	}

	// init screen buff
	astr_screen.redim(max_line + 1);
	int off = 0;
	for (int i = 0; i <= max_line; i++) {
		astr_screen[i].attach(_raw_buffer + off, 0, max_col + 1);
		off += (max_col + 1);
	}

	escseq_attr = 0;
	escseq_attr_default = 0;
}

// add a byte to the terminal
ITerm& ITerm::write (wchar_t c) {
	int16_t i;
	bool bHandled = false;

	uint8_t u8cur_init = (uint8_t)cursor_l;

	// WRAP
	if (wrap_mode) { // no WRAP for single line
		i = calc_line_index(cursor_l);
		int c_vis = column_idx_to_vis(cursor_c, i);

		if (wrapchar >= 0 && is_printable(c)) { // && c_vis >= max_col) {
			// wrap at the right edge
			if (cursor_c >= max_col) cursor_c = max_col;
			astr_screen[i][cursor_c] = GChar(L'_', 0x09); // 0xab(<<) GChar(' ', 0x00); // GChar('|', 0x09); // RED COLOR

			if (max_line > 1) {
				// new line
				cursor_c = 0;
				cursor_l = cursor_l + 1;
				dirtyLine.set_dirty(cursor_l);
				if (cursor_l > max_line) {
					cursor_l = max_line;
					dirtyLine.set_dirty(cursor_l);
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
			dirtyLine.set_dirty_full(); // anyway setting full... necessary???
			wrapchar = -1;
			return (*this);
		}

		// clear wrapchar.
		wrapchar = -1;
	}
	else {
		if (wrapchar >= 0 && is_printable(c)) {
			return (*this);
		}
		else {
			wrapchar = -1;
		}
	}

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
				dirtyLine.set_dirty(cursor_l);
				cursor_c = column_vis_to_idx(col_v, calc_line_index(cursor_l));
				u8OptRefresh |= U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK;
				break;

			case E_ESCSEQ_CURSOR_DOWN:
				col_v = column_idx_to_vis(cursor_c, calc_line_index(cursor_l));
				cursor_l += val1 ? val1 : 1;
				if (cursor_l > max_line) cursor_l = max_line;
				cursor_c = column_vis_to_idx(col_v, calc_line_index(cursor_l));
				dirtyLine.set_dirty(cursor_l);
				u8OptRefresh |= U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK;

				break;

			case E_ESCSEQ_CURSOR_FWD:
				cursor_c += val1 ? val1 : 1;
				if (cursor_c > max_col) cursor_c = max_col;
				dirtyLine.set_dirty(cursor_l);
				u8OptRefresh |= U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK;
				break;

			case E_ESCSEQ_CURSOR_BWD:
				cursor_c -= val1 ? val1 : 1;
				if (cursor_c < 0) cursor_c = 0;
				dirtyLine.set_dirty(cursor_l);
				u8OptRefresh |= U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK;
				break;

			case E_ESCSEQ_CURSOR_POSITION:
				move_cursor(val2 ? val2 - 1 : 0, val1 ? val1 - 1 : 0);
#if 0
				cursor_l = val1 ? val1 - 1 : 0;
				col_v = val2 ? val2 - 1 : 0;
				if (cursor_l < 0) cursor_l = 0;
				if (col_v < 0) col_v = 0;

				if (cursor_l > max_line) cursor_l = max_line;
				if (col_v > max_col) col_v = max_col;
				cursor_c = column_vis_to_idx(col_v, calc_line_index(cursor_l));

				dirtyLine.set_dirty(cursor_l);
				u8OptRefresh |= U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK;
#endif
				break;

			case E_ESCSEQ_CURSOR_POSITION_COLUMN:
				col_v = val1 ? val1 - 1 : 0;
				if (col_v < 0) col_v = 0;
				if (col_v > max_col) col_v = max_col;
				cursor_c = column_vis_to_idx(col_v, calc_line_index(cursor_l));

				dirtyLine.set_dirty(cursor_l);
				u8OptRefresh |= U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK;

				break;

			case E_ESCSEQ_ERASE_DISPLAY:
				if (val1 == 2) {
					// ONLY IMPLEMENT 2
					home(); // DOS spec
					clear();
					//force_refresh();
				}
				break;

			case E_ESCSEQ_ERASE_LINE:
			{
				int i = calc_line_index(cursor_l);
				dirtyLine.set_dirty(cursor_l);
				u8OptRefresh |= U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK;

				// fill with spaces
				switch (val1) {
				case 0: // clear from cursor to line end
					astr_screen[i].redim(max_col + 1);
					for(int j = cursor_c; j <= max_col; j++) {
						astr_screen[i][j] = GChar(' ', escseq_attr);
					}
					#if 0
					astr_screen[i][cursor_c] = GChar(' ', escseq_attr);
					astr_screen[i].redim(cursor_c + 1);
					#endif
					break;
				case 1: // clear head to cursor
					for (int j = 0; j <= cursor_c; j++) astr_screen[i][j] = GChar(' ', escseq_attr);
					break;
				case 2:
					for (int j = 0; j <= cursor_c; j++) astr_screen[i][j] = GChar(' ', escseq_attr);
					astr_screen[i].redim(cursor_c + 1);
					break;
				}
			} break;
			
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
						else if (pVal[i] == 4) {
							escseq_attr |= E_ESCSEQ_UNDERLINE_MASK;
						}
						else if (pVal[i] == 7) {
							escseq_attr |= E_ESCSEQ_REVERSE_MASK;
						}
						else if (pVal[i] >= 30 && pVal[i] <= 37) {
							escseq_attr = (escseq_attr & 0xFFF0) | (8 + (pVal[i] - 30)); // set fg color
						}
						else if (pVal[i] >= 40 && pVal[i] <= 47) {
							escseq_attr = (escseq_attr & 0xFF0F) | ((8 + (pVal[i] - 40)) << 4); // set bg color
						}
						else if (pVal[i] == 101) { // not standard, disable bold attr
							escseq_attr &= ~E_ESCSEQ_BOLD_MASK;
						}
						else if (pVal[i] == 104) { // not standard, disable underline attr
							escseq_attr &= ~E_ESCSEQ_UNDERLINE_MASK;
						}
						else if (pVal[i] == 107) { // not standard, disable reverse attr
							escseq_attr &= ~E_ESCSEQ_REVERSE_MASK;
						}
						else if (pVal[i] == 130) { // not standard, set fg as default
							escseq_attr &= 0xFFF0;
						}
						else if (pVal[i] == 140) { // not standard, set bg as default
							escseq_attr &= 0xFF0F;
						}
					}
				}
				break;

			case E_ESCSEQ_SCREEN_MODE:
				screen_mode = val1;
				force_refresh(U8OPT_REFRESH_WITH_SCREEN_MODE);
				break;

			case E_ESCSEQ_UNKNOWN_COMMAND:
				break;
			default:
				break;
			}

			escseq.s_init(); // init here
		}
		if (dirtyLine) {
			dirtyLine.set_dirty(u8cur_init);
		}
		bHandled = true;
	}

	// SPECIAL CHARS
	if (c == '\r') {
		// carrige return
		cursor_c = 0;
		dirtyLine.set_dirty(cursor_l);
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
		dirtyLine.set_dirty(cursor_l);
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
		dirtyLine.set_dirty(cursor_l);
		cursor_l = cursor_l + 1;
		dirtyLine.set_dirty(cursor_l);

		if (cursor_l > max_line) {
			cursor_l = max_line;
			newline();
			dirtyLine.set_dirty_full();
		}

		bHandled = true;
	}

	// calc buffer index of the line.
	uint8_t L = calc_line_index(cursor_l);

	// append a char
	if (!bHandled && is_printable(c)) { // printable
		// fill with space to the cursor position, when the cursor exceeds line buffer end.
		while (astr_screen[L].length() <= unsigned(cursor_c)) {
			if (!astr_screen[L].append(GChar(' ', 0))) {
				break;
			}
		}
		
		int c_vis = column_idx_to_vis(cursor_c, L);
		bool bSingleChar = TWEUTILS::Unicode_isSingleWidth(c);

		if (bSingleChar || (!bSingleChar && c_vis + 1 <= max_col)) {
			if (cursor_c <= max_col) {
				auto c0 = astr_screen[L][cursor_c];
				bool bSingleChar0 = TWEUTILS::Unicode_isSingleWidth(wchar_t(c0.chr()));

				// overwrite buffer
				if (bSingleChar != bSingleChar0) {
					if (bSingleChar) {
						// wide char is present at original, overwrite single char -> so insert dummy space at the next
						int i = astr_screen[L].length() - 1; // last index
						astr_screen[L].append(astr_screen[L][i]); // append last
						
						if (cursor_c < i - 1) { // skip when cursor_c is at buffer end.
							while (i >= cursor_c + 2) {
								astr_screen[L][i] = astr_screen[L][i - 1];
								--i;
							}

							// add dummy blank
							astr_screen[L][cursor_c + 1] = GChar(' ', 0);
						}
					} else {
						// single char is present at original, overwrite wide char -> erase the next char
						if (cursor_c < int16_t(astr_screen[L].length() - 1)) {
							for (i = cursor_c + 1; i <= int16_t(astr_screen[L].length() - 2); i++) {
								astr_screen[L][i] = astr_screen[L][i + 1]; 
							}
							astr_screen[L].pop_back();
						}
					}
				}

				// put a char at the cursor position
				astr_screen[L][cursor_c] = GChar(c, escseq_attr);
				cursor_c = cursor_c + 1;
				dirtyLine.set_dirty(cursor_l);

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

	for (unsigned l = 0; l <= max_line; l++) {
		unsigned i = calc_line_index(l);

		for (unsigned j = 0; j < astr_screen[i].length(); j++) {
			// fo << (char_t)astr_screen[i][j];
			fo.write_w(astr_screen[i][j]);
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
	dirtyLine.set_dirty_full();
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


// need to be reserve enough buffer size
void ITerm::get_screen_buf(GChar* ptr) {
	GChar nul; // null char

	for (uint8_t l = 0; l <= max_line; l++) {
		uint8_t L = calc_line_index(l); // get the index of buffer at line 'l'

		uint8_t cols = astr_screen[L].length();
		for (uint8_t c = 0; c <= max_col; c++) {
			*ptr = (c < cols) ? astr_screen[L][c] : nul;
			++ptr;
		}
	}
}

void ITerm::set_screen_buf(GChar* ptr, uint8_t oldcols, uint8_t oldlines) {
	clear(); // clear the buffer to the initial

	GChar nul; // null char

	for (uint8_t l = 0; l <= max_line; l++) {
		if (l < oldlines) {
			GChar* p = ptr + l * oldcols;

			for (uint8_t c = 0; c <= max_col; c++) {
				if (c < oldcols) {
					if (*p != nul) {
						astr_screen[l].push_back(*p);
					}
					++p;
				}
			}
		}
	}
}


// move cursor
ITerm& ITerm::move_cursor(uint8_t cols, uint8_t lines) {
	cursor_l = lines;
	int16_t col_v = cols;

	if (cursor_l < 0) cursor_l = 0;
	if (col_v < 0) col_v = 0;

	if (cursor_l > max_line) cursor_l = max_line;
	if (col_v > max_col) col_v = max_col;
	cursor_c = column_vis_to_idx(col_v, calc_line_index(cursor_l));

	dirtyLine.set_dirty(cursor_l);
	u8OptRefresh |= U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK;

	return *this;
}
