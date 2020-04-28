#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "esp32_lcd_color.h"

namespace TWEARD {
	union RGBA {
		uint32_t u32col;
		uint8_t u8col[4];
	};

	struct Rect {
		int16_t x;
		int16_t y;
		uint16_t w;
		uint16_t h;
	};

	constexpr uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
		return (r == 0xFF && g == 0xFF && b == 0xFF) ?
			ALMOST_WHITE : ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
	}

	constexpr uint16_t color565(uint32_t rgb) {
		return color565((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, (rgb & 0xFF));
	}

	static inline RGBA color565toRGBA(uint16_t c) {
		RGBA c32 = { 0x00UL };

		c32.u8col[0] = (c >> 8) & 0xF8;
		c32.u8col[1] = (c >> 3) & 0xFC;
		c32.u8col[2] = (c << 3) & 0xF8;
		c32.u8col[3] = 0xFF;

		return c32;
	}
}