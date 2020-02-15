/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */
#pragma once

#include <mwm5.h>

namespace TWEM5 {
	// lcd console defs
	extern TWEARD::TWETerm_M5_Console the_screen;   // middle screen.
	extern TWEARD::TWETerm_M5_Console the_screen_b; // bottom screen.
	extern TWEARD::TWETerm_M5_Console the_screen_c; // bottom bar.
	extern TWEARD::TWETerm_M5_Console the_screen_t; // top bar.

	// serial parser
	extern TWESERCMD::AsciiParser parse_ascii;

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
}