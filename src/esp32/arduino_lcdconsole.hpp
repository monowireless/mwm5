#pragma once

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

#ifdef ARDUINO

#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_utils_simplebuffer.hpp"
#include "twe_console.hpp"
#include "twe_stream.hpp"

#include <M5Stack.h>

// define while color (0xFFFF behaves transparent)
#define ALMOST_WHITE ((30 << 11) | (62 << 5) | 30)

namespace TWEARD {
	extern const uint16_t COLTBL_DEFAULT[8];

	constexpr uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
		return ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
	}

	struct Rect {
		uint16_t x;
		uint16_t y;
		uint16_t w;
		uint16_t h;
	};

	class TWETerm_M5_Console : public TWETERM::ITerm {
		M5Stack& _M5;
		uint8_t _font_id;
		uint8_t _font_width;
		uint8_t _font_height;
		uint8_t _off_x;
		uint8_t _off_y;

		// cursor control
		uint8_t _u8cursor = 0;
		uint16_t _u16timer_blink;
		uint8_t _cursor_l_last = 0;
		uint8_t _cursor_c_last = 0;

		// color control
		uint16_t _fg, _bg, _fg_def, _bg_def;

		Rect _rect; // the area to be used for console display.

		// the color table
		const uint16_t* COL_TBL;

	public:
		inline uint8_t font_width() { return _font_width; }
		inline uint8_t font_height() { return _font_height; }
		inline uint8_t font_id() { return _font_id; }

	public:
		// set default screen parameter
		void _init() {
			COL_TBL = COLTBL_DEFAULT;

			set_font(1);
			set_color(ALMOST_WHITE, BLACK);
		}

		// construct with pre-allocated memory
		TWETerm_M5_Console(
			uint8_t u8c, uint8_t u8l,
			TWETERM::SimpBuf_GChar* pAryLines, TWETERM::GChar* pBuff,
			Rect drawArea, M5Stack& _M5)
			: ITerm(u8c, u8l, pAryLines, pBuff), _rect(drawArea), _M5(M5) 
		{
			_init();
		}

		// construct with dynamic-allocated memory
		TWETerm_M5_Console(
			uint8_t u8c, uint8_t u8l,
			Rect drawArea, M5Stack& _M5)
			: ITerm(u8c, u8l), _rect(drawArea), _M5(M5)
		{
			_init();
		}

		// report absolute cursor coordinates from cursor position (c, l)
		inline bool get_cursor_pos(uint16_t& x, uint16_t& y, uint8_t c, uint8_t l) {
			x = (uint16_t)font_width() * c + _off_x + _rect.x;
			y = (uint16_t)font_height() * l + _off_y + _rect.y;

			return true;
		}

		void set_font(uint8_t u8id, uint8_t u8col_request = 0, uint8_t u8row_request = 0); // change font and resize screen
		void set_color(uint16_t color, uint16_t bgcolor = 0) { _bg = _bg_def = bgcolor; _fg = _fg_def = color; }
		void set_color_table(const uint16_t* ptbl) {
			COL_TBL = ptbl;
		}
		void refresh(); // redraw function
	};

} // end of namespace TWEARD

#endif