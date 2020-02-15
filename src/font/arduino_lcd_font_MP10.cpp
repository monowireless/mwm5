/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_font.hpp"
#include "arduino_lcd_font_MP10.h"

#define FONT_MP10_DATA_ROWS 10
#define FONT_MP10_ROWS 10
#define FONT_MP10_COLS 5
#define FONT_MP10_DBL_CHARS 2645

namespace TWEFONT {
	extern const uint8_t font_mplus_f10r[128 * FONT_MP10_DATA_ROWS];
	extern const uint8_t font_mplus_f10r_jisx201[64 * FONT_MP10_DATA_ROWS];
	extern const uint8_t font_mplus_f10r_latin1ex[96 * FONT_MP10_DATA_ROWS];
	extern const uint16_t font_mplus_f10j_idx[FONT_MP10_DBL_CHARS];
	extern const uint8_t font_mplus_f10j_data[FONT_MP10_DBL_CHARS * FONT_MP10_DATA_ROWS * 2];
	extern const uint8_t font_mplus_f10j_unsupported[FONT_MP10_DATA_ROWS * 2];

	/// <summary>
	/// create M+ BITMAP FONTS 10dot (Joyo Kanji)
	/// </summary>
	/// <param name="id">font id, used for display changing</param>
	/// <param name="line_space">add line spacing by pixel</param>
	/// <param name="char_space">add char to char spacing by pixel</param>
	/// <returns></returns>
	const FontDef& createFontMP10(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "M+ BITMAP FONTS 10dot";

			font->width = FONT_MP10_COLS;
			font->height = FONT_MP10_ROWS;

			font->data_cols = FONT_MP10_COLS;
			font->data_rows = FONT_MP10_DATA_ROWS;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_mplus_f10r;
			font->font_jisx201 = font_mplus_f10r_jisx201;
			font->font_latin1_ex = font_mplus_f10r_latin1ex;

			font->font_wide = font_mplus_f10j_data;		// WIDE FONT DATA 
			font->font_wide_missing = font_mplus_f10j_unsupported;
			font->font_wide_idx = font_mplus_f10j_idx;	// UNICODE index 
			font->font_wide_count = FONT_MP10_DBL_CHARS;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}

}

// include the font table here (TODO: separate .cpp file would have _unreferenced link error)
#include "arduino_lcd_font_MP10_table.hpp"

