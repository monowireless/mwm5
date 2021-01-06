#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(MWM5_BUILD_RASPI)

#include "twe_common.hpp"
#include "modctl_common.hpp"

class TweModCtlRaspi : public TweModCtlCommon {
    int _port_rst;
    int _port_pgm;
    int _port_set;

public:
	TweModCtlRaspi(int rst = 22, int pgm = 23, int set = 12) : _port_rst(rst), _port_pgm(pgm), _port_set(set) {}
	
	void setup();

	void begin();

	bool reset(bool bHold=false);

	bool setpin(bool);

	bool prog();
};
#endif