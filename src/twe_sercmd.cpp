/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_sercmd.hpp"

#include "twe_utils_fixedque.hpp"

 // uart input queue
TWEUTILS::InputQueue<uint8_t> the_uart_queue(512);

// nothing implemented so far