/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)

#include "serial_ftdi.hpp"

extern "C" int printf_(const char* format, ...);

using namespace TWE;

// implementation
bool SerialFtdi::_set_baudrate(int baud) {
	if (_ftHandle != NULL) {
		FT_SetBaudRate(_ftHandle, ULONG(baud));
		FT_SetDataCharacteristics(_ftHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);

		return true;
	}

	return false;
}


int SerialFtdi::_update() {
	if (_ftHandle != NULL) {
		DWORD rxBytes = 0, rxBytesReceived = 0;

		_buf_len = 0;

		FT_GetQueueStatus(_ftHandle, &rxBytes);

		if (rxBytes >= sizeof(_buf)) {
			rxBytes = sizeof(_buf);
		}
		if (rxBytes >= DWORD(_que.capacity() - _que.size())) {
			rxBytes = DWORD(_que.capacity() - _que.size() - 1);
		}

		_ftStatus = FT_Read(_ftHandle, _buf, rxBytes, &rxBytesReceived);

		if (_ftStatus == FT_OK) {
			for (DWORD i = 0; i < rxBytesReceived; i++) {
				_que.push(uint8_t(_buf[i]));
			}

			_buf_len = rxBytesReceived;
			return _buf_len;
		}
	}

	return 0;
}


int SerialFtdi::_list_devices(bool append_entry) {
	FT_STATUS ftStatus;
	FT_HANDLE ftHandleTemp;
	DWORD numDevs = 0;
	DWORD Flags;
	DWORD ID;
	DWORD Type;
	DWORD LocId;
	char SerialNumber[16] = {};
	char Description[64] = {};

	// if true, append items
	if (!append_entry) ser_count = 0;

	//
	// create the device information list
	//
	ftStatus = FT_CreateDeviceInfoList(&numDevs);
	if (numDevs > (DWORD)ser_desc.capacity() - ser_count) {
		numDevs = (DWORD)ser_desc.capacity() - ser_count;
	}

	if (ftStatus == FT_OK) {
		//
		// get information for device 0
		//
		int i, nStored = 0;
		for (i = 0; i < (int)numDevs; i++) {
			ftStatus = FT_GetDeviceInfoDetail(i, &Flags, &Type, &ID, &LocId, SerialNumber, Description, &ftHandleTemp);
			if (ftStatus == FT_OK) {
				const char* strdev = nullptr;

				const char STR_DEV_MONOSTICK[] = "MONOSTICK";
				const char STR_DEV_TWELITER1[] = "TWELITE R";
				const char STR_DEV_TWELITER2[] = "TWELITE R2";
				

				if (!strncmp(Description, "MONOSTICK", 9)) strdev = STR_DEV_MONOSTICK;
				else if (!strncmp(Description, "TWE-Lite-R", 10)) {
					if (SerialNumber[0] == 'R' && SerialNumber[1] == '2') strdev = STR_DEV_TWELITER2;
					else strdev = STR_DEV_TWELITER1;
				}
				else if (!strncmp(Description, "TWE-Lite-USB", 12)) {
					strdev = STR_DEV_MONOSTICK;
				}

				if (strdev != nullptr) {
					// check if MONOSTICK or TWELITER
#if defined(_MSC_VER) || defined(__MINGW32__)
					strncpy_s(ser_devname[ser_count + nStored], SerialNumber, 32);
					strncpy_s(ser_desc[ser_count + nStored], strdev, 32);
#elif defined(__APPLE__) || defined(__linux)
					strncpy(ser_devname[ser_count + nStored], SerialNumber, 32);
					strncpy(ser_desc[ser_count + nStored], strdev, 32);
#endif
					++nStored;
				}
			}
		}

		ser_count += nStored;
		return nStored;
	}

	return 0;
}

bool SerialFtdi::_open(const char* devname) {
	_session_id = -1; // closed.

	if (!is_opened()) {
		_ftStatus = FT_OpenEx((void*)devname, FT_OPEN_BY_SERIAL_NUMBER, &_ftHandle);

		if (_ftStatus == FT_OK) {
			// default configuration
			FT_SetUSBParameters(_ftHandle, 256, 256);
			FT_SetLatencyTimer(_ftHandle, 2);
			FT_SetTimeouts(_ftHandle, 500, 0);
			FT_SetBaudRate(_ftHandle, FT_BAUD_115200);
			FT_SetFlowControl(_ftHandle, FT_FLOW_NONE, 0, 0);
			FT_SetDataCharacteristics(_ftHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);

#if defined(_MSC_VER) || defined(__MINGW32__)
			strncpy_s(_devname, devname, sizeof(_devname));
#elif defined(__APPLE__) || defined(__linux)
			strncpy(_devname, devname, sizeof(_devname));
#endif
			_session_id = _ftHandle == nullptr ? -1 : int(_ftHandle); // opened!
			return true;
		}
		else {
			_ftHandle = NULL;
			return false;
		}
	}
	else return false;
}

#endif //WIN/MAC