#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include "twe_common.hpp"
#include "twe_serial.hpp"

#include <ftd2xx.h>

namespace TWE {
	class SerialFtdi : public ISerial {
		FT_STATUS _ftStatus;
		FT_HANDLE _ftHandle;
		FT_DEVICE _ftDevice;

		TWEUTILS::FixedQueue<uint8_t> _que;

		int _buf_len;
		char _buf[512];

		char _devname[32];

	public:
		// Serial Devices, global information
		static ISerial::tsAryChar32 ser_devname;
		static ISerial::tsAryChar32 ser_desc;
		static int ser_count;

		static const uint8_t BITBANG_MASK_PGM = 8;
		static const uint8_t BITBANG_MASK_RST = 4;
		static const uint8_t BITBANG_MASK_SET = 2;
		static const uint8_t BITBANG_MASK_ALL = 8 + 4 + 2;

	public:

		/**
		 * @fn	SerialFtdi::SerialFtdi(size_t bufsize = 2048)
		 *
		 * @brief	Constructor
		 *
		 * @param	bufsize	(Optional) The bufsize of internal queue.
		 */
		SerialFtdi(size_t bufsize = 2048) : _ftStatus{}, _ftHandle{}, _ftDevice{}
			, _que(TWEUTILS::FixedQueue<uint8_t>::size_type(bufsize))
			, _buf_len(0)
			, _buf{}
			, _devname{} {}


		/**
		 * @fn	bool SerialFtdi::is_opened()
		 *
		 * @brief	Query if this object is opened
		 *
		 * @returns	True if opened, false if not.
		 */
		bool is_opened() {
			return _ftHandle != NULL;
		}


		/**
		 * @fn	const char* SerialFtdi::get_devname()
		 *
		 * @brief	Gets the devname currently opened.
		 *
		 * @returns	Null if it fails, else the devname.
		 */
		const char* get_devname() {
			return _devname;
		}


		/**
		 * @fn	FT_HANDLE SerialFtdi::get_handle()
		 *
		 * @brief	Gets the handle
		 *
		 * @returns	The handle.
		 */
		FT_HANDLE get_handle() {
			return _ftHandle;
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
		bool set_baudrate(int baud);


		/**
		 * @fn	int SerialFtdi::update();
		 *
		 * @brief	peek data from the serial driver and store data into internal queue.
		 *
		 * @returns	An int.
		 */
		int update();


		/**
		 * @fn	static int SerialFtdi::_list_devices(tsAryChar32& devname, tsAryChar32& desc);
		 *
		 * @brief	List FTDI devices and storing name and descriptions.
		 *
		 * @param [in,out]	devname	The devname list
		 * @param [in,out]	desc   	The description list
		 *
		 * @returns	An int.
		 */
		static int _list_devices(tsAryChar32& devname, tsAryChar32& desc);


		/**
		 * @fn	static int SerialFtdi::list_devices()
		 *
		 * @brief	List devices, stores list to internal static array.
		 *
		 * @returns	An int.
		 */
		static int list_devices() {
			ser_count = _list_devices(ser_devname, ser_desc);
			return ser_count;
		}


		/**
		 * @fn	int SerialFtdi::_get_last_buf(int i)
		 *
		 * @brief	Gets the buffer at index i. 
		 * 			This operation does not change the queue, but to look the queue content.
		 * 			
		 *
		 * @param	i	Zero-based index of the.
		 *
		 * @returns	The last buffer.
		 */
		int _get_last_buf(int i) {
			if (i >= 0 && i < _buf_len) {
				return _buf[i];
			}

			return -1;
		}


		/**
		 * @fn	void SerialFtdi::set_bitbang(uint8_t ucmask, uint8_t ucenable)
		 *
		 * @brief	Sets a bitbang bits
		 *
		 * @param	ucmask  	The ucmask.
		 * @param	ucenable	The ucenable.
		 */
		void set_bitbang(uint8_t ucmask, uint8_t ucenable) {
			if (_ftHandle != NULL) {
				FT_SetBitMode(_ftHandle, ucmask, ucenable);
			}
		}



		/**
		 * @fn	void SerialFtdi::get_bitbang(uint8_t& mode)
		 *
		 * @brief	Gets a bitbang bitmap
		 *
		 * @param [in,out]	mode	The mode.
		 */
		void get_bitbang(uint8_t& mode) {
			if (_ftHandle != NULL) {
				FT_GetBitMode(_ftHandle, &mode);
			}
		}


		/**
		 * @fn	int SerialFtdi::read()
		 *
		 * @brief	read one byte
		 *
		 * @returns	An int. -1:error
		 */
		int read() {
			if (!_que.empty()) {
				uint8_t c = _que.front();
				_que.pop();

				return c;
			}

			return -1;
		}


		/**
		 * @fn	int SerialFtdi::write(const uint8_t* p, int len)
		 *
		 * @brief	Writes bytes
		 *
		 * @param	p  	An uint8_t to process.
		 * @param	len	The length.
		 *
		 * @returns	An int.
		 */
		int write(const uint8_t* p, int len) {
			DWORD len_written = 0;
			if (_ftHandle != NULL) {
				FT_Write(_ftHandle, (LPVOID)p, (DWORD)len, &len_written);
			}

			return len_written;
		}


		/**
		 * @fn	bool SerialFtdi::open(const char* devname);
		 *
		 * @brief	Opens the given devname
		 *
		 * @param	devname	The devname.
		 *
		 * @returns	True if it succeeds, false if it fails.
		 */
		bool open(const char* devname);


		/**
		 * @fn	operator SerialFtdi::bool()
		 *
		 * @brief	True if it's opened.
		 *
		 * @returns	The result of the operation.
		 */
		operator bool() {
			return is_opened();
		}


		/**
		 * @fn	void SerialFtdi::setTimeout(int time_ms = 1000)
		 *
		 * @brief	Sets a timeout
		 *
		 * @param	time_ms	(Optional) The time in milliseconds.
		 */
		void setTimeout(int time_ms = 1000) {
			if (_ftHandle != NULL) {
				FT_SetTimeouts(_ftHandle, time_ms, 0);
			}
		}


		/**
		 * @fn	void SerialFtdi::close()
		 *
		 * @brief	Closes the device
		 */
		void close() {
			if (_ftHandle != NULL) {
				FT_Close(_ftHandle);

				_ftHandle = NULL;
				_devname[0] = 0;
			}
		}



		/**
		 * @fn	void SerialFtdi::flush()
		 *
		 * @brief	Flushes this object
		 */
		void flush() {
			if (_ftHandle != NULL) {
				delay(32);
			}
		}



		/**
		 * @fn	bool SerialFtdi::available()
		 *
		 * @brief	if data is in the internal queue, returns true.
		 *
		 * @returns	True if the queue has some data, false if it's empty.
		 */
		bool available() {
			return !_que.empty();
		}


		/**
		 * @fn	void SerialFtdi::begin(uint32_t baud)
		 *
		 * @brief	begins the device with baud rate passed as an argument.
		 *
		 * @param	baud	The baud.
		 */
		void begin(uint32_t baud) {
			if (_ftHandle != NULL) {
				flush();
				FT_SetBaudRate(_ftHandle, baud);
			}
		}

		// do nothing
		void begin(int, int, int, int) {}
		void setRxBufferSize(int i) {} 
		void printf(const char *fmt, ...) {}
	};
}

#endif //WIN/MAC