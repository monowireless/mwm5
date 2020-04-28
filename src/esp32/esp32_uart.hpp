#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#ifdef ESP32
#include "twe_common.hpp"
#include "twe_serial.hpp"

#include <driver/uart.h>
#include <freertos/queue.h>

namespace TWE {
	class SerialESP32 : public ISerial {
		const uart_port_t _uart_num;
		bool _opened;
		QueueHandle_t _uart_queue;

	public:
		SerialESP32() 
			: _opened(false)
			, _uart_num(UART_NUM_2)
			, _uart_queue(nullptr) {}
		~SerialESP32() {}

		/**
		 * @fn	void SerialESP32::begin(uint32_t baud, uint16_t buff_rx = 1024, uint16_t buff_tx = 1024);
		 *
		 * @brief	Begins
		 *
		 * @param	baud   	The baud.
		 * @param	buff_rx	(Optional) The buffer receive. (only effective at the first call)
		 * @param	buff_tx	(Optional) The buffer transmit. (only effective at the first call)
		 */
		void begin(uint32_t baud, uint16_t buff_rx = 1024, uint16_t buff_tx = 1024);
		
		inline int read() {
			uint8_t buf[1];
			int n = uart_read_bytes(_uart_num, buf, 1, 0);
			return n == 1 ? buf[0] : -1;
		}

		inline int available() {
			size_t n_avail;
			uart_get_buffered_data_len(_uart_num, &n_avail);
			return (int)n_avail;
		}

		int write(const uint8_t* p, int len) { return uart_write_bytes(_uart_num, (const char*)p, len); }
		int write(const char* p, int len) { return write((uint8_t*)p, len); }
		void flush() { uart_wait_tx_done(_uart_num, 100); }

		inline bool is_opened() { return _opened; }
		operator bool() { return is_opened(); }
	};

	extern SerialESP32 Serial2_IDF;
}
#endif
