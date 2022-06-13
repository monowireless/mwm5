/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_sys.hpp"

#include <chrono>

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

uint64_t TWESYS::TweLocalTime::now() {
	// get YYYY MM DD hh mm ss
	tm localTime;
	std::chrono::system_clock::time_point t = std::chrono::system_clock::now();
	epoch = std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count();

	time_t now = std::chrono::system_clock::to_time_t(t);
#if defined(_MSC_VER)
	localtime_s(&localTime, &now);
#else
	localtime_r(&now, &localTime);
#endif

	// get millisec part (by C++11)
	const std::chrono::duration<double> tse = t.time_since_epoch();
	std::chrono::seconds::rep milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(tse).count() % 1000;

	// convert into text
	year = (1900 + localTime.tm_year);
	month = (localTime.tm_mon + 1);
	day = localTime.tm_mday;
	hour = localTime.tm_hour;
	minute = localTime.tm_min;
	second = localTime.tm_sec;
	ms = uint16_t(milliseconds);

	return epoch;
}

void TWESYS::TweLocalTime::set_epoch(uint64_t ep) {
	// get YYYY MM DD hh mm ss
	tm localTime;
	time_t t = (time_t)ep;

#if defined(_MSC_VER)
	localtime_s(&localTime, &t);
#else
	localtime_r(&t, &localTime);
#endif

	// convert into text
	year = (1900 + localTime.tm_year);
	month = (localTime.tm_mon + 1);
	day = localTime.tm_mday;
	hour = localTime.tm_hour;
	minute = localTime.tm_min;
	second = localTime.tm_sec;

	ms = 0;
}

void TWESYS::TweLocalTime::get_epoch() {
	tm lt;
	memset(&lt, 0, sizeof(tm));

	lt.tm_year = year - 1900;
	lt.tm_mon = month - 1;
	lt.tm_mday = day;
	lt.tm_hour = hour;
	lt.tm_min = minute;
	lt.tm_sec = second;

	// mktime converts from localtime.
	time_t ep_lt = mktime(&lt);

#if 0
	// timezon adjustment
	{
		std::tm m;
#if defined(_MSC_VER)
		gmtime_s(&m, &ep_lt);
#else
		gmtime_r(&ep_lt, &m);
#endif
		std::time_t n = std::mktime(&m);
		auto diff = ep_lt - n; // 32400(=+9*3600) in GMT+9.
	}
#endif

	// convert into epoch.
	epoch = static_cast<uint64_t>(static_cast<std::chrono::seconds>(ep_lt).count());
}

uint64_t TWESYS::TweLocalTime::epoch_now() {
	// get YYYY MM DD hh mm ss
	std::chrono::system_clock::time_point t = std::chrono::system_clock::now();
	return std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count();
}