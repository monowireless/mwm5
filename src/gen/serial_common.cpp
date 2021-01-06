/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include "twe_serial.hpp"
#include "serial_common.hpp"
#include <filesystem>

using namespace TWE;

// Serial Devices
TWE::ISerial::tsAryChar32 TWE::SerialPortEntries::ser_devname(TWE::SerialPortEntries::SERPORT_ENT_MAX);
TWE::ISerial::tsAryChar32 TWE::SerialPortEntries::ser_desc(TWE::SerialPortEntries::SERPORT_ENT_MAX);
uint8_t TWE::SerialPortEntries::ser_modctl_mode[TWE::SerialPortEntries::SERPORT_ENT_MAX] = { 0 };
int TWE::SerialPortEntries::ser_count = 0;
int TWE::SerialPortEntries::_idx_C_end = 0;
int TWE::SerialPortEntries::_idx_D_end = 0;

#endif