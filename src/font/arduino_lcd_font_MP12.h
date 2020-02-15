#pragma once 

#include "twe_common.hpp"
#include "twe_font.hpp"

#ifdef ARDUINO
namespace TWEFONT {
	const FontDef& createFontMP12(uint8_t id, uint8_t line_space = 0, uint8_t char_space = 0, uint32_t u32Opt = 0);
}
#endif