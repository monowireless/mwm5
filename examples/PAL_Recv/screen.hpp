#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <mwm5.h>

// lcd console defs
extern TWETerm_M5_Console the_screen;   // middle screen.
extern TWETerm_M5_Console the_screen_b; // bottom screen.
extern TWETerm_M5_Console the_screen_c; // bottom bar.
extern TWETerm_M5_Console the_screen_t; // top bar.

// setup procedure
extern void setup_screen();

// update screen
extern void check_for_refresh();

// change screen font of `the_screen'
extern void change_screen_font();

// change screen color of `the_screen'
extern void change_screen_color();

// color settings
extern uint16_t default_bg_color;
extern uint16_t default_fg_color;
