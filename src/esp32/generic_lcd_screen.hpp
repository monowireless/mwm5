#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(ESP32)
#include "twe_common.hpp"
#include "twe_font.hpp"
#include "generic_lcd_common.h"

//#include <M5Stack.h>

#elif defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)
#include <vector>

#include "twe_common.hpp"
#include "twe_font.hpp"
#include "generic_lcd_common.h"
#include "generic_lcd_screen.hpp"

extern bool g_quit_sdl_loop;
			
namespace TWEARD {
	class LcdScreen {
		int32_t _w; // screen width
		int32_t _h; // screen height

		RGBA* _fb; // screen buffer
		std::vector<bool> _y_upd; // lines flags, ture to be rendered.

		Rect _window; // set rendering window (simulate LCD op)
		int32_t _window_x, _window_y; // render pix position for writeWindows565(), where the rendering window set by setWindow()

	public:
		LcdScreen(int w, int h) :
			_w(w),
			_h(h),
			_fb(new RGBA[_w * _h]),
			_y_upd(_h, false),
			_window{},
			_window_x(0),
			_window_y(0) { }

		~LcdScreen() {
			delete[] _fb;
		}

		inline RGBA get_pt(int32_t x, int32_t y) {
			return _fb[_w * y + x];
		}

		inline RGBA& ref_pt(int32_t x, int32_t y) {
			if (y >= 0 && y < _h) _y_upd[y] = true; 
			return _fb[_w * y + x];
		}

		void setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
			_window.x = x0;
			_window.y = y0;
			_window.w = x1 - x0 + 1;
			_window.h = y1 - y0 + 1;

			_window_x = x0;
			_window_y = y0;
		}

		void writeWindows565(uint16_t c) {
			if (_window_x < _w && _window_y < _h) {
				_y_upd[_window_y] = true;

				auto&& p = ref_pt(_window_x, _window_y);
				p = color565toRGBA(c);
			}

			_window_x++;
			if (_window_x >= _window.x + _window.w) {
				_window_x = _window.x;
				_window_y++;
			}
		}

		void fillRect(const int32_t x, const int32_t y, const int32_t w, const int32_t h, const uint32_t c) {
			RGBA c32 = color565toRGBA(c);

			int32_t wx = x, wy = y;
			
			for (; wy < y + h; wy++) {
				if (wy >= 0 && wy < _h) _y_upd[wy] = true;

				for (wx = x; wx < x + w; wx++) {
					auto&& p = ref_pt(wx, wy);
					p = c32;
				}
			}
		}

		// do nothing
		void startWrite() {}
		void endWrite() {}

		// update
		bool update_line(int32_t y) {
			bool bupd = _y_upd[y];
			_y_upd[y] = false;
			return bupd;
		}

		void update_line_all() {
			_y_upd.assign(_y_upd.size(), true);
		}

	};

	class M5Stack {
	public:
		M5Stack(int32_t lcd_w, int32_t lcd_h) : Lcd(lcd_w, lcd_h), BtnA{}, BtnB{}, BtnC{} {}

		void begin(bool, bool, bool, bool) {};
		void update() {}
		void powerOFF() {
			::g_quit_sdl_loop = true;
		}
		struct _Power {
			void begin() {}
		} Power;

		struct _Btn {
			_Btn() : _press(false), _lpress(false) {}

			bool wasReleased() {
				if (_press) {
					_press = false;
					return true;
				}
				return false;
			}
			bool wasReleasefor(int i) {
				if (_lpress) {
					_lpress = false;
					return true;
				}
				return false;
			}
			bool _press;
			bool _lpress;
		} BtnA, BtnB, BtnC;

		void _keyin(uint8_t c) {
			switch (c) {
			case 'a': BtnA._press = true; break;
			case 'b': BtnB._press = true; break;
			case 'c': BtnC._press = true; break;
			case 'd': BtnA._lpress = true; break;
			case 'e': BtnB._lpress = true; break;
			case 'f': BtnC._lpress = true; break;
			}
		}

	public:
		TWEARD::LcdScreen Lcd;
	};
}
extern TWEARD::M5Stack M5; // dummy class representing M5
#endif

namespace TWEARD {
	int16_t drawChar(const TWEFONT::FontDef& font, int32_t x, int32_t y, uint16_t c, uint32_t color, uint32_t bg, uint8_t opt, M5Stack& _M5);
	int16_t drawChar(const TWEFONT::FontDef& font, int32_t x, int32_t y, const char* s, uint32_t color, uint32_t bg, uint8_t opt, M5Stack& _M5);
	int16_t drawChar(const TWEFONT::FontDef& font, int32_t x, int32_t y, const uint16_t* s, uint32_t color, uint32_t bg, uint8_t opt, M5Stack& _M5);
}