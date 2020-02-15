/* Copyright (/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

//#if defined(ARDUINO)
#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_sys.hpp"
#include "arduino_common.h"

using namespace TWESERCMD;

#ifdef ARDUINO

/// <summary>
/// Write object to Serial.
/// </summary>
TWE_PutChar_ARDUINO_Serial TWE::WriterToSerial(Serial);
TWE_PutChar_ARDUINO_Serial TWE::WriterToTWE(Serial2);

// do nothing so far (for printf.h)
extern "C" void _putchar(char character) {
	Serial.write(character);
}

#endif