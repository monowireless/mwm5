/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_font.hpp"
#include "lcd_font_8x6.h"

namespace TWEFONT {
	extern const uint8_t font_8x6[128 * 7];

	/// <summary>
	/// 8x6 LCD font.
	/// </summary>
	/// <returns></returns>
	const FontDef& createFontLcd8x6(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_name = "Lcd 8x6";
			font->font_code = 1;
			font->width = 6;
			font->height = 8;

			font->w_space = char_space;
			font->h_space = line_space;

			font->data_cols = 6;
			font->data_rows = 7;

			font->font_latin1 = font_8x6;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}
}

// include the font table here (TODO: separate .cpp file would have _unreferenced link error)
#include "lcd_font_8x6_table.src"
