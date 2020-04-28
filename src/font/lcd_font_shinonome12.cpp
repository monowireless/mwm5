/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_font.hpp"
#include "lcd_font_shinonome12.h"

#define FONT_SHINO12_DATA_ROWS 12
#define FONT_SHINO12_ROWS 12
#define FONT_SHINO12_COLS 6
#define FONT_SHINO12_DBL_CHARS 2645
#define FONT_SHINO12_DBL_CHARS_FULL 6868


namespace TWEFONT {
	extern const uint8_t font_shinonome_f12r[128 * FONT_SHINO12_DATA_ROWS];
	extern const uint8_t font_shinonome_f12r_jisx201[64 * FONT_SHINO12_DATA_ROWS];
	extern const uint8_t font_shinonome_f12r_latin1ex[96 * FONT_SHINO12_DATA_ROWS];
	extern const uint16_t font_shinonome_f12j_idx[FONT_SHINO12_DBL_CHARS];
	extern const uint8_t font_shinonome_f12j_data[FONT_SHINO12_DBL_CHARS * FONT_SHINO12_DATA_ROWS * 2];
	extern const uint8_t font_shinonome_f12j_unsupported[FONT_SHINO12_DATA_ROWS * 2];

	extern const uint16_t font_shinonome_f12j_idx_full[FONT_SHINO12_DBL_CHARS_FULL];
	extern const uint8_t font_shinonome_f12j_data_full[FONT_SHINO12_DBL_CHARS_FULL * FONT_SHINO12_DATA_ROWS * 2];

	/// <summary>
	/// create Shinonome 12dot font (Joyo Kanji)
	/// </summary>
	/// <param name="id">font id, used for display changing</param>
	/// <param name="line_space">add line spacing by pixel</param>
	/// <param name="char_space">add char to char spacing by pixel</param>
	/// <returns></returns>
	const FontDef& createFontShinonome12(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "Shinonome 12dot";

			font->width = FONT_SHINO12_COLS;
			font->height = FONT_SHINO12_ROWS;

			font->data_cols = FONT_SHINO12_COLS;
			font->data_rows = FONT_SHINO12_DATA_ROWS;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_shinonome_f12r;
			font->font_jisx201 = font_shinonome_f12r_jisx201;
			font->font_latin1_ex = font_shinonome_f12r_latin1ex;

			font->font_wide = font_shinonome_f12j_data;		// WIDE FONT DATA 
			font->font_wide_missing = font_shinonome_f12j_unsupported;
			font->font_wide_idx = font_shinonome_f12j_idx;	// UNICODE index 
			font->font_wide_count = FONT_SHINO12_DBL_CHARS;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}

	/// <summary>
	/// create Shinonome 16dot font (Full Kanji)
	/// </summary>
	/// <param name="id">font id, used for display changing</param>
	/// <param name="line_space">add line spacing by pixel</param>
	/// <param name="char_space">add char to char spacing by pixel</param>
	/// <returns></returns>
	const FontDef& createFontShinonome12_full(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "Shinonome 12dot";

			font->width = FONT_SHINO12_COLS;
			font->height = FONT_SHINO12_ROWS;

			font->data_cols = FONT_SHINO12_COLS;
			font->data_rows = FONT_SHINO12_DATA_ROWS;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_shinonome_f12r;
			font->font_jisx201 = font_shinonome_f12r_jisx201;
			font->font_latin1_ex = font_shinonome_f12r_latin1ex;

			font->font_wide = font_shinonome_f12j_data_full;		// WIDE FONT DATA 
			font->font_wide_missing = font_shinonome_f12j_unsupported;
			font->font_wide_idx = font_shinonome_f12j_idx_full;	// UNICODE index 
			font->font_wide_count = FONT_SHINO12_DBL_CHARS_FULL;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}
}

// include the font table here (TODO: separate .cpp file would have _unreferenced link error)
#include "lcd_font_shinonome12_table.src"
