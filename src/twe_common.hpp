#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

// check if it's under 64bit system or 32bit
#if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__) ) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
#define IS64BIT 1
#else
#define IS32BIT 1
#endif

// define firstly prior to other headers inclusion.
#if __GNUC__
#define _GLIBCXX_DEQUE_BUF_SIZE 32	// smaller chunk for `deque' in GCC, 
									// the default 512bytes are too big for embedded systems.
#endif

// type definition
#include <stdint.h> // for type name
typedef char char_t;
#ifndef NULL
#define NULL nullptr
#endif

// key input type
namespace TWE {
	typedef int32_t keyinput_type;

	/**
	 * @class	APIRET
	 *
	 * @brief	API return value, consist with 1bit bool and 31bits value.
	 */
	class APIRET {
		uint32_t _code;
	public:
		APIRET() : _code(0) {}
		APIRET(bool b) {
			_code = (b ? 0x80000000 : 0);
		}
		APIRET(bool b, uint32_t val) {
			_code = (b ? 0x80000000 : 0) + (val & 0x7fffffff);
		}
		inline bool is_success() const { return ((_code & 0x80000000) != 0); }
		inline bool is_fail() const { return ((_code & 0x80000000) == 0); }
		inline uint32_t get_value() const { return _code & 0x7fffffff; }
		inline operator uint32_t() const { return get_value(); }
		inline operator bool() const { return is_success(); }
	};
}

// use of RTTI (not mandate anymore)
#if defined(ESP32)
# undef TWE_USE_RTTI // you can enable it (but you need to edit platform.txt)
#elif defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)
#define TWE_USE_RTTI
#endif

// Architecture specific header
#if defined(ESP32)
# include <M5Stack.h>
# undef min // to avoid confilict in M5Stack's system header.
# undef max
#elif defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)
// # include "../msc/MSC_SDL2_Screen/M5Stack.h"
#endif
