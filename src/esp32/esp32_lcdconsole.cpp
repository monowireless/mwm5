/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#undef DEBUG_SER // DEBUGMSG

#include <string.h>

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_printf.hpp"
#include "twe_sys.hpp"

#include "generic_lcd_common.h"
#include "generic_lcd_screen.hpp"

#include "esp32_common.h"
#include "esp32_lcdconsole.hpp"
#include "twe_font.hpp"

using namespace TWE;
using namespace TWEARD;

const uint16_t TWEARD::COLTBL_DEFAULT[8] = { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, ALMOST_WHITE };

// implement refresh function
void TWEARD::TWETerm_M5_Console::refresh() {
	// if not visible, don't draw here!
	if (!visible()) return;

	// screen mode change
	if (u8OptRefresh & U8OPT_REFRESH_WITH_SCREEN_MODE) {
		// screen mode change
		home();
		clear();
		set_font(screen_mode);

		u32Dirty = U32DIRTY_FULL;
		u8OptRefresh = U8OPT_REFRESH_HARDWARE_CLEAR_MASK;
	}

	// redraw whole screen
	if (u32Dirty == U32DIRTY_FULL) {
		if (escseq_attr & 0x80) {
			_bg = COL_TBL[(escseq_attr >> 4) & 0x07];
		}
		else {
			_bg = _bg_def;
		}
		if (escseq_attr & 0x08) {
			_fg = COL_TBL[(escseq_attr >> 4) & 0x07];
		}
		else {
			_fg = _fg_def;
		}
		if (u8OptRefresh & U8OPT_REFRESH_HARDWARE_CLEAR_MASK) {
			// clear screen
			_M5.Lcd.fillRect(_rect.x, _rect.y, _rect_view.w, _rect_view.h, _bg_def);
		}
	}

	// cursor blink
	uint16_t u16now = millis() & 0xffff;
	bool bUpdateCursor = false;
	if (cursor_mode == 0x01) {
		_u8cursor = 1;
	} else
	if (cursor_mode == 0x02 && ((u16now - _u16timer_blink) & 0xffff) > 500) {
		_u8cursor = !_u8cursor; // cursor on or off
		bUpdateCursor = true; // need update
		_u16timer_blink = u16now; // timer count update
	}

	// update the line specified u32Dirty bitmask (bit0 -> line 0, bit1 -> line 1, ...)
	// save previous cursor pos
	if (u32Dirty || bUpdateCursor) { // dirty flag or cursor update

		auto font = TWEFONT::queryFont(font_id());

		//Serial.printf("R(%x)", u32Dirty); // DEBUG
		for (int i = 0; i <= max_line; i++) {
			if (((1UL << i) & u32Dirty) || (bUpdateCursor && i == cursor_l)) { // update only necessary line
				int16_t x, y;

				// find the buffer index at line 'i'.
				int L = calc_line_index(i);

#ifdef DEBUG_SER
				if (max_line <= 2) {
					Serial.printf("\nLINE(%02d/%d): ", i, astr_screen[L].length());
					for (int k = 0; k < astr_screen[L].length(); k++) {
						uint16_t c = astr_screen[L][k];
						if (c < 0x80) {
							Serial.printf("%c", c);
						}
						else {
							Serial.printf("<%04x>", c);
						}
					}
				}
#endif

				// length of the line
				unsigned len = astr_screen[L].length();
				unsigned j_start = 0;
				unsigned j_end = len + 1;

				if (u32Dirty == U32DIRTY_FULL || u8OptRefresh & U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK) {
					j_end = max_col + 1; // clear whole columns of the target line.
				}
				if (u32Dirty == 0 && bUpdateCursor) {
					// only draw cursor
					j_start = cursor_c;
					j_end = cursor_c + 1;
				}

				// loop for column
				for (unsigned j = j_start; j < j_end + 1; j++) { // for drawing/erasing cursor, draw one byte more.
					uint16_t c[2];
					TWETERM::GChar::tAttr attr;

					if (j < astr_screen[L].length()) {
						c[0] = astr_screen[L][j].chr();
						attr = astr_screen[L][j].attr();
					}
					else {
						c[0] = ' '; // blank
						attr = escseq_attr_default; // default attribute is applied (effective, if color attribute applied when screen cleared.)
					}
					c[1] = 0;
					// calc the position
					uint16_t c_vis = column_idx_to_vis(j, L);
					if ((c_vis + (TWEUTILS::Unicode_isSingleWidth(c[0]) ? 0 : 1)) > max_col) break; // check boundary 

					get_cursor_pos(x, y, uint8_t(c_vis), uint8_t(i));

					// if actual position excess the boundary.
					if (x < _rect.x || x + (TWEUTILS::Unicode_isSingleWidth(c[0]) ? 1 : 2) * font.get_width() > _rect.x + _rect_view.w) continue;
					if (y < _rect.y || y + font.get_height() > _rect.y + _rect_view.h) continue;
					
					// calc the color
					uint16_t fg = _fg; // default fg
					uint16_t bg = _bg; // default bg

					if (attr & 0x08) { // fg is set colored
						fg = COL_TBL[attr & 0x07];
					}
					if (attr & 0x80) {
						bg = COL_TBL[(attr >> 4) & 0x07];
					}

					if (attr & TWETERM::E_ESCSEQ_REVERSE_MASK) {
						// swap fg/bg
						uint16_t t = fg;
						fg = bg;
						bg = t;
					}

					bool bCursor = false;
					if (cursor_mode && i == cursor_l && j == cursor_c && _u8cursor) {
						bCursor = true;
						// draw cursor
						//fg = BLACK;
						//bg = RED;
#ifdef DEBUG_SER
						if (max_line > 10) {
							Serial.printf("."); // DEBUG
							Serial.printf("(%d,%d,%2X:%04x)", i, j, attr, c[0]); // DEBUG
						}
#endif
					}
					else if (i == _cursor_l_last && j == _cursor_c_last) {		
						// redraw with attr of last cursor position.
						// render as normal

						//Serial.printf("(%d,%d,%2X:%c)", i, j, attr, c[0]); // DEBUG

						// NOTE: everytime, j_end is set properly to reach here, otherwise, the previous cursor remains...
					}

					if (font.is_default()) {
						// render with default font
#if defined (ESP32)
						if (c[0] < 0x100) _M5.Lcd.drawChar(x, y, c[0], fg, bg, 1);
#endif
					}
					else {
						drawChar(font, x, y, c[0], 
						      fg
							, bg 
							, ((attr & TWETERM::E_ESCSEQ_BOLD_MASK) ? 1 : 0) 
								| ((attr & TWETERM::E_ESCSEQ_UNDERLINE_MASK) ? 2 : 0)
								| (bCursor ? 0x80 : 0)
							, _M5
						);
					}					
				}
			}
		}

		_cursor_l_last = uint8_t(cursor_l);
		_cursor_c_last = uint8_t(cursor_c);
	}

	post_refresh(); // clear flags, etc.
}

void TWEARD::TWETerm_M5_Console::set_font(uint8_t font_id, uint8_t u8col_request, uint8_t u8row_request) {
	const TWEFONT::FontDef& font = TWEFONT::queryFont(font_id);
	_font_height = font.get_height();
	_font_width = font.get_width();
	_font_id = font_id;

	if (_font_width == 0 || _font_height == 0) {
		_font_height = 8;
		_font_width = 6;
	}
	
	// set new columns/lines
	int col = _rect.w / _font_width;
	if (u8col_request && col >= u8col_request) col = u8col_request; // if request size is given.

	int lin = _rect.h / _font_height;
	if (lin == 0) lin = 1; // at least reserve one line (the text will draw at out of region)
	if (u8row_request && lin >= u8row_request) lin = u8row_request; // if request size is given.
	
	// resize internal buffer.
	resize_screen((uint8_t)col, (uint8_t)lin);

	// calc offset of start addr
	col = max_col + 1;
	lin = max_line + 1;
	
	_off_x = _rect.w - (col * _font_width) + 1; _off_x /= 2;
	_off_y = _rect.h - (lin * _font_height) + 1; _off_y /= 2;
}