/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)
#include <SDL.h>
#include "sdl2_clipboard.hpp"

using namespace TWE;
void TWE::twe_clipboard::_copy::copy_to_clip(const char *str) {
    SDL_SetClipboardText(str);
    _breq = false;
};

void TWE::twe_clipboard::_paste::past_from_clip() {
    if (SDL_HasClipboardText()) {
        char *ptr = SDL_GetClipboardText();
        if (ptr) {
            while(*ptr) {
                _que.push(*ptr++);
            }
        }
    }
}

twe_clipboard TWE::the_clip;
#endif