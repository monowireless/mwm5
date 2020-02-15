#pragma once

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

// include all headers
#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_utils_fixedque.hpp"
#include "twe_stream.hpp"
#include "twe_sercmd.hpp"
#include "twe_sercmd_ascii.hpp"
#include "twe_sercmd_binary.hpp"
#include "twe_console.hpp"
#include "twe_printf.hpp"
#include "twe_fmt.hpp"
#include "twe_sys.hpp"

#ifdef USE_CSETTINGS
#include "twe_csettings.hpp"
#endif

// include arch specific defs
#if defined (ARDUINO)
#include "ESP32/arduino_common.h"
#include "ESP32/arduino_lcdconsole.hpp"
#include "twe_font.hpp"
#include "font/arduino_lcd_font_8x6.h"
#include "font/arduino_lcd_font_MP10.h"
#include "font/arduino_lcd_font_MP12.h"
#include "font/arduino_lcd_font_shinonome16.h"
#include "font/arduino_lcd_font_shinonome14.h"
#include "font/arduino_lcd_font_shinonome12.h"
#endif