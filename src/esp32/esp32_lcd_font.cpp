/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_utils.hpp"

#if defined(ESP32)
//#include <M5Stack.h>
#endif

#include "generic_lcd_common.h"
#include "generic_lcd_screen.hpp"

#include "esp32_lcd_color.h"
#include "twe_font.hpp"

using namespace TWEARD;

#undef DEBUGSER

static inline int32_t get_color_force(int n_rows
		, bool cursor, int cursor_rows, uint16_t curosor_color
		, int underline, int underline_rows, uint16_t underline_color)
{
	int32_t col_force = -1;
	if (cursor && n_rows > cursor_rows) { col_force = curosor_color;	}
	else if (underline && n_rows > underline_rows) { col_force = underline_color; }

	return col_force;
}

/// <summary>
/// draw a char in LCD (M5stack)
/// 
/// Note: setWindow() does not allow bigger area (maybe > 256bytes).
///       it should be set row by row when drawing a bigger char.
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name="c">drawing character (UNICODE)</param>
/// <param name="color"></param>
/// <param name="bg"></param>
/// <param name="opt">
///   MASK 0x01 : render bold style.
/// </param>
/// <returns></returns>
int16_t TWEARD::drawChar(const TWEFONT::FontDef& font, int32_t x, int32_t y, uint16_t c, uint32_t color, uint32_t bg, uint8_t opt, M5Stack& _M5) {
	// check if default font. 
	if (font.is_default()) {
		// (maybe this code is unexpected)

#if defined(ESP32)
		if (TWEUTILS::Unicode_isSingleWidth(c) && c < 0x80) {
			M5.Lcd.drawChar(x, y, c, color, bg, 1);
		}
		else {
			M5.Lcd.drawChar(x, y, '#', bg, color, 1); // write something
		}

		return font.get_width();
#elif defined (_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)
		return 0;
#endif
	}

	// check bold option
	bool bold = ((opt & 0x01) == 0x01); // if true, draw with bold style
	bool underline = ((opt & 0x02) == 0x02); // if true draw underline
	bool cursor = ((opt & 0x80) == 0x80); // draw cursor
	int cursor_rows = font.height / 2 - 1; // cursor start row
	int underline_rows = font.height - 2; // underline start row 

#ifdef DEBUGSER
	Serial.printf("(%d:%04x", font.get_font_code(), c);
#endif

	if (TWEUTILS::Unicode_isSingleWidth(c)) {
		// render single width chars

		// find font data (only assuming 8bit width)
		const uint8_t* p = &font.font_latin1[0];
		if (c < 0x80) p = &font.font_latin1[font.data_rows * c];
		else if (c < 0xa0) p = p; // out of range
		else if (c < 0x100) p = font.font_latin1_ex ? &font.font_latin1_ex[font.data_rows * (c - 0xA0)] : p; // latin1 ex
		else if (c < 0xFF60) p = p; // out of range
		else if (c < 0xFFA0) p = font.font_jisx201 ? &font.font_jisx201[font.data_rows * (c - 0xFF60)] : p; // hankaku kana
		else p = p; // out of range


		if (!(font.opt & TWEFONT::U32_OPT_FONT_TATEBAI || font.opt & TWEFONT::U32_OPT_FONT_YOKOBAI)) {
			// do render
			startWrite(_M5);

			// DO NOT USE setWindow() for whole character, but set to one row.
			// M5.Lcd.setWindow(x, y, x + font.width + font.w_space - 1, y + font.height + font.h_space); // y is actual font height, x is font width -1

			int i = 0;
			int w = font.width + font.w_space;
			int h = font.height + font.h_space;

			int s_l, s_r, s_t, s_b;
			s_l = font.w_space / 2; s_r = font.w_space - s_l;
			s_t = font.h_space / 2; s_b = font.h_space - s_t;
				
			uint16_t col;

			// blank line (bottom), if drawing additional line gap.
			for (; i < s_t; i++) {
				int32_t col_force = get_color_force(i, cursor, cursor_rows, RED, underline, underline_rows, color);
				col = col_force == -1 ? bg : col_force;

				setWindow(x, y + i, x + w - 1, y + i + 1, _M5); // y is actual font height, x is font width -1
				for (int j = 0; j < w; j++) {
					windowWrite16(col, _M5);
				}
			}

			for (int i2 = i - s_t; i2 < font.data_rows && i2 < font.height; i++, i2++) {
				setWindow(x, y + i, x + w - 1, y + i + 1, _M5); // y is actual font height, x is font width -1
					
				int32_t col_force = get_color_force(i2, cursor, cursor_rows, RED, underline, underline_rows, color);
					
				uint8_t pat = p[i2];
				if (bold) pat = pat | (pat >> 1);

				int j = 0;

				col = col_force == -1 ? bg : col_force;
				for (; j < s_l; j++) {
					windowWrite16(col, _M5);
				}

				for (; j < s_l + font.width; j++, pat <<= 1) {
					uint8_t bit = (pat & 0x80);
						
					col = (col_force == -1) ? (bit ? color : bg) : (bit ? BLACK : col_force);
					windowWrite16(col, _M5);
				}

				col = col_force == -1 ? bg : col_force;
				for (; j < w; j++) {
					windowWrite16(col, _M5);
				}
			}

			// blank line (bottom), if drawing additional line gap.
			for (; i < h; i++) {
				int32_t col_force = get_color_force(i, cursor, cursor_rows, RED, underline, underline_rows, color);
				col = col_force == -1 ? bg : col_force;

				setWindow(x, y + i, x + w - 1, y + i + 1, _M5); // y is actual font height, x is font width -1
				for (int j = 0; j < w; j++) {
					windowWrite16(col, _M5);
				}
			}

			endWrite(_M5);
		}
		else {
			// Double size.
			bool yokobai = (font.opt & TWEFONT::U32_OPT_FONT_YOKOBAI);
			bool tatebai = (font.opt & TWEFONT::U32_OPT_FONT_TATEBAI);

			int w = 0;
			if (yokobai) w = (font.width + font.w_space) * 2;
			else w = (font.width + font.w_space);

			int h = 0;
			if (tatebai) h = font.height * 2;
			else h = font.height;

			// do render
			startWrite(_M5);
			int i = 0;
			int nLine = 0;
			uint16_t col1, col2;

			int s_l, s_r, s_t, s_b;
			s_l = font.w_space / 2; s_r = font.w_space - s_l;
			s_t = font.h_space / 2; s_b = font.h_space - s_t;

			// blank line (top)
			col1 = cursor ? RED : bg;
			col2 = col1;
			for (; i < s_t; i++) {
				setWindow(x, y + nLine, x + w - 1, y + nLine + 1, _M5); // y is actual font height, x is font width -1
				for (int j = 0; j < font.width + font.w_space; j++) {
					windowWrite16(col1, _M5);
					if (yokobai) {
						windowWrite16(col2, _M5);
					}
				}
				nLine++;
			}

			for (int i2 = 0; i2 < font.height; i++, i2++) {
				int tate_ct = tatebai ? 2 : 1;
				while (tate_ct--) {
					setWindow(x, y + nLine, x + w - 1, y + nLine + 1, _M5); // y is actual font height, x is font width -1

					uint8_t pat = (i2 < font.data_rows) ? p[i2] : 0;
					uint16_t last_bit = 0;

					int j = 0;

					// left margin
					if (cursor && i > cursor_rows) {
						col1 = cursor ? RED : bg;
						col2 = col1;
					}
					else {
						col1 = bg;
						col2 = col1;
					}
					for (; j < s_l; j++) {
						windowWrite16(col1, _M5);
						if (yokobai) {
							windowWrite16(col2, _M5);
						}
					}

					// render char.
					for (int j2 = 0; j2 < font.width; j++, j2++, pat <<= 1) {
						uint8_t bit = (pat & 0x80);

						if (cursor && i > cursor_rows) {
							col2 = bit ? BLACK : RED;
							col1 = (bold && last_bit ? BLACK : col2);
						}
						else {
							col2 = bit ? color : bg;
							col1 = (bold && last_bit ? color : col2);
						}

						windowWrite16(col1, _M5);
						if (yokobai) {
							windowWrite16(col2, _M5);
						}
						last_bit = bit;
					}

					// right margin
					if (cursor && i > cursor_rows) {
						col1 = cursor ? RED : bg;
						col2 = col1;
					}
					else {
						col1 = bg;
						col2 = col1;
					}

					for (; j < (font.width + font.w_space); j++) {
						windowWrite16(col1, _M5);
						if (yokobai) {
							windowWrite16(col2, _M5);
						}
					}
					nLine++;
				}
			}

			// blank line (bottom)
			col1 = cursor ? RED : bg;
			col2 = col1;
			for (; i < font.height + font.h_space; i++) {
				setWindow(x, y + nLine, x + w - 1, y + nLine + 1, _M5); // y is actual font height, x is font width -1
				for (int j = 0; j < font.width + font.w_space; j++) {
					windowWrite16(col1, _M5);
					if (yokobai) {
						windowWrite16(col2, _M5);
					}
				}
				nLine++;
			}
			endWrite(_M5);
		}
			
#ifdef DEBUGSER
		Serial.printf(")", c);
#endif

		return font.get_width();
	}
	else {
		// check wide char font data presence.
		if (font.font_wide == nullptr) return 0; // nurupo check

		// find font data (only assuming 16bit width)
		const uint8_t* p = font.font_wide_missing;
		int idx = font.find_font_index(c); // find index to unicode bitmap data by charcode.
		if (idx >= 0) p = &font.font_wide[idx * font.data_rows * 2]; // find from the table

#ifdef DEBUGSER
		Serial.printf("->%d)", idx);
#endif
		if (!(font.opt & TWEFONT::U32_OPT_FONT_TATEBAI || font.opt & TWEFONT::U32_OPT_FONT_YOKOBAI)) {
			// do render
			startWrite(_M5);
			setWindow(x, y, x + ((font.width + font.w_space) * 2 - 1), y + font.height + font.h_space, _M5); // y is actual font height, x is font width -1
			int i = 0;
			uint16_t col;

			int w = (font.width + font.w_space) * 2;

			int s_l, s_r, s_t, s_b;
			s_l = font.w_space / 2; s_r = font.w_space * 2 - s_l;
			s_t = font.h_space / 2; s_b = font.h_space - s_t;

			// blank line (top)
			for (; i < s_t; i++) {
				int32_t col_force = get_color_force(i, cursor, cursor_rows, RED, underline, underline_rows, color);
				col = col_force == -1 ? bg : col_force;

				for (int j = 0; j < w; j++) {
					windowWrite16(col, _M5);
				}
			}

			// string
			for (int i2 = i - s_t;  i < s_t + font.height; i++, i2++) {
				uint16_t pat = i2 < font.data_rows ? ((p[i2*2] << 8) | p[i2*2+1]) : 0; // if you need >16dot font, you need edit here to expand bit width.
				if (bold) pat = pat | (pat >> 1);

				int32_t col_force = get_color_force(i2, cursor, cursor_rows, RED, underline, underline_rows, color);
					
				col = col_force == -1 ? bg : col_force;
				for (int j = 0; j < s_l; j++) {
					windowWrite16(col, _M5);
				}

				for (int j = 0; j < font.width * 2; j++, pat <<= 1) {
					uint16_t bit = (pat & 0x8000);

					col = col_force == -1 ? (bit ? color : bg) : (bit ? BLACK : col_force);
					windowWrite16(col, _M5);
				}

				col = col_force == -1 ? bg : col_force;
				for (int j = 0; j < s_r; j++) {
					windowWrite16(col, _M5);
				}
			}
				
			// blank line (bottom)
			for (; i < font.height + font.h_space; i++) {
				int32_t col_force = get_color_force(i, cursor, cursor_rows, RED, underline, underline_rows, color);
				col = col_force == -1 ? bg : col_force;
					
				for (int j = 0; j < w; j++) {
					windowWrite16(col, _M5);
				}
			}
			endWrite(_M5);
		}
		else {
			bool yokobai = (font.opt & TWEFONT::U32_OPT_FONT_YOKOBAI);
			bool tatebai = (font.opt & TWEFONT::U32_OPT_FONT_TATEBAI);
				
			int w = 0;
			if (yokobai) w = (font.width + font.w_space) * 4;
			else w = (font.width + font.w_space) * 2;

			int h = 0;
			if (tatebai) h = font.height * 2;
			else h = font.height;

			// do render
			startWrite(_M5);
			int i = 0;
			int nLine = 0;

			int s_l, s_r, s_t, s_b;
			s_l = font.w_space / 2; s_r = font.w_space * 2 - s_l;
			s_t = font.h_space / 2; s_b = font.h_space - s_t;

			uint16_t col1, col2;

			// blank line (top)
			for (; i < s_t; i++) {
				setWindow(x, y + nLine, x + w - 1, y + nLine + 1, _M5); // y is actual font height, x is font width -1
				col1 = cursor ? RED : bg;
				col2 = col1;
				for (int j = 0; j < (font.width + font.w_space) * 2; j++) {
					windowWrite16(col1, _M5);
					if (yokobai) {
						windowWrite16(col2, _M5);
					}
				}
				nLine++;
			}

			// char
			for (int i2 = 0; i2 < font.height; i++, i2++) {
				int tate_ct = tatebai ? 2 : 1;
				while (tate_ct--) {
					setWindow(x, y + nLine, x + w - 1, y + nLine + 1, _M5); // y is actual font height, x is font width -1

					uint16_t pat = (i2 < font.data_rows) ? ((p[i2 * 2] << 8) | p[i2 * 2 + 1]) : 0;
					uint16_t last_bit = 0;

					int j = 0;

					if (cursor && i > cursor_rows) {
						col1 = cursor ? RED : bg;
						col2 = col1;
					}
					else {
						col1 = bg;
						col2 = col1;
					}
					for (; j < s_l; j++) {
						windowWrite16(col1, _M5);
						if (yokobai) {
							windowWrite16(col2, _M5);
						}
					}

					for (int j2 = 0; j2 < font.width * 2; j++, j2++, pat <<= 1) {
						uint16_t bit = (pat & 0x8000);

						if (cursor && i > cursor_rows) {
							col2 = bit ? BLACK : RED;
							col1 = (bold && last_bit ? BLACK : col2);
						}
						else {
							col2 = bit ? color : bg;
							col1 = (bold && last_bit ? color : col2);
						}

						windowWrite16(col1, _M5);
						if (yokobai) {
							windowWrite16(col2, _M5);
						}
						last_bit = bit;
					}

					if (cursor && i > cursor_rows) {
						col1 = cursor ? RED : bg;
						col2 = col1;
					}
					else {
						col1 = bg;
						col2 = col1;
					}
					for (; j < (font.width + font.w_space) * 2; j++) {
						windowWrite16(col1, _M5);
						if (yokobai) {
							windowWrite16(col2, _M5);
						}
					}
					nLine++;
				}
			}
			// blank line (bottom)
			for (; i < font.height + font.h_space; i++) {
				setWindow(x, y + nLine, x + w - 1, y + nLine + 1, _M5); // y is actual font height, x is font width -1
				col1 = cursor ? RED : bg;
				col2 = col1;
				for (int j = 0; j < (font.width + font.w_space) * 2; j++) {
					windowWrite16(col1, _M5);
					if (yokobai) {
						windowWrite16(col2, _M5);
					}
				}
				nLine++;
			}
			endWrite(_M5);
		}

		return font.get_width() * 2;
	}
}

