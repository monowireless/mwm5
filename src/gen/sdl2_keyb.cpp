/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include "twe_common.hpp"
#include "twe_cui_keyboard.hpp"

#include "sdl2_common.h"
#include "sdl2_keyb.hpp"

using namespace TWE;
using namespace TWECUI;


bool KeyInput_SDL2::handle_event(SDL_Event& e, int nTextEditing) {
	bool bHandled = false;

	if (e.type == SDL_KEYDOWN) {
		if (!(e.key.keysym.mod & (KMOD_STG | KMOD_CTRL))) {
			int key = 0;
			
			if (!key && e.key.keysym.scancode == SDL_SCANCODE_KP_ENTER) key = KeyInput::KEY_ENTER;
			if (!key && e.key.keysym.sym == SDLK_RETURN) key = KeyInput::KEY_ENTER;
			if (!key && e.key.keysym.sym == SDLK_UP) key = KeyInput::KEY_UP;
			if (!key && e.key.keysym.sym == SDLK_LEFT) key = KeyInput::KEY_LEFT;
			if (!key && e.key.keysym.sym == SDLK_DOWN) key = KeyInput::KEY_DOWN;
			if (!key && e.key.keysym.sym == SDLK_RIGHT) key = KeyInput::KEY_RIGHT;
			if (!key && e.key.keysym.sym == SDLK_ESCAPE) key = KeyInput::KEY_ESC;
			if (!key && e.key.keysym.sym == SDLK_BACKSPACE) key = KeyInput::KEY_BS;
			if (!key && e.key.keysym.sym == SDLK_PAGEUP) key = KeyInput::KEY_PAGEUP;
			if (!key && e.key.keysym.sym == SDLK_PAGEDOWN) key = KeyInput::KEY_PAGEDN;

			if (key) {
				bool ret = true;

				if (key == 0x0d)
				{
					if (nTextEditing != 1) {
						KeyInput::push(0x0d);
						KeyInput::push(0x0a);
					} else
					{
						ret = false;
					}
				}
				else KeyInput::push(key);

				return ret;
			}
		}
	}

	if (e.type == SDL_TEXTINPUT) {
		char *p = e.text.text;
		while (*p) {
			KeyInput::push(uint8_t(*p));
			++p;
		}

		if (p != e.text.text)
			return true;
	}

	return false;
}

KeyInput_SDL2 TWECUI::the_keyboard_sdl2; // raw access.
KeyInput& TWE::the_keyboard = TWECUI::the_keyboard_sdl2;

#endif // WIN/MAC