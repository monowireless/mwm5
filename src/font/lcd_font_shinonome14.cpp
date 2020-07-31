/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */
#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_font.hpp"
#include "lcd_font_Shinonome14.h"

namespace TWEFONT {
	extern const uint8_t font_Shinonome14k_unsupported[14*2];
	extern const uint8_t font_Shinonome14r[1792];
	extern const uint8_t font_Shinonome14r_latin1ex[1344];
	extern const uint8_t font_Shinonome14r_jisx201[896];


	/**********************************************************
	 * createFontShinonome14 [chrs = 576]
	 **********************************************************/
	extern const uint16_t font_Shinonome14k_idx[576];
	extern const uint8_t font_Shinonome14k_data[576*14*2];

	const FontDef& createFontShinonome14(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "Shinonome font 14dot (Shinonome14, 576)";

			font->width = 7;
			font->height = 14;

			font->data_cols = 7;
			font->data_rows = 14;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_Shinonome14r;
			font->font_jisx201 = font_Shinonome14r_jisx201;
			font->font_latin1_ex = font_Shinonome14r_latin1ex;

			font->font_wide = font_Shinonome14k_data;		// WIDE FONT DATA 
			font->font_wide_idx = font_Shinonome14k_idx;	// UNICODE index 
			font->font_wide_missing = font_Shinonome14k_unsupported;

			font->font_wide_count = 576;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}


	/**********************************************************
	 * createFontShinonome14_std [chrs = 2645]
	 **********************************************************/
	extern const uint16_t font_Shinonome14k_std_idx[2645];
	extern const uint8_t font_Shinonome14k_std_data[2645*14*2];

	const FontDef& createFontShinonome14_std(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "Shinonome font 14dot (Shinonome14_std, 2645)";

			font->width = 7;
			font->height = 14;

			font->data_cols = 7;
			font->data_rows = 14;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_Shinonome14r;
			font->font_jisx201 = font_Shinonome14r_jisx201;
			font->font_latin1_ex = font_Shinonome14r_latin1ex;

			font->font_wide = font_Shinonome14k_std_data;		// WIDE FONT DATA 
			font->font_wide_idx = font_Shinonome14k_std_idx;	// UNICODE index 
			font->font_wide_missing = font_Shinonome14k_unsupported;

			font->font_wide_count = 2645;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}


	/**********************************************************
	 * createFontShinonome14_full [chrs = 6867]
	 **********************************************************/
	extern const uint16_t font_Shinonome14k_full_idx[6867];
	extern const uint8_t font_Shinonome14k_full_data[6867*14*2];

	const FontDef& createFontShinonome14_full(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "Shinonome font 14dot (Shinonome14_full, 6867)";

			font->width = 7;
			font->height = 14;

			font->data_cols = 7;
			font->data_rows = 14;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_Shinonome14r;
			font->font_jisx201 = font_Shinonome14r_jisx201;
			font->font_latin1_ex = font_Shinonome14r_latin1ex;

			font->font_wide = font_Shinonome14k_full_data;		// WIDE FONT DATA 
			font->font_wide_idx = font_Shinonome14k_full_idx;	// UNICODE index 
			font->font_wide_missing = font_Shinonome14k_unsupported;

			font->font_wide_count = 6867;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}


}

#include "Shinonome14r.src"
#include "Shinonome14k.src"
#include "Shinonome14k_std.src"
#include "Shinonome14k_full.src"