#if 0
int16_t TWEARD::drawChar(const TWEFONT::FontDef& font, int32_t x, int32_t y, uint16_t* str, uint32_t color, uint32_t bg, uint8_t opt, M5Stack& _M5) {
	TWEUTILS::Unicode_UTF8Converter utf8conv;
	int32_t x_now = x;

	// utf-8 converter
	for (uint16_t* p = str; *p != 0; p++) {
		uint8_t font_w = font.get_width(*p);

		drawChar(font, x_now, y, *p, color, bg, opt, _M5);
		x_now += font_w;
	}

	return (int16_t)(x_now - x);
}
#endif

int16_t TWEARD::drawChar(const TWEFONT::FontDef& font, int32_t x, int32_t y, const wchar_t* str, uint32_t color, uint32_t bg, uint8_t opt, M5Stack& _M5) {
	TWEUTILS::Unicode_UTF8Converter utf8conv;
	int32_t x_now = x;

	// utf-8 converter
	for (const wchar_t* p = str; *p != 0; p++) {
		uint8_t font_w = font.get_width(*p);

		drawChar(font, x_now, y, *p, color, bg, opt, _M5);
		x_now += font_w;
	}

	return (int16_t)(x_now - x);
}

/// <summary>
/// draw multi chars
/// </summary>
/// <param name="font"></param>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name="str"></param>
/// <param name="color"></param>
/// <param name="bg"></param>
/// <param name="opt"></param>
/// <returns>witdh of drawing string.</returns>
int16_t TWEARD::drawChar(const TWEFONT::FontDef& font, int32_t x, int32_t y, const char* str, uint32_t color, uint32_t bg, uint8_t opt, M5Stack& _M5) {
	TWEUTILS::Unicode_UTF8Converter utf8conv;
	int32_t x_now = x;

	// utf-8 converter
	for (uint8_t* p = (uint8_t*)str; *p != 0; p++) {
		uint16_t wc = utf8conv(*p);
		if (wc) {
			uint8_t font_w = font.get_width(wc);

			drawChar(font, x_now, y, wc, color, bg, opt, _M5);
			x_now += font_w;
		}
	}

	return (int16_t)(x_now - x);
}
