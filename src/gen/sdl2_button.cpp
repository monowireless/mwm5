/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include "sdl2_button.hpp"

void twe_wid_button::setup(SDL_Renderer* renderer) {
	if (_pScr) {
		auto& t = *_pScr;

		// setup screen
		int cols = 0;
		for (auto x : _label) cols += TWEUTILS::Unicode_isSingleWidth(x) ? 1 : 2;

		t.set_font(0x83, cols, 1);
		t.set_wraptext(false);
		t.set_color(WHITE, color565(0x0, 0x40, 0x40));
		t.set_cursor(0); // 0: no 1: curosr 2: blink cursor
		t.force_refresh();

		// draw initial label
		redraw();

		// generate texture
		_mTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
			_location.w, _location.h);
	}
}

twe_wid_button::~twe_wid_button() {
	SDL_DestroyTexture(_mTexture);
	_mTexture = nullptr;
}

void twe_wid_button::redraw() {
	const char fmtbtn[][16] = {
		"", // NONE
		"", // HOVER
		"\033[7m", // DOWN
		"\033[36;7m", // button up
		"\033[35;7m" // button up w/ long hold
	};

	if (_nButtonState < 0 || _nButtonState >= (int)E_BTN_STATE::NO_MORE_ENTRY) {
		_nButtonState = 0;
		_b_render_texture = true;
	}

	if (_pScr) {
		auto& t = *_pScr;

		t << "\033[2J"; // clear the line
		
		t << fmtbtn[_nButtonState];

		t << _label.c_str();

		t << "\033[0m";
	}

	if (_pScr) _pScr->refresh();
}

bool twe_wid_button::update(SDL_Event& e) {
	int prevbtn = _nButtonState;
	bool b;

	if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP || e.type == SDL_FINGERMOTION) {
		int x_s = int((::SCREEN_POS_X * 2 + ::SCREEN_WIDTH) * e.tfinger.x); // - ::SCREEN_POS_X;
		int y_s = int((::SCREEN_POS_Y * 2 + ::SCREEN_HEIGHT) * e.tfinger.y); // - ::SCREEN_POS_Y;

		SDL_Event em;
		memset(&em, 0, sizeof(SDL_Event));

		switch(e.type) {
		case SDL_FINGERDOWN: 
			em.type = SDL_MOUSEBUTTONDOWN;
			em.button.button = SDL_BUTTON_LEFT;
			em.button.timestamp = e.button.timestamp;
			em.button.clicks = 1;
			em.button.x = x_s;
			em.button.y = y_s;
			break;
		case SDL_FINGERUP:
			em.type = SDL_MOUSEBUTTONUP;
			em.button.button = SDL_BUTTON_LEFT;
			em.button.timestamp = e.button.timestamp;
			em.button.clicks = 1;
			em.button.x = x_s;
			em.button.y = y_s;
			break;
		case SDL_FINGERMOTION:
			em.type = SDL_MOUSEMOTION;
			em.motion.timestamp = e.motion.timestamp;
			em.motion.x = x_s;
			em.motion.x = y_s;
			break;
		}

		b = update_core(em);
	} else {
		b = update_core(e);
	}
	
	if (_nButtonState != prevbtn) {
		_b_render_texture = true;
	}

	return b;
}

bool twe_wid_button::update_core(SDL_Event& e) {
	if (!_nHoldScreen && (e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN)) {
		if (is_in_hotspot(e)) {
			if (e.type == SDL_MOUSEBUTTONDOWN) {
				_u32tickbutton = e.common.timestamp;
				_nButtonState = (int)E_BTN_STATE::BTNDOWN;
			}

			if (_nButtonState != (int)E_BTN_STATE::BTNDOWN) {
				_nButtonState = (int)E_BTN_STATE::HOVER;
			}

			_nButtonOver = 1;
			redraw();

			return true;
		}
		else {
			if (_nButtonOver) {
				_nHoldScreen = 0x77; // fadeout effect
				start_timer1();

				_nButtonOver = 0;
				_nButtonState = 0;
			}
		}
	}

	if (!_nHoldScreen && e.type == SDL_MOUSEBUTTONUP && _nButtonState == (int)E_BTN_STATE::BTNDOWN) {
		int x = _screen_weight_w(e.button.x);
		int y = _screen_weight_h(e.button.y);

		if (is_in_hotspot(e) && _press_event == 0) {
			uint32_t tick_delta = e.common.timestamp - _u32tickbutton;
			bool blong = (tick_delta < 10000 && tick_delta > 500); 

			if (blong) {
				_nButtonState = (int)E_BTN_STATE::BTNUP_LONG;
			}
			else {
				_nButtonState = (int)E_BTN_STATE::BTNUP;
			}
			redraw();

			if (blong) _press_event = 2; // long
			else _press_event = 1;

			_u32tickbutton = e.common.timestamp + 86400*1000; // set tick a day away.

			if (!_nHoldScreen) _nHoldScreen = 0xFF;
			start_timer1();

			_nButtonOver = 0;
			_nButtonState = 0;

			return true;
		}
	}

	return false;
}

uint32_t twe_wid_button::callbackTimer1(uint32_t interval, void* param) {
	twe_wid_button* btn = (twe_wid_button*)param;
	btn->reinit_state();
	return 0;
}

void twe_wid_button::start_timer1() {
	_timer_id = SDL_AddTimer(500, callbackTimer1, (void*)this);
}

void twe_wid_button::render_sdl(SDL_Renderer* renderer) {
	if (_mTexture) {
		auto& lcd = _pM5->Lcd;

		if (_nButtonOver > 0 || (_nHoldScreen > 0 && (_nHoldScreen & 0xF) > 0)) {
			SDL_Rect rect_src = { 0, 0, _location.w, _location.h };
			SDL_Rect rct_dst = {
				_screen_weight_w(_location.x),
				_screen_weight_h(_location.y),
				_screen_weight_w(_location.w),
				_screen_weight_h(_location.h),
			};

			if (_b_render_texture) {
				SDL_SetRenderTarget(renderer, _mTexture);
				for (int y = 0; y < _location.h; y++) {
					if (lcd.update_line(y)) {
						for (int x = 0; x < _location.w; x++) {
							auto c = lcd.get_pt(x, y);

							SDL_SetRenderDrawColor(renderer, c.u8col[0], c.u8col[1], c.u8col[2], 0xFF);
							SDL_RenderDrawPoint(renderer, x, y);
						}
					}
				}
				_b_render_texture = false;
			}

			int alpha = 0xff;
			if (g_enable_fade_effect) {
				if (_nHoldScreen == 0) { // normal
					alpha = 0xC0;
				}
				else if (_nHoldScreen == -1) { // hold count expires, reset by timer
					alpha = 0;
				}
				else { // hold count (after button press)
					int mdiv = (_nHoldScreen >> 4) + 1;
					alpha = (_nHoldScreen & 0xF) * 0xc0 / mdiv;
				}
			}

			// render bottom bar
			SDL_SetRenderTarget(renderer, NULL);
			SDL_SetTextureBlendMode(_mTexture, SDL_BLENDMODE_BLEND);
			SDL_SetTextureAlphaMod(_mTexture, alpha);
			SDL_RenderCopy(renderer, _mTexture, &rect_src, &rct_dst);

			if (_nHoldScreen > 0 && (_nHoldScreen & 0xF) > 0) {
				_nHoldScreen--;
				if ((_nHoldScreen & 0xF) == 0) {
					_nHoldScreen = -1;
				}
			}
		}
	}
}

#endif // WIN/MAC/LINUX