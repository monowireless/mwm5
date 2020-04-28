/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#pragma once 

#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_utils_unicode.hpp"

namespace TWEFONT {
	const uint32_t U32_OPT_FONT_TATEBAI = 0x0100;
	const uint32_t U32_OPT_FONT_YOKOBAI = 0x0200;

	struct FontDef {
	private:
		/// <summary>
		/// set THIS object as default font.
		/// </summary>
		/// <returns></returns>
		FontDef& set_default();

	public:
		uint8_t font_code;			// as registered code (0: default font)
		uint8_t _default_font;		// 1: default font, 0: other font

		uint8_t width;				// font width
		uint8_t height;				// font height

		uint8_t w_space;			// char space (padding pixels between chars)
		uint8_t h_space;			// line space (padding pixels between lines)
		
		uint8_t data_cols;			// data count for column (should be same with width, otherwise experimental)
		uint8_t data_rows;			// data count for rows (should be same with height, otherwise experimental)

		uint32_t opt;				// attribute

		const char* font_name;
		const uint8_t* font_latin1;		// standard latin set (0x00-0x7F)
		const uint8_t* font_latin1_ex;	// extended latic chars (0xA0-0XFF)

		const uint8_t* font_jisx201;	// JIS X201 HANKAKU KANA char set

		const uint8_t* font_wide;			// wide font data
		const uint8_t* font_wide_missing;	// a wide char data used when missing. (use dotted box)
		const uint16_t* font_wide_idx;		// stores supported Unicode value at the index of font_wide[]
		uint16_t font_wide_count;			// total count of wide chars stored.

		/// <summary>
		/// get font width with additional space(w_space)
		/// </summary>
		/// <returns></returns>
		inline uint8_t get_width() const { return width * ((opt & U32_OPT_FONT_YOKOBAI) ? 2 : 1) + w_space; }

		/// <summary>
		/// get font width with additional space(w_space) WITH specifing character code.
		/// </summary>
		/// <returns></returns>
		inline uint8_t get_width(uint16_t wc) const {
			return TWEUTILS::Unicode_isSingleWidth(wc) ? get_width() : get_width() * 2;
		}

		/// <summary>
		/// get font height with additional space(h_space)
		/// </summary>
		/// <returns></returns>
		inline uint8_t get_height() const { return height * ((opt & U32_OPT_FONT_TATEBAI) ? 2 : 1) + h_space; }

		/// <summary>
		/// the constructor
		/// </summary>
		/// <param name="id">stores font_code.</param>
		/// <param name="b_default_font">true if it's created as default font instance.</param>
		FontDef(uint8_t id = 0, bool b_default_font = false) :
			font_code(id), font_name(""),
			width(0), w_space(0),
			height(0), h_space(0),
			data_cols(0), data_rows(0),
			font_latin1(0), font_latin1_ex(0), font_jisx201(0),
			font_wide(0), font_wide_missing(0), font_wide_idx(0), font_wide_count(0),
			opt(0)
		{
			_default_font = b_default_font ? 1 : 0;

			if (_default_font) {
				set_default();
			}
		}

		/// <summary>
		/// check if the instance is the default (the default instance is singleton).
		/// </summary>
		/// <returns>true if the instance is default.</returns>
		inline bool is_default() const  { return _default_font; }

		/// <summary>
		/// get font code(id).
		/// </summary>
		/// <returns>font code</returns>
		inline uint8_t get_font_code() const { return font_code; }

		/// <summary>
		/// find font index data from unicode using binary search.
		/// </summary>
		/// <param name="c">unicode char</param>
		/// <returns>-1: not found, 0>=: found the fond.</returns>
		inline int find_font_index(uint16_t c) const {
			int b = 0;
			int e = this->font_wide_count - 1;
			int m = 0;

			if (this->font_wide_idx == nullptr) {
				return -1;
			}

			while (b != e) {
				m = (b + e) >> 1;
				if (this->font_wide_idx[m] == c) { // bingo!
					return m;
				}
				else if (e - b <= 1) {
					if (this->font_wide_idx[e] == c) {
						return m;
					}
					else {
						// not found.
						return -1;
					}

				}
				else if (this->font_wide_idx[m] < c) {
					b = m;
				}
				else {
					e = m;
				}
			}

			return -1;
		}
	};

	const struct FontDef& queryFont(uint8_t id);
	struct FontDef* _queryFont(uint8_t id);
}
