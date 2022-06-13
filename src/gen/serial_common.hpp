#pragma once

/* Copyright (C) 2020-2022 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_serial.hpp"

#include "sdl2_config.h"
# include "sdl2_utils.hpp" 

namespace TWE {
    class SerialPortEntries {
	protected:
        static int _idx_C_end;
        static int _idx_D_end;

    public:
		const static int SERPORT_ENT_MAX = 8;
        static ISerial::tsAryChar32 ser_devname;
		static ISerial::tsAryChar32 ser_desc;
		static uint8_t ser_modctl_mode[SERPORT_ENT_MAX]; // 0: no capability 1: serial port has capability
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
		TWEUTILS::SmplBuf_WCharSL<64> _devname_extra_info;
		int _devname_opened_idx;

        TWEUTILS::FixedQueue<uint8_t> _que; // primary buffer (used when read() is called.)

		char _buf[SIZ_READ_BUFF]; // secondary buffer (used for system API.)
		int _buf_len; // size of effective data in _buf[]

        void (*_hook_on_write)(const uint8_t* p, int len); // hook function when writing.

        int _session_id; // -1:no_session or positive value

		uint8_t _modctl_capable;
						   // modctl capability
						   //   0: no capability (e.g. FTDI bitbang does not work)
		                   //   1: enabled (e.g. FTDI bitbang is enabled)
		                   //   2: depends on modctl implementation (e.g. /dev/serial)
		
		// MUTEX
#if MWM5_SDL2_USE_MULTITHREAD_RENDER == 1 && MWM5_USE_SDL2_MUTEX == 1
		SDL_mutex* _mtx;
#elif MWM5_SDL2_USE_MULTITHREAD_RENDER == 1 && MWM5_USE_SDL2_MUTEX == 0
		std::mutex _mtx;
#else
		const bool _mtx = true;
#endif

    public:
        SerialCommon(size_t bufsize = 2048) : 
			  _devname(), _devname_prev(), _devname_extra_info()
			, _devname_opened_idx(-1)
            , _que(TWEUTILS::FixedQueue<uint8_t>::size_type(bufsize))
            , _buf()
            , _buf_len(0)
            , _hook_on_write(nullptr)
            , _session_id(-1)
			, _modctl_capable(false)
			, _mtx()
        {
#if MWM5_SDL2_USE_MULTITHREAD_RENDER == 1 && MWM5_USE_SDL2_MUTEX == 1
			_mtx = SDL_CreateMutex();
#endif
		}

		void _set_modctl_capable(bool n) { _modctl_capable = n; } // may set internally when opened.
		int get_modctl_capable() { return _modctl_capable; }

		/**
		 * @fn	int SerialCommon::find_idx_by_name(const char* devname)
		 *
		 * @brief	Searches for the first index by name
		 *
		 * @param	devname	The device name.
		 *
		 * @returns	The found index by name.
		 */
		int find_idx_by_name(const char* devname) {
			for (int i = 0; i < ser_count; i++) {
				if (!strcmp(devname, ser_devname[i])) return i;
			}
			return -1;
		}

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

        /**
         * @fn	bool SerialCommon::open(const char* devname)
         *
         * @brief	Opens port by the given devname
         *
         * @param	devname	The device name
         *
         * @returns	True if it succeeds, false if it fails.
         */
        bool open(const char* devname) {
			// find index of the device list
			int devidx = find_idx_by_name(devname);
			if (devidx == -1) return false;

			_modctl_capable = ser_modctl_mode[devidx]; // check if the device name is TWELITE-R (modctl capable)

			// open it (call method by CTRP)
			bool ret = static_cast<CDER&>(*this)._open(devname);

			// success!
			if (ret) {
#if defined(_MSC_VER) || defined(__MINGW32__)
				strncpy_s(_devname_prev, _devname, sizeof(_devname_prev));
#elif defined(__APPLE__) || defined(__linux)
				strncpy(_devname_prev, _devname, sizeof(_devname_prev));
#endif
				_devname_opened_idx = devidx;
			}

			// returns (true if success)
			return ret;
        }

        /**
         * @fn	void SerialCommon::close()
         *
         * @brief	Closes this port
         */
	    void close() { 
			static_cast<CDER&>(*this)._close();
			_devname_opened_idx = -1;
			_session_id = -1;
			_devname[0] = 0; // clear _devname as well
        }
	
		/**
		 * @fn	bool SerialCommon::reopen()
		 *
		 * @brief	Reopens this port
		 *
		 * @returns	True if it succeeds, false if it fails.
		 */
		bool reopen() {
			if (_is_opened()) {
				close();
			}

			bool ret = false;
			if (_devname_prev[0] != 0)
				ret = open(_devname_prev);

			return ret;
		}

        /**
         * @fn	void SerialCommon::flush()
         *
         * @brief	Flushes TX requests.
         */
        void flush() {
			if (auto l = TWE::LockGuard(_mtx)) {
				static_cast<CDER&>(*this)._flush();
			}
        }

		/**
		 * @fn	bool SerialCommon::is_opened()
		 *
		 * @brief	Query if this object is opened
		 *
		 * @returns	True if opened, false if not.
		 */
	private:
		bool _is_opened() {
			return _session_id != -1;
		}
	public:
		bool is_opened() {
			bool r = false;
			if (auto l = TWE::LockGuard(_mtx)) {
				r = _is_opened();
			}
			return r;
		}

        /**
         * @fn	bool SerialCommon::set_baudrate(int baud)
         *
         * @brief	Sets a baudrate
         *
         * @param	baud	The baud.
         *
         * @returns	True if it succeeds, false if it fails.
         */
	private:
        bool _set_baudrate(int baud) {
			return static_cast<CDER&>(*this)._set_baudrate(baud);
        }
	public:
		bool set_baudrate(int baud) {
			return _set_baudrate(baud);
		}

		/**
		 * @fn	const char* SerialFtdi::get_devname()
		 *
		 * @brief	Gets the devname currently opened.
		 *
		 * @returns	Null if it fails, else the devname.
		 */
	private:
		const char* _get_devname() {
			return _devname;
		}
	public:
		const char* get_devname() {
			const char* r = nullptr;
			if (auto l = TWE::LockGuard(_mtx)) {
				r = _get_devname();
			}

			return  r;
		}
		/**
		 * @fn	int SerialCommon::get_handle()
		 *
		 * @brief	Gets the handle
		 *
		 * @returns	The handle. (-1:not open, other session id)
		 */
	private:
		int _get_handle() {
			return _session_id;
		}
	public:
		int get_handle() {
			int r = 0;
			if (auto l = TWE::LockGuard(_mtx)) {
				r = _get_handle();
			} 
			return r;
		}

	public:
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
			int r = -1;
			if (auto l = TWE::LockGuard(_mtx)) {
				if (i >= 0 && i < _buf_len) {
					r = _buf[i];
				}
			}

			return r;
		}

        /**
		 * @fn	bool SerialCommon::available()
		 *
		 * @brief	if data is in the internal queue, returns true.
		 *
		 * @returns	True if the queue has some data, false if it's empty.
		 */
		bool available() {
			bool r = false;
			if (auto l = TWE::LockGuard(_mtx)) {
				r = !_que.empty();
			}
			return r;
		}

        /**
		 * @fn	int SerialCommon::read()
		 *
		 * @brief	read one byte (from _que)
		 *
		 * @returns	An int. -1:error
		 */
		int read() {
			int r = -1;
			if (auto l = TWE::LockGuard(_mtx)) {
				if (!_que.empty()) {
					uint8_t c = _que.front();
					_que.pop();

					r = c;
				}
			}

			return r;
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
			bool b_hook_use = false;

			if (auto l = TWE::LockGuard(_mtx)) {
				if (_is_opened()) {
					len_written = static_cast<CDER&>(*this)._write(p, len);		
					b_hook_use = true;
				}
			}
			if (b_hook_use && _hook_on_write) _hook_on_write(p, len);

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
			int r = 0;
			if (auto l = TWE::LockGuard(_mtx)) {
				r = static_cast<CDER&>(*this)._update();
			}
			return r;
        }

		/**
		 * @fn	TWEUTILS::SmplBuf_WChar& SerialCommon::query_extra_device_info()
		 *
		 * @brief	Queries extra device information
		 *
		 * @returns	The extra device information.
		 */
		const wchar_t* query_extra_device_info() {
			static_cast<CDER&>(*this)._query_extra_device_info();
			return _devname_extra_info.c_str();
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
		SERSRV_ESC() : _chr_save(), _chr_save_ct(0), _state(0) {}

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