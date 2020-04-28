#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_serial.hpp"

// ESP32 SUPPORT LIBS
#if defined(ESP32)
// #include <M5Stack.h> // assume always M5.

#if defined(_MSC_VER) // just for coding assist with VS (note: cannot build on VS)
#include "esp32_vs_compat.h"
#endif

namespace TWE {
	/**
	 * @brief	Writer object implementing IStereamOut to Serial console
	 */
	extern TWE::TWE_PutChar_Serial<HardwareSerial> WrtCon;

	/**
	 * @brief	Writer object implementing IStereamOut to Serial2=TWE console
	 */
	extern TWE::IStreamOutWrapper WrtTWE;
}

/** @brief	In case it has some trouble using SET pin (=GPIO5) control, set 1. */
extern uint8_t _g_twemodctl_esp32_no_set_control;
#endif	