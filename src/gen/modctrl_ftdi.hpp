#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include "twe_common.hpp"
#include "serial_ftdi.hpp"

class TweModCtlFTDI {
	TWE::SerialFtdi& _ser_ftdi;

	void setpins(int rst, int pgm, int set);

public:
	TweModCtlFTDI(TWE::SerialFtdi& serobj) : _ser_ftdi(serobj) {}
	
	void setup() {}

	bool reset();

	bool setpin(bool);

	bool prog();
};
#endif