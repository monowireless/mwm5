#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

// Tell SDL2 not to replace main as SDL_Main
#define SDL_MAIN_HANDLED

#include <SDL.h>

// Use Alt or Command Key.
#ifdef __APPLE__
#define KMOD_STG KMOD_GUI
#else
#define KMOD_STG KMOD_ALT
#endif