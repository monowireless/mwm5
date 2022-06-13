/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if !defined(MWM5_SERIAL_NO_FTDI) && (defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__))

#include "serial_ftdi.hpp"
#include "mwm5.h"

extern "C" int printf_(const char* format, ...);

using namespace TWE;

// implementation
bool SerialFtdi::_set_baudrate(int baud) {
	if (_ftHandle != NULL) {
#ifdef USE_FT_W32_API
		FTDCB ftDCB;
		if (FT_W32_GetCommState(_ftHandle, &ftDCB)) {
			ftDCB.BaudRate = baud;
			if (FT_W32_SetCommState(_ftHandle, &ftDCB)) return true;
		}
#else
		FT_SetBaudRate(_ftHandle, ULONG(baud));
		FT_SetDataCharacteristics(_ftHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);

		return true;
#endif
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

#if defined(USE_FT_W32_API)
		if (FT_W32_ReadFile(_ftHandle, _buf, rxBytes, &rxBytesReceived, NULL)) {
			_ftStatus = FT_OK;
		}
		else {
			_ftStatus = FT_OTHER_ERROR;
		}
#else
		_ftStatus = FT_Read(_ftHandle, _buf, rxBytes, &rxBytesReceived);
#endif

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
				const char STR_DEV_GENERAL[] = "FTDI_GEN";

				bool b_dev_general = false;
				if (!strncmp(Description, "MONOSTICK", 9)) strdev = STR_DEV_MONOSTICK;
				else if (!strncmp(Description, "TWE-Lite-R", 10)) {
					if (SerialNumber[0] == 'R' && SerialNumber[1] == '2') strdev = STR_DEV_TWELITER2;
					else strdev = STR_DEV_TWELITER1;
				}
				else if (!strncmp(Description, "TWE-Lite-USB", 12)) {
					strdev = STR_DEV_MONOSTICK;
				}
				else {
					strdev = STR_DEV_GENERAL;
					b_dev_general = true;
				}

				// SerialNumber would be NULL string in case of an error (already opened)
				if (SerialNumber[0] == 0) {
					strdev = nullptr;
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
					ser_modctl_mode[ser_count + nStored] = b_dev_general ? 0 : 1;
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

#ifdef USE_FT_W32_API
		_ftHandle = FT_W32_CreateFile(
			devname,
			GENERIC_READ | GENERIC_WRITE,
			0,
			0,
			OPEN_EXISTING,
			FT_OPEN_BY_SERIAL_NUMBER // FILE_ATTRIBUTE_NORMAL | FT_OPEN_BY_SERIAL_NUMBER
			,
			0
		);
		_ftStatus = _ftHandle != INVALID_HANDLE_VALUE ? FT_STATUS(FT_OK) : FT_STATUS(FT_INVALID_HANDLE);
#else
		_ftStatus = FT_OpenEx((void*)devname, FT_OPEN_BY_SERIAL_NUMBER, &_ftHandle);
#endif

		if (_ftStatus == FT_OK) {
			// default configuration
#ifdef USE_FT_W32_API
			FT_W32_SetupComm(_ftHandle, 2048, 2048);

			FTDCB ftDCB;
			if (FT_W32_GetCommState(_ftHandle, &ftDCB)) {
				ftDCB.BaudRate = 115000;
				ftDCB.ByteSize = 8;
				ftDCB.Parity = FT_PARITY_NONE;
				ftDCB.StopBits = FT_STOP_BITS_1;

				ftDCB.fBinary = TRUE;
				ftDCB.fOutX = false;
				ftDCB.fInX = false;
				ftDCB.fErrorChar = false;
				ftDCB.fRtsControl = false;
				ftDCB.fAbortOnError = false;

				if (FT_W32_SetCommState(_ftHandle, &ftDCB))
					; // FT_W32_SetCommState ok
				else
					; // FT_W32_SetCommState failed
			}
			else {
				;
			}

			/*
			FTTIMEOUTS ftTS;
			ftTS.ReadIntervalTimeout = 0;
			ftTS.ReadTotalTimeoutMultiplier = 0;
			ftTS.ReadTotalTimeoutConstant = 500;
			ftTS.WriteTotalTimeoutMultiplier = 0;
			ftTS.WriteTotalTimeoutConstant = 500;
			if (FT_W32_SetCommTimeouts(_ftHandle, &ftTS))
				; // FT_W32_SetCommTimeouts OK
			else
				; // FT_W32_SetCommTimeouts failed
			*/

			FT_W32_PurgeComm(_ftHandle, FT_PURGE_TX | FT_PURGE_RX);

			FT_SetLatencyTimer(_ftHandle, 2); //2ms to 16ms
#else
			//FT_SetUSBParameters(_ftHandle, 4096, 4096);
			FT_SetLatencyTimer(_ftHandle, 2);
			FT_SetTimeouts(_ftHandle, 500, 0);
			FT_SetBaudRate(_ftHandle, FT_BAUD_115200);
			FT_SetFlowControl(_ftHandle, FT_FLOW_NONE, 0, 0);
			FT_SetDataCharacteristics(_ftHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
			FT_Purge(_ftHandle, FT_PURGE_TX | FT_PURGE_RX);
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
			strncpy_s(_devname, devname, sizeof(_devname));
#elif defined(__APPLE__) || defined(__linux)
			strncpy(_devname, devname, sizeof(_devname));
#endif
			_session_id = _ftHandle == nullptr ? -1 : (*(uint64_t*)(void*)_ftHandle) & 0xFFFFFFFF; // opened!


			return true;
		}
		else {
			_ftHandle = NULL;
			return false;
		}
	}
	else return false;
}

void SerialFtdi::_query_extra_device_info() {
	SUPER_SER::_devname_extra_info.clear();

	if (is_opened()) {
		FT_STATUS ftStatus;
		LONG lComPortNumber;
		ftStatus = FT_GetComPortNumber(_ftHandle, &lComPortNumber);

		if (ftStatus == FT_OK) {
			if (lComPortNumber == -1) {
				// No COM port assigned
			}
			else {
				// COM port assigned with number held in lComPortNumber
				_devname_extra_info << printfmt("COM%d", lComPortNumber);
			}
		}
		else {
			// FT_GetComPortNumber FAILED!
		}
	}
}


#endif //WIN/MAC