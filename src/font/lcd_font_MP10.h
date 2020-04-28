/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

#pragma once 

#include "twe_common.hpp"
#include "twe_font.hpp"

namespace TWEFONT {
	const FontDef& createFontMP10(uint8_t id, uint8_t line_space = 0, uint8_t char_space = 0, uint32_t u32Opt = 0);
}
