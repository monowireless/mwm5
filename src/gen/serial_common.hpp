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
		char _devname_prev[SIZ_DEV_NAME];

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
			bool ret = static_cast<CDER&>(*this)._open(devname);
			if (ret) {
#if defined(_MSC_VER) || defined(__MINGW32__)
				strncpy_s(_devname_prev, _devname, sizeof(_devname_prev));
#elif defined(__APPLE__) || defined(__linux)
				strncpy(_devname_prev, _devname, sizeof(_devname_prev));
#endif
			}
			return ret;
        }

        void close() { 
			static_cast<CDER&>(*this)._close();
			_session_id = -1;
			_devname[0] = 0; // clear _devname as well
        }

		bool reopen() {
			if (is_opened()) {
				close();
			}

			bool ret = false;
			if (_devname_prev[0] != 0)
				ret = open(_devname_prev);

			return ret;
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
		int list_devices(bool append_entry=false) {
			//return CDER::_list_devices(append_entry);
			return static_cast<CDER&>(*this)._list_devices(append_entry);
		}
    };

	// empty class of Serial (for test w/o serial driver)
	class SerialDummy : public ISerial, public SerialCommon<SerialDummy> {
		friend class SerialCommon<SerialDummy>;
		using SUPER_SER = SerialCommon<SerialDummy>;

    public:
        SerialDummy(size_t bufsize = 2048) : SerialCommon(bufsize) {}

	private:
        bool _open(const char* devname) {return true;}
        void _close() {}
        void _flush() {}
        bool _set_baudrate(int baud) {return true;}
		int _write(const uint8_t* p, int len) { return len; }
		int _update() { return 0; }

	private:
		static int _list_devices(bool append_entry=false) { return 0; }
    }; // class SerialDummy

	// empty class of TweModCtl (for test w/o serial driver)
	class TweModCtlDummy {
	public:
		TweModCtlDummy() {}
		void setup() {}
		bool reset(bool bHold=false) { return true; }
		bool setpin(bool) { return true; }
		bool prog() { return true; }
	};

	// use for communication between serial_srv.
	class SERSRV_ESC {
		uint8_t _chr_save[4];
		uint8_t _chr_save_ct;
		int8_t _state;

		void push_chr(uint8_t c) {
			_chr_save_ct++;
			_chr_save[sizeof(_chr_save) - _chr_save_ct] = c;
		}

		int pop_chr() {
			int c = -1;
			if (_chr_save_ct > 0) {
				c = _chr_save[sizeof(_chr_save) - _chr_save_ct];
				_chr_save_ct--;
			}
			if (_chr_save_ct == 0) {
				_state = 0;
			}
			return c;
		}

		void clear_chr() {
			_chr_save_ct = 0;
		}

	public:
		static const int E_STATE_WAIT = -1;
		static const int E_STATE_ESC_IN = -2;
		static const int E_STATE_ESC_OUT = -3;

		static const int E_STATE_READ = -4;
	public:
		SERSRV_ESC() : _chr_save{}, _chr_save_ct(0), _state(0) {}

		int operator()(int c) {
			switch (_state) {
				case 0:
					if (c == 0xC2) {
						clear_chr();
						_state = E_STATE_WAIT; // wait for next char
						push_chr(c);
					} else {
						_state = E_STATE_READ;
						push_chr(c);
					}
				break;

				case E_STATE_WAIT:
					if (c == 0xC2) { // double 0xC2 -> 0xC2
						_state = E_STATE_READ;
					} else
					if (c == 0xAB) {
						_state = 0;
						clear_chr();
						return E_STATE_ESC_IN;
					} else
					if (c == 0xBB) {
						_state = 0;
						clear_chr();
						return E_STATE_ESC_OUT;
					} else {
						_state = E_STATE_READ;
						push_chr(c);
					}
				break;

				default:
					clear_chr();
			}

			return (int)_state;
		}
		bool available() {
			return (_state == E_STATE_READ); 
		}
		int read() {
			return pop_chr();
		}
	};

	struct SERSRV_CMD {
		static const uint8_t LIST_DEVS = 0;
		static const uint8_t OPEN = 1;
		static const uint8_t CLOSE = 2;
		static const uint8_t SET_BAUD = 4;
		static const uint8_t MODCTL_RESET = 0x11;
		static const uint8_t MODCTL_PROG = 0x12;
		static const uint8_t MODCTL_SETPIN = 0x13;
	};
}