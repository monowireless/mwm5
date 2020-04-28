/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include "twe_common.hpp"
#include "twe_serial.hpp"

#include "modctrl_ftdi.hpp"

using namespace TWE;

void TweModCtlFTDI::setpins(int rst, int pgm, int set) {
	if (_ser_ftdi.is_opened()) {
		uint8_t u8pin;

		_ser_ftdi.get_bitbang(u8pin);
		u8pin = ~u8pin; // now Lo=1 for PGM/RST.
		u8pin &= SerialFtdi::BITBANG_MASK_ALL; // set others to 0

		if (rst >= 0) {
			u8pin &= ~SerialFtdi::BITBANG_MASK_RST; // clear the bit
			u8pin |= rst ? SerialFtdi::BITBANG_MASK_RST : 0; // set the bit
		}

		if (pgm >= 0) {
			u8pin &= ~SerialFtdi::BITBANG_MASK_PGM; // clear the bit
			u8pin |= pgm ? SerialFtdi::BITBANG_MASK_PGM : 0; // set the bit
		}

		if (set >= 0) {
			u8pin &= ~SerialFtdi::BITBANG_MASK_SET; // clear the bit
			u8pin |= set ? SerialFtdi::BITBANG_MASK_SET : 0; // set the bit
		}

		u8pin = ~u8pin;
		_ser_ftdi.set_bitbang(u8pin, 0x20);
	}
}

bool TweModCtlFTDI::reset() {
	if (_ser_ftdi.is_opened()) {
		// _ser_ftdi.set_bitbang(0xFB, 0x20);
		setpins(1, 0, -1);

		delay(20);
		
		//_ser_ftdi.set_bitbang(0xFF, 0x20);
		setpins(0, 0, -1);

		return true;
	}
	return false;
}

bool TweModCtlFTDI::prog() {
	if (_ser_ftdi.is_opened()) {
		// _ser_ftdi.set_bitbang(0xF3, 0x20);
		setpins(1, 1, 0);
		delay(50);

		// _ser_ftdi.set_bitbang(0xF7, 0x20);
		setpins(0, 1, 0);
		delay(200); //delay(40);

		// _ser_ftdi.set_bitbang(0xFF, 0x20);
		setpins(0, 0, 0);

		return true;
	}
	else {
		return false;
	}
}

bool TweModCtlFTDI::setpin(bool bSet) {
	if (_ser_ftdi.is_opened()) {
		setpins(-1, -1, bSet ? 1 : 0);
		return true;
	}
	return false;
}

#endif