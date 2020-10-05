/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */
#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_font.hpp"
#include "font_Shinonome12.h"

namespace TWEFONT {
	extern const uint8_t font_Shinonome12k_unsupported[12*2];
	extern const uint8_t font_Shinonome12r[1536];
	extern const uint8_t font_Shinonome12r_latin1ex[1152];
	extern const uint8_t font_Shinonome12r_jisx201[768];


	/**********************************************************
	 * createFontShinonome12_mini [chrs = 601]
	 **********************************************************/
	extern const uint16_t font_Shinonome12k_mini_idx[601];
	extern const uint8_t font_Shinonome12k_mini_data[601*12*2];

	const FontDef& createFontShinonome12_mini(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "Shinonome font 12dot (Shinonome12_mini, 601)";

			font->width = 6;
			font->height = 12;

			font->data_cols = 6;
			font->data_rows = 12;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_Shinonome12r;
			font->font_jisx201 = font_Shinonome12r_jisx201;
			font->font_latin1_ex = font_Shinonome12r_latin1ex;

			font->font_wide = font_Shinonome12k_mini_data;		// WIDE FONT DATA 
			font->font_wide_idx = font_Shinonome12k_mini_idx;	// UNICODE index 
			font->font_wide_missing = font_Shinonome12k_unsupported;

			font->font_wide_count = 601;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}


	/**********************************************************
	 * createFontShinonome12_std [chrs = 2645]
	 **********************************************************/
	extern const uint16_t font_Shinonome12k_std_idx[2645];
	extern const uint8_t font_Shinonome12k_std_data[2645*12*2];

	const FontDef& createFontShinonome12_std(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "Shinonome font 12dot (Shinonome12_std, 2645)";

			font->width = 6;
			font->height = 12;

			font->data_cols = 6;
			font->data_rows = 12;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_Shinonome12r;
			font->font_jisx201 = font_Shinonome12r_jisx201;
			font->font_latin1_ex = font_Shinonome12r_latin1ex;

			font->font_wide = font_Shinonome12k_std_data;		// WIDE FONT DATA 
			font->font_wide_idx = font_Shinonome12k_std_idx;	// UNICODE index 
			font->font_wide_missing = font_Shinonome12k_unsupported;

			font->font_wide_count = 2645;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}


	/**********************************************************
	 * createFontShinonome12_full [chrs = 6867]
	 **********************************************************/
	extern const uint16_t font_Shinonome12k_full_idx[6867];
	extern const uint8_t font_Shinonome12k_full_data[6867*12*2];

	const FontDef& createFontShinonome12_full(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "Shinonome font 12dot (Shinonome12_full, 6867)";

			font->width = 6;
			font->height = 12;

			font->data_cols = 6;
			font->data_rows = 12;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_Shinonome12r;
			font->font_jisx201 = font_Shinonome12r_jisx201;
			font->font_latin1_ex = font_Shinonome12r_latin1ex;

			font->font_wide = font_Shinonome12k_full_data;		// WIDE FONT DATA 
			font->font_wide_idx = font_Shinonome12k_full_idx;	// UNICODE index 
			font->font_wide_missing = font_Shinonome12k_unsupported;

			font->font_wide_count = 6867;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}


}

#include "Shinonome12r.src"
#include "Shinonome12k_mini.src"
#include "Shinonome12k_std.src"
#include "Shinonome12k_full.src"
