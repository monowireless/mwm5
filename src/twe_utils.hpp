#pragma once

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

#include "twe_common.hpp"
#include "twe_utils_crc8.hpp"
#include "twe_utils_simplebuffer.hpp"

namespace TWEUTILS {
	inline uint8_t G_OCTET(uint8_t*& p) {
		return *(p)++; 
	}
	inline uint16_t G_WORD(uint8_t*& p) {
		uint32_t r = *p++;
		r = (r << 8) + *p++;
		return r;
	}
	inline uint32_t G_DWORD(uint8_t*& p) {
		uint32_t r = *p++;
		r = (r << 8) + *p++;
		r = (r << 8) + *p++;
		r = (r << 8) + *p++;
		return r;
	}
	inline uint8_t& S_OCTET(uint8_t*& q, uint8_t c) {
		*q++ = c;
		return *q;
	}
	inline uint8_t& S_WORD(uint8_t*& q, uint16_t c) {
		*(q) = ((c) >> 8) & 0xff; (q)++;
		*(q) = ((c) & 0xff); (q)++;
		return *q;
	}
	inline uint8_t& S_DWORD(uint8_t*& q, uint32_t c) {
		*(q) = ((c) >> 24) & 0xff; (q)++;
		*(q) = ((c) >> 16) & 0xff; (q)++;
		*(q) = ((c) >>  8) & 0xff; (q)++;
		*(q) = ((c) & 0xff); (q)++;
		return *q;
	}

	/// <summary>
	/// UNICODE single/double width check
	/// </summary>
	/// <param name="c">UCS code</param>
	/// <returns>true if doubled</returns>
	inline bool Unicode_isSingleWidth(uint16_t c) {
		return  (
			(c < 0x0100)
			|| (c >= 0xFF61 && c <= 0xFF9F) // hankaku kana
			);
	}


	/// <summary>
	/// UTF8 to UCS2 converter
	/// </summary>
	class Unicode_UTF8Converter {
		uint8_t _utf8_stat;
		uint16_t _utf8_result;
	public:
		Unicode_UTF8Converter() : _utf8_stat(0), _utf8_result(0) {}
		inline uint16_t operator() (const char c) {
			operator() ((uint8_t)c);
		}
		inline uint16_t operator() (uint8_t c) {
			uint16_t wc = 0;
			// ASCII
			if ((c & 0x80) == 0x00) {
				_utf8_stat = 0;
				wc = c;
			}

			if (_utf8_stat == 0) {
				// 11bit
				if ((c & 0xE0) == 0xC0) {
					_utf8_result = ((c & 0x1F) << 6);
					_utf8_stat = 1;
				}

				// 16bit
				if ((c & 0xF0) == 0xE0) {
					_utf8_result = ((c & 0x0F) << 12);
					_utf8_stat = 2;
				}

				// not support
				if ((c & 0xF8) == 0xF0) wc = c;
			}
			else {
				if (_utf8_stat == 2) {
					_utf8_result |= ((c & 0x3F) << 6);
					_utf8_stat--;
				}
				else {
					_utf8_result |= (c & 0x3F);
					_utf8_stat = 0;
					wc = _utf8_result;
				}
			}

			return wc;
		}
	};
	typedef SimpleBuffer<uint8_t> SmplBuf_Byte;
} // TWEUTILS

