#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

// include all headers
#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_utils_unicode.hpp"
#include "twe_utils_fixedque.hpp"
#include "twe_stream.hpp"
#include "twe_sercmd.hpp"
#include "twe_sercmd_ascii.hpp"
#include "twe_sercmd_binary.hpp"
#include "twe_console.hpp"
#include "twe_printf.hpp"
#include "twe_fmt.hpp"
#include "twe_serial.hpp"
#include "twe_firmprog.hpp"
#include "twe_sys.hpp"
#include "twe_file.hpp"
#include "twe_stgsmenu.hpp"

// settings library
#include "twe_csettings.hpp"

// application definitions
#include "twe_appdefs.hpp"

// user interface
#include "twe_cui_keyboard.hpp"
#include "twe_cui_widget.hpp"
#include "twe_cui_listview.hpp"
#include "twe_cui_button.hpp"
#include "twe_cui_tabs.hpp"

// font defs
#include "twe_font.hpp"

// include arch specific defs
#include "esp32/esp32_common.h"
#include "esp32/esp32_uart.hpp"

#include "esp32/esp32_lcdconsole.hpp"
#include "esp32/generic_lcd_common.h"
#include "esp32/generic_lcd_screen.hpp"

#include "font/lcd_font_8x6.h"
#include "font/font_MP10.h"
#include "font/font_MP12.h"
#include "font/font_Shinonome16.h"
#include "font/font_Shinonome14.h"
#include "font/font_Shinonome12.h"

// for PC/Win/Linux
#include "gen/sdl2_clipboard.hpp"

// misc library
#include "oss/oss_regex.h"
