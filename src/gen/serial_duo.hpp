#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include "twe_common.hpp"
#include "twe_serial.hpp"
#include "serial_common.hpp"

namespace TWE {
    template <class C, class D>
    class SerialDuo : public ISerial, public SerialCommon<SerialDuo<C,D>> {
        using SUPER_SER = SerialCommon<SerialDuo<C,D>>;
        friend SUPER_SER;

    public:
        C _objC;
        D _objD;
        int _idx_obj;

    public:
        SerialDuo(size_t bufsize = 2048) : 
            SUPER_SER(bufsize)
            , _objC(SUPER_SER::SIZ_READ_BUFF)
            , _objD(SUPER_SER::SIZ_READ_BUFF)
            , _idx_obj(-1)
        {}

        // -1: not opened, 0:objC, 1:objD
        int get_cur_obj_id() {
            if (SUPER_SER::is_opened()) {
                return _idx_obj;
            }
            
            return -1; // not opened
        }

    private:
        bool _open(const char* devname) {
            if (SUPER_SER::is_opened()) {
                _close();
            }

            SUPER_SER::_session_id = -1; // mark as closed

            int idx = -1;
            if (devname != nullptr) {
                int i;
                for(i = 0; i < SerialPortEntries::ser_count; i++) {
                    if (!strcmp(devname, SerialPortEntries::ser_devname[i])) {
                        break;
                    }
                }

                if (i < SerialPortEntries::ser_count) {
                    idx = i;
                    if (i <  SerialPortEntries::_idx_C_end) {
                        if (_objC.open(devname)) {
                            _idx_obj = 0;
                            SUPER_SER::_session_id = _objC._session_id;
                            strncpy(SUPER_SER::_devname, _objC._devname, SUPER_SER::SIZ_DEV_NAME);
                        }
                    } else {
                        if (_objD.open(devname)) {
                            _idx_obj = 1;
                            SUPER_SER::_session_id = _objD._session_id;
                            strncpy(SUPER_SER::_devname, _objD._devname, SUPER_SER::SIZ_DEV_NAME);
                        }
                    }
                }
            }

            return _idx_obj != -1;
        }

        void _close() {
            if (SUPER_SER::is_opened()) {
                if(_idx_obj == 0) _objC.close();
                else if (_idx_obj == 1) _objD.close();
            }
            _idx_obj = -1;
            SUPER_SER::_session_id = -1;
		}

        void _flush() {
            if (SUPER_SER::is_opened()) {
                if(_idx_obj == 0) _objC.flush();
                else if (_idx_obj == 1) _objD.flush();
            }
		}

        bool _set_baudrate(int baud) {
            if (SUPER_SER::is_opened()) {
                if(_idx_obj == 0) return _objC.set_baudrate(baud);
                else if (_idx_obj == 1) return _objD.set_baudrate(baud);
            }
            return false;
        }

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
            if (SUPER_SER::is_opened()) {
                if(_idx_obj == 0) return _objC.write(p, len);
                else if (_idx_obj == 1) return _objD.write(p, len);
            }
            return -1;
		}

        // update this object (copy serial buffer in C/D into this object/)
        template <class T>
        int _update_this(T& obj) {
            int ret = -1;
            ret = obj.update();
            SUPER_SER::_buf_len = obj._buf_len;
            for (int i = 0; i < SUPER_SER::_buf_len; i++) {
                SUPER_SER::_buf[i] = obj._buf[i];
                SUPER_SER::_que.push(uint8_t(SUPER_SER::_buf[i]));
            }
            obj._que.clear();
            return ret;
        }

		/**
		 * @fn	int SerialFtdi::update();
		 *
		 * @brief	peek data from the serial driver and store data into internal queue.
		 *
		 * @returns	An int. (0>: read bytes)
		 */
		int _update() {
            if (SUPER_SER::is_opened()) {
                int ret = -1;
                if(_idx_obj == 0) ret = _update_this<C>(_objC);
                else if (_idx_obj == 1) ret = _update_this<D>(_objD);
                return ret;
            }
            return -1;
        }

	private:
		/**
		 * @fn	static int SerialFtdi::list_devices()
		 *
		 * @brief	List devices, stores list to internal static array.
		 *
		 * @returns	An int.
		 */
		static int _list_devices(bool append_entry=false) {
            C::_list_devices();
            SerialPortEntries::_idx_C_end = SerialPortEntries::ser_count;
            D::_list_devices(true);
            SerialPortEntries::_idx_D_end = SerialPortEntries::ser_count;

            return SerialPortEntries::ser_count;
        }
    };
}

#endif