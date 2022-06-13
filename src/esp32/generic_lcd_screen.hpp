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
#include <utility>

#include "twe_common.hpp"
#include "twe_font.hpp"
#include "generic_lcd_common.h"

extern bool g_quit_sdl_loop;
			
namespace TWEARD {
	class LcdScreen {
		typedef bool boolean_type;
		int32_t _w; // screen width
		int32_t _h; // screen height

		RGBA* _fb; // screen buffer
		RGBA _fb_out;
		//std::vector<bool> _y_upd; // lines flags, ture to be rendered.
		boolean_type*_y_upd;

		Rect _window; // set rendering window (simulate LCD op)
		int32_t _window_x, _window_y; // render pix position for writeWindows565(), where the rendering window set by setWindow()

	public:
		LcdScreen(int w, int h) :
			_w(w),
			_h(h),
			_fb(new RGBA[int32_t(_w * _h)]), _fb_out(),
			//_y_upd(_h, false),
			_y_upd(new boolean_type[_h]),
			_window(),
			_window_x(0),
			_window_y(0) { }

		~LcdScreen() {
			delete[] _fb;
		}

		inline RGBA get_pt(int32_t x, int32_t y) {
			if (x < 0 || x >= _w || y < 0 || y >= _h) return _fb_out;
			else return _fb[_w * y + x];
		}

		inline RGBA& ref_pt(int32_t x, int32_t y) {
			if (y >= 0 && y < _h) _y_upd[y] = true; 

			if (x < 0 || x >= _w || y < 0 || y >= _h) return _fb_out;
			else return _fb[_w * y + x];
		}

		// set window region in M5 Stack's API.
		void setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
			_window.x = x0;
			_window.y = y0;
			_window.w = x1 - x0 + 1;
			_window.h = y1 - y0 + 1;

			_window_x = x0;
			_window_y = y0;
		}

		// put a pixel into the window region (from top-left)
		void writeWindows565(uint16_t c) {
			if (_window_x < _w && _window_y < _h) {
				_y_upd[_window_y] = true;

				auto&& p = ref_pt(_window_x, _window_y);
				if (c != 0xFFFF) p = color565toRGBA(c);
			}

			_window_x++;
			if (_window_x >= _window.x + _window.w) {
				_window_x = _window.x;
				_window_y++;
			}
		}

		// do nothing
		void startWrite() {}
		void endWrite() {}

		// unmark the line 'y' which has been updated(rendered)
		bool update_line(int32_t y) {
			bool bupd = _y_upd[y];
			_y_upd[y] = false;
			return bupd;
		}

		// mark all lines which should be updated(redrawed)
		void update_line_all() {
			for (int i = 0; i < _h; i++) _y_upd[i] = true;
		}

		// drawing APIs
		void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, const RGBA c);
		void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t c) { drawRect(x, y, w, h, color565toRGBA(c)); }
		void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, const RGBA c);
		void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t c) { fillRect(x, y, w, h, color565toRGBA(c)); }

		void drawPixel(int32_t x, int32_t y, const RGBA c32) { ref_pt(x, y) = c32; }
		void drawPixel(int32_t x, int32_t y, const uint16_t c) { ref_pt(x, y) = color565toRGBA(c); }

		void drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const RGBA c);
		void drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t c) { drawLine(x1, y1, x2, y2, color565toRGBA(c)); }
		void drawCircle(int32_t x, int32_t y, int32_t r, const RGBA c);
		void drawCircle(int32_t x, int32_t y, int32_t r, const uint16_t c) { drawCircle(x, y, r, color565toRGBA(c)); }
		void fillCircle(int32_t x, int32_t y, int32_t r, const RGBA c);
		void fillCircle(int32_t x, int32_t y, int32_t r, const uint16_t c) { fillCircle(x, y, r, color565toRGBA(c)); }
	};

	class M5Stack {
	public:
		M5Stack(int32_t lcd_w, int32_t lcd_h) : Lcd(lcd_w, lcd_h), BtnA(), BtnB(), BtnC() {}

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

			// same with Released so far.
			bool wasPressed() {
				if (_press) {
					_press = false;
					return true;
				}
				return false;
			}

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
	// set drawing area
	static inline void setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1, M5Stack& _M5) {
		_M5.Lcd.setWindow(x0, y0, x1, y1);
	}

	static inline void startWrite(M5Stack& _M5) {
		_M5.Lcd.startWrite();
	}

	static inline void endWrite(M5Stack& _M5) {
		_M5.Lcd.endWrite();
	}

	static inline void windowWrite16(uint16_t c, M5Stack& _M5) {
#if defined(ESP32)
		tft_Write_16(c);
#elif defined (_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)
		_M5.Lcd.writeWindows565(c);
#endif
	}

	int16_t drawChar(const TWEFONT::FontDef& font, int32_t x, int32_t y, uint16_t c, uint32_t color, uint32_t bg, uint8_t opt, M5Stack& _M5);
	int16_t drawChar(const TWEFONT::FontDef& font, int32_t x, int32_t y, const char* s, uint32_t color, uint32_t bg, uint8_t opt, M5Stack& _M5);
	// int16_t drawChar(const TWEFONT::FontDef& font, int32_t x, int32_t y, const uint16_t* s, uint32_t color, uint32_t bg, uint8_t opt, M5Stack& _M5);
	int16_t drawChar(const TWEFONT::FontDef& font, int32_t x, int32_t y, const wchar_t* s, uint32_t color, uint32_t bg, uint8_t opt, M5Stack& _M5);
}