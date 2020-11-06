#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include "twe_common.hpp"
#include "twe_serial.hpp"
#include "serial_common.hpp"

#include <ftd2xx.h>

namespace TWE {
	class SerialFtdi : public ISerial, public SerialCommon<SerialFtdi> {
		friend class SerialCommon<SerialFtdi>;
		template <class C, class D> friend class SerialDuo;

		FT_STATUS _ftStatus;
		FT_HANDLE _ftHandle;
		FT_DEVICE _ftDevice;

	public:
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
		SerialFtdi(size_t bufsize = 2048) : SerialCommon(bufsize), _ftStatus{}, _ftHandle{}, _ftDevice{} {}

	public: //TODO shou;ld be PRIVATE
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
		 * @fn	int SerialFtdi::update();
		 *
		 * @brief	peek data from the serial driver and store data into internal queue.
		 *
		 * @returns	An int.
		 */
		int _update();


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
		int _write(const uint8_t* p, int len) {
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
		bool _open(const char* devname);


		/**
		 * @fn	void SerialFtdi::close()
		 *
		 * @brief	Closes the device
		 */
		void _close() {
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
		void _flush() {
			if (_ftHandle != NULL) {
				delay(32); // TODO: it's not concrete way...
			}
		}

	public:		
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

#if 0
	public:
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
#endif

	private:
		/**
		 * @fn	static int SerialFtdi::_list_devices(bool append_entry=false);
		 *
		 * @brief	List FTDI devices and storing name and descriptions.
		 *
		 * @returns	An int.
		 */
		static int _list_devices(bool append_entry=false);
	};
}

#endif //WIN/MAC