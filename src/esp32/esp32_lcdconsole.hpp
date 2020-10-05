#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"

#include "twe_utils.hpp"
#include "twe_utils_simplebuffer.hpp"
#include "twe_console.hpp"
#include "twe_stream.hpp"


#include "esp32_lcd_color.h"
#include "generic_lcd_common.h"
#include "generic_lcd_screen.hpp"

#include "twe_font.hpp"

#if defined(ESP32)

#elif defined (_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#endif

namespace TWEARD {
	extern const uint16_t COLTBL_DEFAULT[8];

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
		Rect _rect_view; // the area is actual view area.

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

			_rect_view.w = _rect.w;
			_rect_view.h = _rect.h;
			_rect_view.x = 0;
			_rect_view.y = 0;
		}

		// construct with pre-allocated memory
		TWETerm_M5_Console(
			uint8_t u8c, uint8_t u8l,
			TWETERM::SimpBuf_GChar* pAryLines, TWETERM::GChar* pBuff,
			Rect drawArea, M5Stack& M5OBJ)
			: ITerm(u8c, u8l, pAryLines, pBuff), _rect(drawArea), _M5(M5OBJ)
		{
			_init();
		}

		// construct with dynamic-allocated memory
		TWETerm_M5_Console(
			uint8_t u8c, uint8_t u8l,
			Rect drawArea, M5Stack& M5OBJ)
			: ITerm(u8c, u8l), _rect(drawArea), _M5(M5OBJ)
		{
			_init();
		}

		// report (column, line) from absolute position
		_coord_cols_lines get_term_coord_from_screen(int16_t x, int16_t y) {
			int16_t c, l;
			uint8_t brange = false;

			c = (x + _rect_view.x - (_off_x + _rect.x)) / font_width();
			l = (y + _rect_view.y - (_off_y + _rect.y)) / font_height();


			if ((x >= _rect.x && x <= _rect.x + _rect.w)
				&& (y >= _rect.y && y <= _rect.y + _rect.h))
			{
				brange = true;
			}

			return { c, l, brange };
		}

		// report absolute coordinates from character position (c, l)
		inline bool get_cursor_pos(int16_t& x, int16_t& y, int16_t c, int16_t l) {
			x = (int16_t)font_width() * c + _off_x + _rect.x - _rect_view.x;
			y = (int16_t)font_height() * l + _off_y + _rect.y - _rect_view.y;

			return true;
		}

		// report absolute coordinates of current cursor position
		inline bool get_cursor_pos(int16_t& x, int16_t& y) {
			return get_cursor_pos(x, y, cursor_c, cursor_l);
		}

		// report relative coordinates from character position (c, l)
		inline bool get_cursor_pos_relative(int16_t& x, int16_t& y, int16_t c, int16_t l) {
			x = (int16_t)font_width() * c + _off_x;
			y = (int16_t)font_height() * l + _off_y;

			return true;
		}

		// report relative coordinates of current cursor position
		inline bool get_cursor_pos_relative(int16_t& x, int16_t& y) {
			return get_cursor_pos_relative(x, y, cursor_c, cursor_l);
		}

		// set position of virtual area.
		void set_view_pos(int16_t x, int16_t y) {
			_rect_view.x = x;
			_rect_view.y = y;
		}

		// set actual rendering size.
		void set_view_size(int16_t w, int16_t h) {
			_rect_view.w = w;
			_rect_view.h = h;
		}

		// set drawing area (should be set font again)
		void set_draw_area(Rect drawArea) {
			_rect = drawArea;
		}

		void set_font(uint8_t u8id, uint8_t u8col_request = 0, uint8_t u8row_request = 0); // change font and resize screen
		void set_color(uint16_t color, uint16_t bgcolor = 0) { _bg = _bg_def = bgcolor; _fg = _fg_def = color; }
		void set_color_table(const uint16_t* ptbl) {
			COL_TBL = ptbl;
		}

		void refresh(); // redraw function
	};

} // end of namespace TWEARD
