/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_font.hpp"
#include "arduino_lcd_font_shinonome14.h"

#define FONT_SHINO14_DATA_ROWS 14
#define FONT_SHINO14_ROWS 14
#define FONT_SHINO14_COLS 7
#define FONT_SHINO14_DBL_CHARS 2645
#define FONT_SHINO14_DBL_CHARS_FULL 6868


namespace TWEFONT {
	extern const uint8_t font_shinonome_f14r[128 * FONT_SHINO14_DATA_ROWS];
	extern const uint8_t font_shinonome_f14r_jisx201[64 * FONT_SHINO14_DATA_ROWS];
	extern const uint8_t font_shinonome_f14r_latin1ex[96 * FONT_SHINO14_DATA_ROWS];
	extern const uint16_t font_shinonome_f14j_idx[FONT_SHINO14_DBL_CHARS];
	extern const uint8_t font_shinonome_f14j_data[FONT_SHINO14_DBL_CHARS * FONT_SHINO14_DATA_ROWS * 2];
	extern const uint8_t font_shinonome_f14j_unsupported[FONT_SHINO14_DATA_ROWS * 2];

	extern const uint16_t font_shinonome_f14j_idx_full[FONT_SHINO14_DBL_CHARS_FULL];
	extern const uint8_t font_shinonome_f14j_data_full[FONT_SHINO14_DBL_CHARS_FULL * FONT_SHINO14_DATA_ROWS * 2];

	/// <summary>
	/// create Shinonome 14dot font (Joyo Kanji)
	/// </summary>
	/// <param name="id">font id, used for display changing</param>
	/// <param name="line_space">add line spacing by pixel</param>
	/// <param name="char_space">add char to char spacing by pixel</param>
	/// <returns></returns>
	const FontDef& createFontShinonome14(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "Shinonome 14dot";

			font->width = FONT_SHINO14_COLS;
			font->height = FONT_SHINO14_ROWS;

			font->data_cols = FONT_SHINO14_COLS;
			font->data_rows = FONT_SHINO14_DATA_ROWS;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_shinonome_f14r;
			font->font_jisx201 = font_shinonome_f14r_jisx201;
			font->font_latin1_ex = font_shinonome_f14r_latin1ex;

			font->font_wide = font_shinonome_f14j_data;		// WIDE FONT DATA 
			font->font_wide_missing = font_shinonome_f14j_unsupported;
			font->font_wide_idx = font_shinonome_f14j_idx;	// UNICODE index 
			font->font_wide_count = FONT_SHINO14_DBL_CHARS;

			font->opt = opt;


			return *font;
		}
		else {
			return queryFont(0);
		}

	}

	/// <summary>
	/// create Shinonome 14dot font (Kanji fullset)
	/// </summary>
	/// <param name="id">font id, used for display changing</param>
	/// <param name="line_space">add line spacing by pixel</param>
	/// <param name="char_space">add char to char spacing by pixel</param>
	/// <returns></returns>
	const FontDef& createFontShinonome14_full(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "Shinonome 14dot";

			font->width = FONT_SHINO14_COLS;
			font->height = FONT_SHINO14_ROWS;

			font->data_cols = FONT_SHINO14_COLS;
			font->data_rows = FONT_SHINO14_DATA_ROWS;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_shinonome_f14r;
			font->font_jisx201 = font_shinonome_f14r_jisx201;
			font->font_latin1_ex = font_shinonome_f14r_latin1ex;

			font->font_wide = font_shinonome_f14j_data_full;		// WIDE FONT DATA 
			font->font_wide_missing = font_shinonome_f14j_unsupported;
			font->font_wide_idx = font_shinonome_f14j_idx_full;	// UNICODE index 
			font->font_wide_count = FONT_SHINO14_DBL_CHARS_FULL;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}
}

// include the font table here (TODO: separate .cpp file would have _unreferenced link error)
#include "arduino_lcd_font_shinonome14_table.hpp"
