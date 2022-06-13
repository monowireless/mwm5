#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include "twe_common.hpp"
#include "mwm50.h"
#include "esp32/esp32_lcd_color.h"

#include "sdl2_common.h"
#include "twe_sdl_m5.h"

extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;
extern int SCREEN_POS_X;
extern int SCREEN_POS_Y;

struct twe_wid_button {
	enum class E_BTN_STATE {
		NONE = 0,
		HOVER,
		BTNDOWN,
		BTNUP,
		BTNUP_LONG,
		NO_MORE_ENTRY
	};

private:
	int _press_event;

	int _nButtonState; //button down: 1, button_long: 2, none: 0
	int _u32tickbutton; // tickcount when button is down (check long press)
	int _nButtonOver; // 1:A, 2:B, 3:C
	int _nHoldScreen; // -1: after fadeout, 0: not, 1... fadeout

	uint32_t _timer_id;

	std::wstring _label;
	std::unique_ptr<M5Stack> _pM5;
	std::unique_ptr< TWETerm_M5_Console> _pScr;
	SDL_Rect _location;
	SDL_Texture* _mTexture;
	bool _b_render_texture;

private:
	void _setup_dafault_vars() {
		; // nothing so far
	}

	inline int _screen_weight_w(int i) {
		return i * ::SCREEN_WIDTH / 640;
	}
	
	inline int _screen_weight_h(int i) {
		return i * ::SCREEN_HEIGHT / 480;
	}

	inline void reinit_state()
	{
		_nButtonState = 0;
		_nButtonOver = 0;
		_nHoldScreen = 0;
	}

	inline bool is_in_hotspot(SDL_Event& e) {
		int x = e.button.x;
		int y = e.button.y;

		bool b = (
			y >= _screen_weight_h(_location.y) + SCREEN_POS_Y
			&& y <= _screen_weight_h(_location.y + _location.h) + SCREEN_POS_Y
			&& x >= _screen_weight_w(_location.x) + SCREEN_POS_X
			&& x <= _screen_weight_w(_location.x + _location.w) + SCREEN_POS_X);

		return b;
	}

public:
	twe_wid_button(const wchar_t* lbl, SDL_Rect loc)
		: _press_event(0), _timer_id()
		, _label(lbl)
		, _pM5()
		, _pScr()
		, _location(loc)
		, _mTexture(nullptr)
		, _b_render_texture(true)
	{
		_pM5.reset(new M5Stack(loc.w, loc.h));
		_pScr.reset(new TWETerm_M5_Console(32, 1, { 0, 0, uint16_t(loc.w), uint16_t(loc.h) }, * _pM5));

		reinit_state();
		_setup_dafault_vars();
	}

	~twe_wid_button();

	// setup rest of
	// * shall be called after SDL_Renderer generation.
	void setup(SDL_Renderer* renderer);

	void redraw(bool force=false);

	// called every SDL events
	bool update(SDL_Event& e);
	bool update_core(SDL_Event& e);

	void show_button(int mode) {
		_nButtonOver = mode;
		_nButtonState = mode;
		redraw(true);
		start_timer1();
	}

private:
	static uint32_t callbackTimer1(uint32_t interval, void* param);
	
	void start_timer1();

public:
	inline bool available() {
		return (_press_event != 0);
	}

	inline int read() {
		int r = _press_event;
		_press_event = 0;
		return r;
	}
public:

	/* RENDER BOTTON BAR ON THE BOTTOM */
	void render_sdl(SDL_Renderer* renderer);
};

#endif // WIN/MAC