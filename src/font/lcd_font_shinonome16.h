/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */
#pragma once 

#include "twe_common.hpp"
#include "twe_font.hpp"

namespace TWEFONT {
	const FontDef& createFontShinonome16(uint8_t id, uint8_t line_space = 0, uint8_t char_space = 0, uint32_t u32Opt = 0);
	const FontDef& createFontShinonome16_std(uint8_t id, uint8_t line_space = 0, uint8_t char_space = 0, uint32_t u32Opt = 0);
	const FontDef& createFontShinonome16_full(uint8_t id, uint8_t line_space = 0, uint8_t char_space = 0, uint32_t u32Opt = 0);
}
