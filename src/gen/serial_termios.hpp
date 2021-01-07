#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(__APPLE__) || defined(__linux)
#include "twe_common.hpp"
#include "twe_serial.hpp"
#include "serial_common.hpp"

#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART

namespace TWE {
	class SerialTermios : public ISerial, public SerialCommon<SerialTermios> {
		friend class SerialCommon<SerialTermios>;
		template <class C, class D> friend class SerialDuo;

		using SUPER_SER = SerialCommon<SerialTermios>;

		int _fd;
        struct termios _options;

    public:
        SerialTermios(size_t bufsize = 2048) : SerialCommon(bufsize), _fd(-1), _options{} {}

	private:
        bool _open(const char* devname);

        void _close() {
			::close(_fd);
			_fd = -1;
		}

        void _flush() {
			if (SUPER_SER::is_opened()) ::tcflush(_fd, TCIFLUSH);
		}

		/**
		 * @fn	bool SerialFtdi::set_baudrate(int baud);
		 *
		 * @brief	Sets a baudrate
		 *
		 * @param	baud	The baud.
		 *
		 * @returns	True if it succeeds, false if it fails.
		 */
        bool _set_baudrate(int baud);

		/**
		 * @fn	int SerialFtdi::write(const uint8_t* p, int len)
		 *
		 * @brief	Writes bytes
		 *
		 * @param	p  	An uint8_t to process.
		 * @param	len	The length.
		 *
		 * @returns	An int. (<0:error, 0:noop, 0>:written bytes)
		 */
		int _write(const uint8_t* p, int len) {
			int len_written = 0;

			if (SUPER_SER::is_opened()) {
                len_written = ::write(_fd, (char*)p, len);
				if (_hook_on_write) _hook_on_write(p, len);
			}

			return len_written;
		}

		/**
		 * @fn	int SerialFtdi::update();
		 *
		 * @brief	peek data from the serial driver and store data into internal queue.
		 *
		 * @returns	An int. (0>: read bytes)
		 */
		int _update();

		/**
		 * @fn	TWEUTILS::SmplBuf_WChar& SerialTermios::_query_extra_deviceinfo()
		 *
		 * @brief	Queries extra deviceinfo
		 *
		 * @returns	The extra deviceinfo.
		 */
		void _query_extra_deviceinfo() {
			SUPER_SER::_devname_extra_info.clear();
		}

	private:
		/**
		 * @fn	static int SerialFtdi::list_devices()
		 *
		 * @brief	List devices, stores list to internal static array.
		 *
		 * @returns	An int.
		 */
		static int _list_devices(bool append_entry=false);


    }; // class SerialTermios
} // TWE
#endif