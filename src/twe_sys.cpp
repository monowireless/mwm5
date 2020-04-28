/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_sys.hpp"

#if defined(ESP32)
uint32_t _u32GetTick_ms() {
	return millis();
}
#elif defined(_MSC_VER)|| defined(__MINGW32__)
#include <Windows.h>
#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif
#elif defined(__APPLE__) || defined(__linux)
#include <unistd.h>
# ifdef USE_CURSES
#include <curses.h>
# endif
#endif

#ifndef ESP32
uint32_t _u32TickCount_ms_on_boot;
extern "C" volatile uint32_t u32TickCount_ms;
#endif

void TWESYS::Sleep_ms(int ms) {
#if defined(ESP32)
	delay(ms);
#else
# if defined(_MSC_VER) || defined(__MINGW32__)
	Sleep(ms);
# elif defined(__APPLE__) || defined(__linux)
	usleep(ms * 1000);
# endif
	u32TickCount_ms = u32GetTick_ms();
#endif
}

void TWESYS::SysInit() {
#if defined(ESP32)
	// nothing
#elif defined(_MSC_VER)
	timeBeginPeriod(1); // increase the accuracy of timer
#endif

#ifndef ESP32
	// adjust internal tick counter to zero.
	_u32TickCount_ms_on_boot = 0;
	_u32TickCount_ms_on_boot = u32GetTick_ms();
#endif
}

#if defined(ESP32)
// nothing
#elif defined(_MSC_VER) || defined(__MINGW32__)
// do nothing so far (for printf.h)
extern "C" void _putchar(char character) {
	char c[2] = { character, 0 };

	OutputDebugStringA(c);
}
#elif defined(__APPLE__) || defined(__linux)
extern "C" void _putchar(char character) {
#ifdef USE_CURSES
	waddch(stdscr, character);
#else
	putchar(character);
#endif
}
#endif