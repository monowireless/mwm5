#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"

extern const char pix_icon_win_32[32*32*2+1];
extern const char pix_icon_win_128[128*128*2+1];

extern void prepare_icon_data(uint32_t* u32pixmap, int w, int h, const char* pixdata);