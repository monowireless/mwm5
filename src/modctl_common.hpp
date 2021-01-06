#pragma once

/* Copyright (C) 2021 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */


#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include "twe_common.hpp"
class TweModCtlCommon {
	uint8_t _mode; // 1: enabled to control MOD
	uint8_t _capable; // 1: capable to control MOD
	
public:
	TweModCtlCommon() : _mode(1), _capable(1) {}

	void _set_capable(bool m) { _capable = m; } // set by TweModCtl::begin()
	void set_enabled(bool m) { _mode = m; }     // enabling module control (if it's capable)
	bool get_enabled() { return _mode && _capable;  } // true: okay to control module.
};

#endif