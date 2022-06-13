#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_utils_crc8.hpp"
#include "twe_utils_simplebuffer.hpp"

#include <string>
#include <cctype>
#include <cwctype>
#include <utility>
#include <type_traits>

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
	
	inline uint8_t G_OCTET(const uint8_t*& p) {
		return *(p)++; 
	}
	
	inline uint16_t G_WORD(const uint8_t*& p) {
		uint16_t r = *p++;
		r = (r << 8) + *p++;
		return r;
	}
	
	inline uint32_t G_DWORD(const uint8_t*& p) {
		uint32_t r = *p++;
		r = (r << 8) + *p++;
		r = (r << 8) + *p++;
		r = (r << 8) + *p++;
		return r;
	}

	inline uint16_t G_LE_WORD(const uint8_t*& p) {
		uint16_t r = *p++;
		r |= (*p++ << 8);
		return r;
	}

	inline uint32_t G_LE_DWORD(const uint8_t*& p) {
		uint32_t r = *p++;
		r |= (*p++ << 8);
		r |= (*p++ << 16);
		r |= (*p++ << 24);
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

	inline uint8_t& S_LE_WORD(uint8_t*& q, uint16_t c) {
		*(q) = ((c) & 0xff); (q)++;
		*(q) = ((c) >> 8) & 0xff; (q)++;
		return *q;
	}

	inline uint8_t& S_DWORD(uint8_t*& q, uint32_t c) {
		*(q) = ((c) >> 24) & 0xff; (q)++;
		*(q) = ((c) >> 16) & 0xff; (q)++;
		*(q) = ((c) >>  8) & 0xff; (q)++;
		*(q) = ((c) & 0xff); (q)++;
		return *q;
	}

	inline uint8_t& S_LE_DWORD(uint8_t*& q, uint32_t c) {
		*(q) = ((c) & 0xff); (q)++;
		*(q) = ((c) >> 8) & 0xff; (q)++;
		*(q) = ((c) >> 16) & 0xff; (q)++;
		*(q) = ((c) >> 24) & 0xff; (q)++;
		return *q;
	}

	inline uint8_t EncodeVolt(uint16_t m) {
		return (m < 1950 ? 0 : (m > 3650 ? 255 : (m <= 2802 ? ((m-1950+2)/5) : ((m-2800-5)/10+171)) ));
	}

	inline uint16_t DecodeVolt(uint8_t i) {
		return (i <= 170 ? (1950+i*5) : (2800+(i-170)*10) );
	}

#if 0
	/**
	 * @class	enum_wapper
	 *
	 * @brief	An enum wapper.
	 *
	 * @tparam	_Enum	Type of the enum.
	 * @tparam	T	 	Generic type parameter.
	 */
	template<class _Enum, typename T = uint8_t>
	class enum_wapper_strict
	{
	private:
		_Enum _e;
		// operator int() = delete;
	public:
		enum_wapper_strict(const _Enum& e) { _e = e; }
		inline operator _Enum() const { return _e; }
		inline _Enum to_enum() const { return _e; }
		inline void operator =(const _Enum& x) { _e = x; }
		inline bool operator ==(const _Enum& x) const { return _e == x; }
	};
#endif

	/**
	 * @class	enum_wapper
	 *
	 * @brief	An enum wapper.
	 *
	 * @tparam	_Enum	Type of the enum.
	 * @tparam	T	 	Generic type parameter.
	 */
	template<class _Enum>
	class enum_wapper
	{
	public:
		using T = typename std::underlying_type<_Enum>::type;

	private:
		_Enum _e;
		// operator int() = delete;
	public:
		enum_wapper() : _e() {}
		enum_wapper(const _Enum e) { _e = e; }
		
		inline operator _Enum() const { return _e; }
		inline _Enum to_enum() const { return _e; }
		inline void operator =(const _Enum e) { _e = e; }
		inline bool operator ==(const _Enum e) const { return _e == e; }
		
		enum_wapper(const T& x) { _e = static_cast<_Enum>(x); }
		//inline operator int() { return static_cast<int>(_e); }
		inline _Enum to_value() const { return _e; }
		inline void operator =(const T x) { _e = static_cast<_Enum>(x); }
		inline bool operator ==(const T x) const { return _e == static_cast<_Enum>(x); }
	};


	/**
	 * @class	backwards
	 *
	 * @brief	A backwards iterator support in range for loop.
	 * 			
	  				SmplBuf_ByteL<128> lbuff;

					lbuff.push_back('a');
					lbuff.push_back('b');
					lbuff.push_back('c');

					for (auto x : backwards(lbuff.get())) {
						putchar(x);
					}

	 *
	 * @tparam	T	Generic type parameter.
	 */
#ifndef ESP32
	template<class T>
	class backwards {
		T& _obj;
		public:
		backwards(T& obj) : _obj(obj) {}
		auto begin() { return _obj.rbegin(); }
		auto end() { return _obj.rend(); }
	};
#endif


	/**
	 * @fn	template <typename T, size_t SIZ> size_t sizeof_array(const T(&)[SIZ])
	 *
	 * @brief	Sizeof array
	 *
	 * @tparam	T  	Generic type parameter.
	 * @tparam	SIZ	Type of the siz.
	 * @param	parameter1	The first parameter.
	 *
	 * @returns	A size_t.
	 */
	template <typename T, size_t N>
	constexpr size_t elements_of_array(T(&)[N])
	{
		return N;
	}



	/**
	 * @class	ptr_wrapper
	 *
	 * @brief	A pointer wrapper.
	 * 			if (auto p = 
	 *
	 * @tparam	T	Generic type parameter.
	 */
	template <typename T>
	class _ptr_wrapper {
		T _ptr;
	public:
		_ptr_wrapper(T ptr) : _ptr(ptr) {}
		explicit operator bool() { return _ptr != nullptr; }
		T ref() { return _ptr; }
	};

	template <typename T>
	_ptr_wrapper<T*> ptr_wrapper(T* ptr) {
		return _ptr_wrapper<T*>(ptr);
	}

	// add const
	template< class T >
	const T* as_copying(T* ptr)
	{
		return const_cast<const T*>(ptr);
	}

	// add const (e.g. explicitly copying to operator =)
	template< class T >
	const T& as_copying(T& ref)
	{
		return const_cast<const T&>(ref);
	}

	// just mimic std:move
	template <class T>
	constexpr typename std::remove_reference<T>::type&& as_moving(T&& _Arg) noexcept { // forward _Arg as movable
		return static_cast<typename std::remove_reference<T>::type&&>(_Arg);
	}

	//remove line end \r \n
	template <class T>
	void remove_endl(T& str) {
		if (str.size() > 0 && (str[str.size() - 1] == '\r' || str[str.size() - 1] == '\n')) {
			str.resize(str.size() - 1);
			if (str.size() > 0 && (str[str.size() - 1] == '\r' || str[str.size() - 1] == '\n')) {
				str.resize(str.size() - 1);
			}
		}
	}
} // TWEUTILS

