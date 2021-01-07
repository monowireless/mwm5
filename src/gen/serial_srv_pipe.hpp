#pragma once

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

//#if !defined(MWM5_SERIAL_NO_FTDI) && (defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__))
#if defined(__APPLE__) // && defined(MWM5_SERIAL_NO_FTDI) 

#include "twe_common.hpp"
#include "twe_serial.hpp"
#include "serial_common.hpp"
#include "twe_file.hpp"
#include "twe_sercmd.hpp"
#include "twe_sercmd_ascii.hpp"
#include "twe_utils_simplebuffer.hpp"

namespace TWE {
	class SerialSrvPipe : public ISerial, public SerialCommon<SerialSrvPipe> {
		friend class SerialCommon<SerialSrvPipe>;
		using SUPER_SER = SerialCommon<SerialSrvPipe>;
		template <class C, class D> friend class SerialDuo;
	
        TweCmdPipeInOut _pipe;

        TWESERCMD::AsciiParser parse_ascii;
        TWESERCMD::AsciiParser format_ascii;

		SERSRV_ESC esc_handler;
		bool b_esc_commands;

	public:

		/**
		 * @fn	SerialFtdi::SerialFtdi(size_t bufsize = 2048)
		 *
		 * @brief	Constructor
		 *
		 * @param	bufsize	(Optional) The bufsize of internal queue.
		 */
		SerialSrvPipe(const char* srv_cmd, size_t bufsize = 2048) : 
                SerialCommon(bufsize), _pipe(nullptr),
                parse_ascii(512), format_ascii(512),
				b_esc_commands(false)
        {}

		// destructor (close command)
		~SerialSrvPipe() {
			close_sersrv();
		}

		// Open the pipe command
		bool open_sersrv(const char*cmd) {
			return _pipe.begin(cmd);
		}

		// Close the pipe command
		void close_sersrv() {
			_pipe.end();
		}

	public: //TODO should be PRIVATE
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
		 * @returns	byte count of reading from pipe.
		 */
		int _update();

		// peek data from serial from ESCIN to ESCOUT.
		bool _wait_respond();

		// send a command
		void _send_cmd(uint8_t *b, uint8_t *e);

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
		int _write(const uint8_t* p, int len);

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
		void _close();

		/**
		 * @fn	void SerialFtdi::flush()
		 *
		 * @brief	Flushes this object
		 */
		void _flush() {
			delay(32); // TODO: it's not concrete way...
		}

		/**
		 * @fn	TWEUTILS::SmplBuf_WChar& SerialSrvPipe::query_extra_deviceinfo()
		 *
		 * @brief	Queries extra deviceinfo
		 *
		 * @returns	The extra deviceinfo.
		 */
		void _query_extra_deviceinfo() {
			SUPER_SER::_devname_extra_info.clear();
		}

	public:
		bool modctl_reset(bool bHold=false);

		bool modctl_setpin(bool bSetPin);

		bool modctl_prog();

	private:
		/**
		 * @fn	static int SerialFtdi::_list_devices(bool append_entry=false);
		 *
		 * @brief	List FTDI devices and storing name and descriptions.
		 *
		 * @returns	An int.
		 */
		int _list_devices(bool append_entry=false);
	};

	class TweModCtlSerialSrvPipe {
		TWE::SerialSrvPipe& _ser_srv;
	public:
		TweModCtlSerialSrvPipe(TWE::SerialSrvPipe& serobj) : _ser_srv(serobj) {}
		void setup() {}
		bool reset(bool bHold=false) { return _ser_srv.modctl_reset(bHold); }
		bool setpin(bool bSetPin) { return _ser_srv.modctl_setpin(bSetPin); }
		bool prog() { return _ser_srv.modctl_prog(); }
	};
}

#endif //WIN/MAC