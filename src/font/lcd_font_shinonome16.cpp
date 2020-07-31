/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */
#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_font.hpp"
#include "lcd_font_Shinonome16.h"

namespace TWEFONT {
	extern const uint8_t font_Shinonome16k_unsupported[16*2];
	extern const uint8_t font_Shinonome16r[2048];
	extern const uint8_t font_Shinonome16r_latin1ex[1536];
	extern const uint8_t font_Shinonome16r_jisx201[1024];


	/**********************************************************
	 * createFontShinonome16 [chrs = 577]
	 **********************************************************/
	extern const uint16_t font_Shinonome16k_idx[577];
	extern const uint8_t font_Shinonome16k_data[577*16*2];

	const FontDef& createFontShinonome16(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "Shinonome font 16dot (Shinonome16, 577)";

			font->width = 8;
			font->height = 16;

			font->data_cols = 8;
			font->data_rows = 16;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_Shinonome16r;
			font->font_jisx201 = font_Shinonome16r_jisx201;
			font->font_latin1_ex = font_Shinonome16r_latin1ex;

			font->font_wide = font_Shinonome16k_data;		// WIDE FONT DATA 
			font->font_wide_idx = font_Shinonome16k_idx;	// UNICODE index 
			font->font_wide_missing = font_Shinonome16k_unsupported;

			font->font_wide_count = 577;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}


	/**********************************************************
	 * createFontShinonome16_std [chrs = 2646]
	 **********************************************************/
	extern const uint16_t font_Shinonome16k_std_idx[2646];
	extern const uint8_t font_Shinonome16k_std_data[2646*16*2];

	const FontDef& createFontShinonome16_std(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "Shinonome font 16dot (Shinonome16_std, 2646)";

			font->width = 8;
			font->height = 16;

			font->data_cols = 8;
			font->data_rows = 16;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_Shinonome16r;
			font->font_jisx201 = font_Shinonome16r_jisx201;
			font->font_latin1_ex = font_Shinonome16r_latin1ex;

			font->font_wide = font_Shinonome16k_std_data;		// WIDE FONT DATA 
			font->font_wide_idx = font_Shinonome16k_std_idx;	// UNICODE index 
			font->font_wide_missing = font_Shinonome16k_unsupported;

			font->font_wide_count = 2646;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}


	/**********************************************************
	 * createFontShinonome16_full [chrs = 6868]
	 **********************************************************/
	extern const uint16_t font_Shinonome16k_full_idx[6868];
	extern const uint8_t font_Shinonome16k_full_data[6868*16*2];

	const FontDef& createFontShinonome16_full(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "Shinonome font 16dot (Shinonome16_full, 6868)";

			font->width = 8;
			font->height = 16;

			font->data_cols = 8;
			font->data_rows = 16;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_Shinonome16r;
			font->font_jisx201 = font_Shinonome16r_jisx201;
			font->font_latin1_ex = font_Shinonome16r_latin1ex;

			font->font_wide = font_Shinonome16k_full_data;		// WIDE FONT DATA 
			font->font_wide_idx = font_Shinonome16k_full_idx;	// UNICODE index 
			font->font_wide_missing = font_Shinonome16k_unsupported;

			font->font_wide_count = 6868;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}


}

#include "Shinonome16r.src"
#include "Shinonome16k.src"
#include "Shinonome16k_std.src"
#include "Shinonome16k_full.src"
