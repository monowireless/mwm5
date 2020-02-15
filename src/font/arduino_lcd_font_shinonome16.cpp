/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_font.hpp"
#include "arduino_lcd_font_shinonome16.h"

#define FONT_SHINO16_DATA_ROWS 16
#define FONT_SHINO16_ROWS 16
#define FONT_SHINO16_COLS 8
#define FONT_SHINO16_DBL_CHARS 2645
#define FONT_SHINO16_DBL_CHARS_FULL 6868



namespace TWEFONT {
	extern const uint8_t font_shinonome_f16r[128 * FONT_SHINO16_DATA_ROWS];
	extern const uint8_t font_shinonome_f16r_jisx201[64 * FONT_SHINO16_DATA_ROWS];
	extern const uint8_t font_shinonome_f16r_latin1ex[96 * FONT_SHINO16_DATA_ROWS];
	extern const uint16_t font_shinonome_f16j_idx[FONT_SHINO16_DBL_CHARS];
	extern const uint8_t font_shinonome_f16j_data[FONT_SHINO16_DBL_CHARS * FONT_SHINO16_DATA_ROWS * 2];
	extern const uint8_t font_shinonome_f16j_unsupported[FONT_SHINO16_DATA_ROWS * 2];

	extern const uint16_t font_shinonome_f16j_idx_full[FONT_SHINO16_DBL_CHARS_FULL];
	extern const uint8_t font_shinonome_f16j_data_full[FONT_SHINO16_DBL_CHARS_FULL * FONT_SHINO16_DATA_ROWS * 2];

	/// <summary>
	/// create Shinonome 16dot font (Joyo Kanji)
	/// </summary>
	/// <param name="id">font id, used for display changing</param>
	/// <param name="line_space">add line spacing by pixel</param>
	/// <param name="char_space">add char to char spacing by pixel</param>
	/// <returns></returns>
	const FontDef& createFontShinonome16(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "Shinonome 16dot";

			font->width = FONT_SHINO16_COLS;
			font->height = FONT_SHINO16_ROWS;

			font->data_cols = FONT_SHINO16_COLS;
			font->data_rows = FONT_SHINO16_DATA_ROWS;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_shinonome_f16r;
			font->font_jisx201 = font_shinonome_f16r_jisx201;
			font->font_latin1_ex = font_shinonome_f16r_latin1ex;

			font->font_wide = font_shinonome_f16j_data;		// WIDE FONT DATA 
			font->font_wide_missing = font_shinonome_f16j_unsupported;
			font->font_wide_idx = font_shinonome_f16j_idx;	// UNICODE index 
			font->font_wide_count = FONT_SHINO16_DBL_CHARS;

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
	const FontDef& createFontShinonome16_full(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "Shinonome 16dot";

			font->width = FONT_SHINO16_COLS;
			font->height = FONT_SHINO16_ROWS;

			font->data_cols = FONT_SHINO16_COLS;
			font->data_rows = FONT_SHINO16_DATA_ROWS;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_shinonome_f16r;
			font->font_jisx201 = font_shinonome_f16r_jisx201;
			font->font_latin1_ex = font_shinonome_f16r_latin1ex;

			font->font_wide = font_shinonome_f16j_data_full;		// WIDE FONT DATA 
			font->font_wide_missing = font_shinonome_f16j_unsupported;
			font->font_wide_idx = font_shinonome_f16j_idx_full;	// UNICODE index 
			font->font_wide_count = FONT_SHINO16_DBL_CHARS_FULL;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}
}

// include the font table here (TODO: separate .cpp file would have _unreferenced link error)
#include "arduino_lcd_font_shinonome16_table.hpp"
