#pragma once

#include "twe_common.hpp"
#include "twe_font.hpp"
namespace TWEFONT {
	const FontDef& createFontLcd8x6(uint8_t id, uint8_t line_space = 0, uint8_t char_space = 0, uint32_t u32Opt = 0);
}