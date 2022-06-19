/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_sercmd.hpp"

#include "twe_utils_fixedque.hpp"

 // uart input queue
#ifndef ESP32
TWEUTILS::InputQueue<uint8_t, true> the_uart_queue(4096);
#else
TWEUTILS::InputQueue<uint8_t, false> the_uart_queue(512);
#endif

// nothing implemented so far