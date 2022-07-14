#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include "twe_common.hpp"
#include "twe_cui_keyboard.hpp"
#include <SDL.h>

namespace TWECUI {
	class KeyInput_SDL2 : public KeyInput {
	public: // override method of KeyInput
		KeyInput_SDL2() : KeyInput(512) {} // reserve more fifo buffer for pasting from clipboard.

		void setup(void* arg = nullptr) {
			return;
		}

		void update() {
			// nothing, SDL2 has direct events.
			return;
		}

	public: // class specific method
		bool handle_event(SDL_Event& e, int nTextEditing = 0);
	};

	extern KeyInput_SDL2 the_keyboard_sdl2;
}

#endif // WIN/MAC