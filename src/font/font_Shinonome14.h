/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */
#pragma once 

#include "twe_common.hpp"
#include "twe_font.hpp"

namespace TWEFONT {
	const FontDef& createFontShinonome14_mini(uint8_t id, uint8_t line_space = 0, uint8_t char_space = 0, uint32_t u32Opt = 0);
	const FontDef& createFontShinonome14_std(uint8_t id, uint8_t line_space = 0, uint8_t char_space = 0, uint32_t u32Opt = 0);
	const FontDef& createFontShinonome14_full(uint8_t id, uint8_t line_space = 0, uint8_t char_space = 0, uint32_t u32Opt = 0);
	static inline const FontDef& createFontShinonome14(uint8_t id, uint8_t line_space = 0, uint8_t char_space = 0, uint32_t u32Opt = 0) {
			return createFontShinonome14_std(id, line_space, char_space, u32Opt); }
}
