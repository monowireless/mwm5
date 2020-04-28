/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#ifdef ESP32
#include <cstring>
#include <driver/uart.h>
#include <freertos/queue.h>

#include "twe_serial.hpp"
#include "twe_firmprog.hpp"

#include "esp32_uart.hpp"
#include "esp32_modctrl.hpp"

#include "esp32_common.h"

using namespace TWE;

// instance of SerialTWE
TWE::SerialESP32 TWE::Serial2_IDF;
extern void _gen_modctrl_for_esp32_uart();

void SerialESP32::begin(uint32_t baud, uint16_t buff_rx, uint16_t buff_tx) {
	if (!_opened) {
		uart_config_t config;

		memset(&config, 0, sizeof(uart_config_t));
		config.baud_rate = baud;
		config.data_bits = UART_DATA_8_BITS;
		config.parity = UART_PARITY_DISABLE;
		config.stop_bits = UART_STOP_BITS_1;
		config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
		//config.use_ref_tick = true;

		uart_param_config(_uart_num, &config);

		// Install UART driver using an event queue here	
		uart_set_pin(_uart_num, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
		uart_driver_install(_uart_num, buff_rx, buff_tx, 10, &_uart_queue, 0);
		//uart_driver_install(_uart_num, uart_buffer_size, uart_buffer_size, 0, nullptr, 0);
		//uart_set_mode(_uart_num, UART_MODE_UART);

		// recreate new instance
		WrtTWE.reset(new TWE_PutChar_Serial<SerialESP32>(*this)); // switch object
		_gen_modctrl_for_esp32_uart();

		_opened = true;
	}
	else {
		// if opened, set baudrate again.
		uart_set_baudrate(_uart_num, baud);
	}
}
#endif