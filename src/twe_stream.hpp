#pragma once 

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */


#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_utils.hpp"
#include <cstdarg>

namespace TWE {
	class IStreamOut;
	class IStreamSpecial;
	class IStream_endl;

	/// <summary>
	/// special char handling (endl, etc...)
	/// </summary>
	class IStreamSpecial {
	public:
		virtual inline IStreamOut& operator ()(IStreamOut& of) = 0;
	};


	/// <summary>
	/// function object to write to stream (serial or etc...)
	/// </summary>
	class IStreamOut {
	protected:
		IStreamOut() {}
	public:
		virtual inline IStreamOut& operator ()(char_t c) = 0; //! () operator as a function object
		virtual inline IStreamOut& write_w(wchar_t c) { return *this; }
		inline IStreamOut& operator << (char_t c) { return (*this)(c); } // should be on root class
		inline IStreamOut& operator << (wchar_t c) { return write_w(c); } // should be on root class
		inline IStreamOut& operator << (IStreamSpecial& sc) { return sc(*this); } // implement std::endl like object
		inline IStreamOut& operator << (const char* s) { // const char*
			while (*s != 0) operator ()((char_t)*s++);
			return *this;
		}
	};

	/// <summary>
	/// Dummy putchar.
	/// </summary>
	class PutCharNull : public IStreamOut {
	public:
		inline IStreamOut& operator ()(char_t c) { return *this; }
	};


	/// <summary>
	/// TWESERCMD::endl handling
	/// </summary>
	class IStream_endl : public IStreamSpecial {
	private:
		uint8_t _u8Style;

	public:
		const uint8_t CRLF = 0;
		const uint8_t LF = 1;

		// set or get style
		uint8_t& style() { return _u8Style; }

		IStream_endl(uint8_t u8Style = 0) : _u8Style(u8Style) {}

		inline IStreamOut& operator ()(IStreamOut& of) {
			if (!_u8Style) {
				of('\r');
			}
			of('\n');
			return of;
		}
	};


	/// <summary>
	/// object to read from stream (serial or etc...)
	/// </summary>
	class IStreamIn {
	public:
		IStreamIn() {}
		virtual inline int get_a_byte() = 0;
	};

	/// <summary>
	/// dummy get char class
	/// </summary>
	class GetCharNull : public IStreamIn {
	public:
		inline int get_a_byte() {
			return -1;
		}
	};


	/// <summary>
	///  pre-created endl object.
	/// </summary>
	extern IStream_endl Endl;
	extern IStream_endl crlf;
}