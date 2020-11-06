#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include "twe_common.hpp"

template <class SDUO, class C, class D>
class TweModCtlDuo {
    SDUO& _sduo;
    C& _c;
    D& _d;

public:
	TweModCtlDuo(SDUO& sduo, C& c, D& d) : _sduo(sduo), _c(c), _d(d) {}
	
	void setup() {
        // initialize both of mods (called only once)
        _c.setup();
        _d.setup();
    }

	bool reset(bool bHold=false) {
        if(_sduo.is_opened()) {
            if(_sduo.get_cur_obj_id() == 0) { return _c.reset(bHold); }
            else { return _d.reset(bHold); }
        }
        return false;
    }

	bool setpin(bool b) {
        if(_sduo.is_opened()) {
            if(_sduo.get_cur_obj_id() == 0) { return _c.setpin(b); }
            else { return _d.setpin(b); }
        }
        return false;
    }

	bool prog() {
        if(_sduo.is_opened()) {
            if(_sduo.get_cur_obj_id() == 0) { return _c.prog(); }
            else { return _d.prog(); }
        }
        return false;
    }
};
#endif