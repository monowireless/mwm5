#pragma once

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_serial.hpp"

namespace TWE {
    class SerialPortEntries {
	protected:
        static int _idx_C_end;
        static int _idx_D_end;
    public:
        static ISerial::tsAryChar32 ser_devname;
		static ISerial::tsAryChar32 ser_desc;
		static int ser_count;

    public:
        SerialPortEntries() {}
    };

    template <class CDER>
    class SerialCommon : public SerialPortEntries {
		friend CDER;

    protected:
        static const int SIZ_READ_BUFF = 512;
        static const int SIZ_DEV_NAME = 32;

        char _devname[SIZ_DEV_NAME];

        TWEUTILS::FixedQueue<uint8_t> _que; // primary buffer (used when read() is called.)

		char _buf[SIZ_READ_BUFF]; // secondary buffer (used for system API.)
		int _buf_len; // size of effective data in _buf[]

        void (*_hook_on_write)(const uint8_t* p, int len); // hook function when writing.

        int _session_id; // -1:no_session or positive value
    
    public:
        SerialCommon(size_t bufsize = 2048) : 
              _devname{}
            , _que(TWEUTILS::FixedQueue<uint8_t>::size_type(bufsize))
            , _buf{}
            , _buf_len(0)
            , _hook_on_write(nullptr)
            , _session_id(-1)
        {}
    
    	/**
		 * @fn	void SerialCommon::begin(uint32_t baud)
		 *
		 * @brief	begins the device with baud rate passed as an argument.
		 *
		 * @param	baud	The baud.
		 */
		void begin(uint32_t baud) {
            auto p = static_cast<CDER*>(this);

			if (p->is_opened()) {
                // flush() and change baud.
                p->flush();
				p->set_baudrate(baud);
            }
		}

        bool open(const char* devname) {
            return static_cast<CDER&>(*this)._open(devname);
        }

        void close() { 
            static_cast<CDER&>(*this)._close();
            
			_session_id = -1;
			_devname[0] = 0;
        }

        void flush() {
            static_cast<CDER&>(*this)._flush();
        }

		bool is_opened() { return _session_id != -1; }

        bool set_baudrate(int baud) {
            return static_cast<CDER&>(*this)._set_baudrate(baud);
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
		 * @fn	int SerialCommon::get_handle()
		 *
		 * @brief	Gets the handle
		 *
		 * @returns	The handle. (-1:not open, other session id)
		 */
		int get_handle() {
			return _session_id;
		}

        /**
		 * @fn	int SerialCommon::_get_last_buf(int i)
		 *
		 * @brief	Gets the buffer at index i. 
		 * 			This operation does not change the queue, but to look the queue content.
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
		 * @fn	bool SerialCommon::available()
		 *
		 * @brief	if data is in the internal queue, returns true.
		 *
		 * @returns	True if the queue has some data, false if it's empty.
		 */
		bool available() {
			return !_que.empty();
		}

        /**
		 * @fn	int SerialCommon::read()
		 *
		 * @brief	read one byte (from _que)
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
		 * @fn	int SerialCommon::write(const uint8_t* p, int len)
		 *
		 * @brief	Writes bytes
		 *
		 * @param	p  	An uint8_t to process.
		 * @param	len	The length.
		 *
		 * @returns	An int. (<0:error, 0:noop, 0>:written bytes)
		 */
		int write(const uint8_t* p, int len) {
			int len_written = 0;

			if (is_opened()) {
                len_written = static_cast<CDER&>(*this)._write(p, len);
				if (_hook_on_write) _hook_on_write(p, len);
			}

			return len_written;
		}
        
		/**
		 * @fn	int SerialCommon::update();
		 *
		 * @brief	peek data from the serial driver and store data into internal queue.
		 *
		 * @returns	An int. (0>: read bytes)
		 */
		int update() {
            return static_cast<CDER&>(*this)._update();
        }

		/**
		 * @fn	void SerialCommon::set_hook_on_write(void (*ptr)(char_t))
		 *
		 * @brief	Sets hook on write
		 *
		 * @param [in,out]	ptr	If non-null, the pointer.
		 */
		void set_hook_on_write(void (*ptr)(const uint8_t* p, int len)) {
			_hook_on_write = ptr;
		}

	public:
		// do nothing (for API compat.)
		void begin(int, int, int, int) {}
		void setRxBufferSize(int i) {} 
		void printf(const char *fmt, ...) {}

	public:
		/**
		 * @fn	static int SerialFtdi::list_devices()
		 *
		 * @brief	List devices, stores list to internal static array.
		 *
		 * @returns	An int.
		 */
		static int list_devices(bool append_entry=false) {
			return CDER::_list_devices(append_entry);
		}
    };
}