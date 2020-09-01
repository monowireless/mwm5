/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */
#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_font.hpp"
#include "lcd_font_MP10.h"

namespace TWEFONT {
	extern const uint8_t font_MP10k_unsupported[10*2];
	extern const uint8_t font_MP10r[1280];
	extern const uint8_t font_MP10r_latin1ex[960];
	extern const uint8_t font_MP10r_jisx201[640];


	/**********************************************************
	 * createFontMP10_mini [chrs = 577]
	 **********************************************************/
	extern const uint16_t font_MP10k_mini_idx[577];
	extern const uint8_t font_MP10k_mini_data[577*10*2];

	const FontDef& createFontMP10_mini(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "M+ BITMAP FONTS 10dot (MP10_mini, 577)";

			font->width = 5;
			font->height = 10;

			font->data_cols = 5;
			font->data_rows = 10;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_MP10r;
			font->font_jisx201 = font_MP10r_jisx201;
			font->font_latin1_ex = font_MP10r_latin1ex;

			font->font_wide = font_MP10k_mini_data;		// WIDE FONT DATA 
			font->font_wide_idx = font_MP10k_mini_idx;	// UNICODE index 
			font->font_wide_missing = font_MP10k_unsupported;

			font->font_wide_count = 577;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}


	/**********************************************************
	 * createFontMP10_std [chrs = 2646]
	 **********************************************************/
	extern const uint16_t font_MP10k_std_idx[2646];
	extern const uint8_t font_MP10k_std_data[2646*10*2];

	const FontDef& createFontMP10_std(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "M+ BITMAP FONTS 10dot (MP10_std, 2646)";

			font->width = 5;
			font->height = 10;

			font->data_cols = 5;
			font->data_rows = 10;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_MP10r;
			font->font_jisx201 = font_MP10r_jisx201;
			font->font_latin1_ex = font_MP10r_latin1ex;

			font->font_wide = font_MP10k_std_data;		// WIDE FONT DATA 
			font->font_wide_idx = font_MP10k_std_idx;	// UNICODE index 
			font->font_wide_missing = font_MP10k_unsupported;

			font->font_wide_count = 2646;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}


	/**********************************************************
	 * createFontMP10_full [chrs = 6941]
	 **********************************************************/
	extern const uint16_t font_MP10k_full_idx[6941];
	extern const uint8_t font_MP10k_full_data[6941*10*2];

	const FontDef& createFontMP10_full(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "M+ BITMAP FONTS 10dot (MP10_full, 6941)";

			font->width = 5;
			font->height = 10;

			font->data_cols = 5;
			font->data_rows = 10;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_MP10r;
			font->font_jisx201 = font_MP10r_jisx201;
			font->font_latin1_ex = font_MP10r_latin1ex;

			font->font_wide = font_MP10k_full_data;		// WIDE FONT DATA 
			font->font_wide_idx = font_MP10k_full_idx;	// UNICODE index 
			font->font_wide_missing = font_MP10k_unsupported;

			font->font_wide_count = 6941;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}


}

#include "MP10r.src"
#include "MP10k_mini.src"
#include "MP10k_std.src"
#include "MP10k_full.src"
