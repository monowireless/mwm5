/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */
#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_font.hpp"
#include "font_MP12.h"

namespace TWEFONT {
	extern const uint8_t font_MP12k_unsupported[12*2];
	extern const uint8_t font_MP12r[1536];
	extern const uint8_t font_MP12r_latin1ex[1152];
	extern const uint8_t font_MP12r_jisx201[768];


	/**********************************************************
	 * createFontMP12_mini [chrs = 638]
	 **********************************************************/
	extern const uint16_t font_MP12k_mini_idx[638];
	extern const uint8_t font_MP12k_mini_data[638*12*2];

	const FontDef& createFontMP12_mini(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "M+ BITMAP FONTS 12dot (MP12_mini, 638)";

			font->width = 6;
			font->height = 12;

			font->data_cols = 6;
			font->data_rows = 12;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_MP12r;
			font->font_jisx201 = font_MP12r_jisx201;
			font->font_latin1_ex = font_MP12r_latin1ex;

			font->font_wide = font_MP12k_mini_data;		// WIDE FONT DATA 
			font->font_wide_idx = font_MP12k_mini_idx;	// UNICODE index 
			font->font_wide_missing = font_MP12k_unsupported;

			font->font_wide_count = 638;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}


	/**********************************************************
	 * createFontMP12_std [chrs = 2646]
	 **********************************************************/
	extern const uint16_t font_MP12k_std_idx[2646];
	extern const uint8_t font_MP12k_std_data[2646*12*2];

	const FontDef& createFontMP12_std(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "M+ BITMAP FONTS 12dot (MP12_std, 2646)";

			font->width = 6;
			font->height = 12;

			font->data_cols = 6;
			font->data_rows = 12;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_MP12r;
			font->font_jisx201 = font_MP12r_jisx201;
			font->font_latin1_ex = font_MP12r_latin1ex;

			font->font_wide = font_MP12k_std_data;		// WIDE FONT DATA 
			font->font_wide_idx = font_MP12k_std_idx;	// UNICODE index 
			font->font_wide_missing = font_MP12k_unsupported;

			font->font_wide_count = 2646;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}


	/**********************************************************
	 * createFontMP12_full [chrs = 6941]
	 **********************************************************/
	extern const uint16_t font_MP12k_full_idx[6941];
	extern const uint8_t font_MP12k_full_data[6941*12*2];

	const FontDef& createFontMP12_full(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "M+ BITMAP FONTS 12dot (MP12_full, 6941)";

			font->width = 6;
			font->height = 12;

			font->data_cols = 6;
			font->data_rows = 12;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_MP12r;
			font->font_jisx201 = font_MP12r_jisx201;
			font->font_latin1_ex = font_MP12r_latin1ex;

			font->font_wide = font_MP12k_full_data;		// WIDE FONT DATA 
			font->font_wide_idx = font_MP12k_full_idx;	// UNICODE index 
			font->font_wide_missing = font_MP12k_unsupported;

			font->font_wide_count = 6941;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}


}

#include "MP12r.src"
#include "MP12k_mini.src"
#include "MP12k_std.src"
#include "MP12k_full.src"
