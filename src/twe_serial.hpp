#pragma once 

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_utils_simplebuffer.hpp"
#include "twe_utils_fixedque.hpp"

#include "twe_sys.hpp"

namespace TWE {
	// so far, not used for MSC
	class ISerial {
	public:
		static const char tsAryChar32_MAXLEN = 32;
		typedef TWEUTILS::SimpleBuffer<char[tsAryChar32_MAXLEN]> tsAryChar32;

		// virtual ~ISerial() {}
		// virtual int list_devices(tsAryChar32& sn, tsAryChar32& desc) = 0;
		// virtual bool open(const char* strName) = 0;
	};


	/**
	 * @class	TWE_PutChar_ESP32_Serial
	 *
	 * @brief	Output to Serial via IStreamOut interface.
	 */
	template <class SER>
	class TWE_PutChar_Serial : public TWE::IStreamOut {
		SER& _ser;
	public:
		TWE_PutChar_Serial(SER& ser) : _ser(ser) {}

		inline IStreamOut& operator ()(char_t c) {
			uint8_t b[2] = { uint8_t(c), 0 };
			_ser.write(b, 1);

			return (*this);
		}

		inline IStreamOut& write_w(wchar_t c) {
			uint8_t b[2] = { uint8_t(c >> 8), uint8_t(c & 0xFF) };
			_ser.write(b, 2);

			return (*this);
		}
	};
}
