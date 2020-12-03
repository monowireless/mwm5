/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(__APPLE__) || defined(__linux)

#include "serial_termios.hpp"
#include <filesystem>

using namespace TWE;

bool SerialTermios::_open(const char* devname) {
    if (devname != nullptr) {
        char devname_full[128];
        ::snprintf(_devname, SIZ_DEV_NAME, "%s", devname);
        ::snprintf(devname_full, 128, "/dev/%s", devname);

        _fd = ::open(devname_full, O_RDWR | O_NOCTTY | O_NDELAY);
        _session_id = _fd;

        if (_fd == -1) return false;
        if (!_set_baudrate(115200)) return false;

        _session_id = _fd;

        return true;
    }

    return false;
}

bool SerialTermios::_set_baudrate(int baud) {
    if (is_opened()) {
        ::tcgetattr(_fd, &_options);

        switch(baud) {
            case 9600: _options.c_cflag = B9600; break;
            case 19200: _options.c_cflag = B19200; break;
            case 38400: _options.c_cflag = B38400; break;
            case 57600: _options.c_cflag = B57600; break;
            case 115200: _options.c_cflag = B115200; break;
            case 230400: _options.c_cflag = B230400; break;
            case 500000: _options.c_cflag = B500000; break;
            case 1000000: _options.c_cflag = B1000000; break;
            default: return false;
        }

        _options.c_cflag |= CS8 | CLOCAL | CREAD;
        _options.c_iflag = 0; // no translation of CR/NLs.
        _options.c_oflag = 0;
        _options.c_lflag = 0;
        ::tcflush(_fd, TCIFLUSH);
        ::tcsetattr(_fd, TCSANOW, &_options);

        return true;
    } else {
        return false;
    }
}

int SerialTermios::_update() {
	if (is_opened()) {
        int rxBytesMax = _que.capacity() - _que.size() - 1;
		_buf_len = 0;

        // read it to _buf[]
        int rxLength = ::read(_fd, (void*)_buf, rxBytesMax);

        if (rxLength > 0) {
            // push into internal queue
            for(int i = 0; i < rxLength; i++) {
                _que.push(uint8_t(_buf[i]));
            }

            _buf_len = rxLength;
            return _buf_len;
        }
	}

	return 0;
}

/**
 * @fn	static int SerialFtdi::list_devices()
 *
 * @brief	List devices, stores list to internal static array.
 *
 * @returns	An int.
 */
int SerialTermios::_list_devices(bool append_entry) {
    if (!append_entry) ser_count = 0;
    int nStored = 0;
    
    // entry
    if (std::filesystem::exists("/dev/serial0")) {
        ::strncpy(ser_devname[ser_count + nStored], "serial0", 32);
        ::strncpy(ser_desc[ser_count + nStored], "UART", 32);
        nStored++;
    }

    // entry
    if (std::filesystem::exists("/dev/serial1")) {
        ::strncpy(ser_devname[ser_count + nStored], "serial1", 32);
        ::strncpy(ser_desc[ser_count + nStored], "UART", 32);
        nStored++;
    }

    ser_count += nStored;
    return nStored;
}
#endif