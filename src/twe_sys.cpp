/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

#include "twe_common.hpp"
#include "twe_sys.hpp"

#ifdef _MSC_VER
#include <Windows.h>
extern "C" volatile uint32_t u32TickCount_ms;
#elif defined(ARDUINO)
#include <Arduino.h>
#endif

uint32_t TWESYS::u32GetTick_ms() {
#ifdef _MSC_VER
	return u32TickCount_ms;
#elif defined(ARDUINO)
	return millis();
#endif
}

void TWESYS::Sleep_ms(int ms) {
#ifdef _MSC_VER
	Sleep(ms);
#elif defined(ARDUINO)
	delay(ms);
#endif
}

