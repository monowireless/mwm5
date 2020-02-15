/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_font.hpp"
#include "arduino_lcd_font_MP12.h"

#define FONT_MP12_DATA_ROWS 12
#define FONT_MP12_ROWS 13
#define FONT_MP12_COLS 6
#define FONT_MP12_DBL_CHARS 2645


namespace TWEFONT {
	extern const uint8_t font_mplus_f12r[128 * FONT_MP12_DATA_ROWS];
	extern const uint8_t font_mplus_f12r_jisx201[64 * FONT_MP12_DATA_ROWS];
	extern const uint8_t font_mplus_f12r_latin1ex[96 * FONT_MP12_DATA_ROWS];
	extern const uint16_t font_mplus_f12j_idx[FONT_MP12_DBL_CHARS];
	extern const uint8_t font_mplus_f12j_data[FONT_MP12_DBL_CHARS * FONT_MP12_DATA_ROWS *2];
	extern const uint8_t font_mplus_f12j_unsupported[FONT_MP12_DATA_ROWS * 2];

	/// <summary>
	/// create M+ BITMAP FONT 12 (Joyo Kanji)
	/// </summary>
	/// <param name="id">font id, used for display changing</param>
	/// <param name="line_space">add line spacing by pixel</param>
	/// <param name="char_space">add char to char spacing by pixel</param>
	/// <returns></returns>
	const FontDef& createFontMP12(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "M+ BITMAP FONTS 12dot";

			font->width = FONT_MP12_COLS;
			font->height = FONT_MP12_ROWS;

			font->data_cols = FONT_MP12_COLS;
			font->data_rows = FONT_MP12_DATA_ROWS;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_mplus_f12r;
			font->font_jisx201 = font_mplus_f12r_jisx201;
			font->font_latin1_ex = font_mplus_f12r_latin1ex;

			font->font_wide = font_mplus_f12j_data;		// WIDE FONT DATA 
			font->font_wide_missing = font_mplus_f12j_unsupported;
			font->font_wide_idx = font_mplus_f12j_idx;	// UNICODE index 
			font->font_wide_count = FONT_MP12_DBL_CHARS;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}
}

// include the font table here (TODO: separate .cpp file would have _unreferenced link error)
#include "arduino_lcd_font_MP12_table.hpp"
