#pragma once

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

#include "twe_common.hpp"

// ARDUINO SUPPORT LIBS
#if !defined(ARDUINO) // NON ARDUINO (skip this code)
//#error "ARDUINO speficic include, it shoud not be."
#else // ARDUINO
#ifndef _MSC_VER // just for edit with VS.
#include <Arduino.h>
#include <M5Stack.h> // assume always M5.
#ifndef ESP32
# error "only assume ESP32."
#endif
#else
#include "arduino_vs_compat.h"
#endif

namespace TWESERCMD {
	/// <summary>
	/// Output to Serial via IStreamOut interface.
	/// </summary>
	class TWE_PutChar_ARDUINO_Serial : public TWE::IStreamOut {
		HardwareSerial& _ser;
	public:
		//TWE_PutChar_ARDUINO_Serial() {}
		TWE_PutChar_ARDUINO_Serial(HardwareSerial& ser) : _ser(ser) {}
		inline IStreamOut& operator ()(char_t c) { _ser.write(c); return (*this); }
	};
}

namespace TWE {
	// has instance
	extern TWESERCMD::TWE_PutChar_ARDUINO_Serial WriterToSerial;
	extern TWESERCMD::TWE_PutChar_ARDUINO_Serial WriterToTWE;
}

#endif	