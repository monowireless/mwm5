#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_utils_simplebuffer.hpp"

#include <string>
#include <cctype>
#include <cwctype>

namespace TWEUTILS {

	/**
	 * @fn	inline bool Unicode_isSingleWidth(uint16_t c)
	 *
	 * @brief	UNICODE single/double width check
	 *
	 * @param	c	UCS code.
	 *
	 * @returns	true if doubled.
	 */
	inline constexpr bool Unicode_isSingleWidth(uint16_t c) {
		return  (
			(c < 0x0100)
			|| (c >= 0xFF61 && c <= 0xFF9F) // hankaku kana
			);
	}


	/**
	 * @fn	inline int strlen_vis(const wchar_t* p)
	 *
	 * @brief	get visual length of string (count wide char as two)
	 *
	 * @param	p	A wchar_t to process.
	 *
	 * @returns	An int.
	 */
	inline int strlen_vis(const wchar_t* p) {
		int len = 0;
		while(*p != 0) {
			len += Unicode_isSingleWidth(uint16_t(*p)) ? 1 : 2;
			++p;
		}

		return len;
	}

	inline int strlen_vis(const wchar_t c) {
		return Unicode_isSingleWidth(c) ? 1 : 2;
	}


	/**
	 * @fn	inline int strlen_vis(const wchar_t* p)
	 *
	 * @brief	get visual length of string (count wide char as two)
	 *
	 * @param	p	A wchar_t to process.
	 *
	 * @returns	An int.
	 */
	inline int strlen_vis(const TWEUTILS::SmplBuf_WChar& str) {
		int len = 0;
		for (auto x : str) {
			len += Unicode_isSingleWidth(uint16_t(x)) ? 1 : 2;
		}

		return len;
	}

	/**
	 * @class	Unicode_UTF8Converter
	 *
	 * @brief	UTF8 to UCS2 converter
	 */
	class Unicode_UTF8Converter {
		uint8_t _utf8_stat;
		uint16_t _utf8_result;
	public:
		Unicode_UTF8Converter() : _utf8_stat(0), _utf8_result(0) {}
		inline uint16_t operator() (const char c) {
			return operator() ((uint8_t)c);
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


	/**
	 * @fn	static inline void toUpper(std::basic_string<char>& s)
	 *
	 * @brief	string to upper
	 *
	 * @param [in,out]	s	A std::basic_string&lt;char&gt; to process.
	 */
	static inline void toUpper(std::basic_string<char>& s) {
		for (std::basic_string<char>::iterator p = s.begin();
			p != s.end(); ++p) {
			*p = toupper(*p); // toupper is for char
		}
	}


	/**
	 * @fn	static inline void toUpper(std::basic_string<wchar_t>& s)
	 *
	 * @brief	Converts the s to an upper
	 *
	 * @param [in,out]	s	A std::basic_string&lt;wchar_t&gt; to process.
	 */
	static inline void toUpper(std::basic_string<wchar_t>& s) {
		for (std::basic_string<wchar_t>::iterator p = s.begin();
			p != s.end(); ++p) {
			*p = towupper(*p); // towupper is for wchar_t
		}
	}


	/**
	 * @fn	static inline void toLower(std::basic_string<char>& s)
	 *
	 * @brief	Converts the s to a lower
	 *
	 * @param [in,out]	s	A std::basic_string&lt;char&gt; to process.
	 */
	static inline void toLower(std::basic_string<char>& s) {
		for (std::basic_string<char>::iterator p = s.begin();
			p != s.end(); ++p) {
			*p = tolower(*p);
		}
	}


	/**
	 * @fn	static inline void toLower(std::basic_string<wchar_t>& s)
	 *
	 * @brief	Converts the s to a lower
	 *
	 * @param [in,out]	s	A std::basic_string&lt;wchar_t&gt; to process.
	 */
	static inline void toLower(std::basic_string<wchar_t>& s) {
		for (std::basic_string<wchar_t>::iterator p = s.begin();
			p != s.end(); ++p) {
			*p = towlower(*p);
		}
	}


	/**
	 * @fn	static inline void toUpper(SmplBuf_WChar& str)
	 *
	 * @brief	Converts a str to an upper
	 *
	 * @param [in,out]	str	The string.
	 */
	static inline void toUpper(SmplBuf_WChar& str) {
		for (auto&& x : str) {
			x = toupper(x);
		}
	}


	/**
	 * @fn	static inline void toLower(SmplBuf_WChar& str)
	 *
	 * @brief	Converts a str to a lower
	 *
	 * @param [in,out]	str	The string.
	 */
	static inline void toLower(SmplBuf_WChar& str) {
		for (auto&& x : str) {
			x = towlower(x);
		}
	}


	/**
	 * @fn	template <typename T> static inline bool endsWith(const std::basic_string<T>& str, const T* suffix, unsigned suffixLen)
	 *
	 * @brief	string compare at the end with.
	 *
	 * @tparam	T	Generic type parameter.
	 * @param	str		 	The string.
	 * @param	suffix   	The suffix.
	 * @param	suffixLen	Length of the suffix.
	 *
	 * @returns	True if it succeeds, false if it fails.
	 */
	template <typename T>
	static inline bool endsWith(const std::basic_string<T>& str, const T* suffix, unsigned suffixLen)
	{
		return str.size() >= suffixLen && 0 == str.compare(str.size() - suffixLen, suffixLen, suffix, suffixLen);
	}


	/**
	 * @fn	template <typename T> static inline bool endsWith(const std::basic_string<T>& str, const T* suffix)
	 *
	 * @brief	string compare at the end with.
	 *
	 * @tparam	T	Generic type parameter.
	 * @param	str   	The string.
	 * @param	suffix	The suffix.
	 *
	 * @returns	True if it succeeds, false if it fails.
	 */
	template <typename T>
	static inline bool endsWith(const std::basic_string<T>& str, const T* suffix)
	{
		return endsWith(str, suffix, std::basic_string<T>::traits_type::length(suffix));
	}


	/**
	 * @fn	static inline bool endsWith_NoCase(SmplBuf_Byte& str, const char_t* suffix, unsigned suffixlen)
	 *
	 * @brief	string compare at the end with. (case in-sensitive)
	 *
	 * @param [in,out]	str		 	The string.
	 * @param 		  	suffix   	The suffix.
	 * @param 		  	suffixlen	The suffixlen.
	 *
	 * @returns	True if it succeeds, false if it fails.
	 */
	static inline bool endsWith_NoCase(const SmplBuf_Byte& str, const char_t* suffix, unsigned suffixlen) {
		if (str.size() > suffixlen) {
			bool b = true;
			for (unsigned i = str.size() - suffixlen, j = 0; j < suffixlen; i++, j++) {
				b &= (towupper(str[i]) == towupper(suffix[j]));
			}
			return b;
		}
		else {
			return false;
		}
	}


	/**
	 * @fn	static inline bool endsWith_NoCase(SmplBuf_WChar& str, const wchar_t* suffix, unsigned suffixlen)
	 *
	 * @brief	string compare at the end with. (case in-sensitive)
	 *
	 * @param [in,out]	str		 	The string.
	 * @param 		  	suffix   	The suffix.
	 * @param 		  	suffixlen	The suffixlen.
	 *
	 * @returns	True if it succeeds, false if it fails.
	 */
	static inline bool endsWith_NoCase(const SmplBuf_WChar& str, const wchar_t* suffix, unsigned suffixlen) {
		if (str.size() > suffixlen) {
			bool b = true;
			for (unsigned i = str.size() - suffixlen, j = 0; j < suffixlen; i++, j++) {
				b &= (towupper(str[i]) == towupper(suffix[j]));
			}
			return b;
		}
		else {
			return false;
		}
	}

	/**
	 * @fn	template <typename T> static inline bool beginsWith(const std::basic_string<T>& str, const T* suffix, unsigned suffixLen)
	 *
	 * @brief	string compare at the end with.
	 *
	 * @tparam	T	Generic type parameter.
	 * @param	str		 	The string.
	 * @param	prefix   	The prefix.
	 * @param	prefixLen	Length of the prefix.
	 *
	 * @returns	True if it succeeds, false if it fails.
	 */
	template <typename T>
	static inline bool beginsWith(const std::basic_string<T>& str, const T* prefix, unsigned prefixLen)
	{
		return str.size() >= prefixLen && 0 == str.compare(0, prefixLen, prefix, prefixLen);
	}

	/**
	 * @fn	static inline bool endsWith_NoCase(SmplBuf_Byte& str, const char_t* suffix, unsigned suffixlen)
	 *
	 * @brief	string compare at the end with. (case in-sensitive)
	 *
	 * @param [in,out]	str		 	The string.
	 * @param 		  	prefix   	The prefix.
	 * @param 		  	prefixlen	The suffixlen.
	 *
	 * @returns	True if it succeeds, false if it fails.
	 */
	static inline bool beginsWith_NoCase(const SmplBuf_Byte& str, const char_t* prefix, unsigned prefixlen) {
		if (str.size() > prefixlen) {
			bool b = true;
			for (unsigned i = 0, j = 0; j < prefixlen; i++, j++) {
				b &= (towupper(str[i]) == towupper(prefix[j]));
			}
			return b;
		}
		else {
			return false;
		}
	}
	template <size_t N>
	static inline bool beginsWith_NoCase(const SmplBuf_Byte& str, const char_t(&prefix)[N]) {
		return beginsWith_NoCase(str, (const char_t*)prefix, N - 1);
	}

	/**
	 * @fn	static inline bool endsWith_NoCase(SmplBuf_WChar& str, const wchar_t* suffix, unsigned suffixlen)
	 *
	 * @brief	string compare at the end with. (case in-sensitive)
	 *
	 * @param [in,out]	str		 	The string.
	 * @param 		  	prefix   	The prefix.
	 * @param 		  	prefixlen	The prefixlen.
	 *
	 * @returns	True if it succeeds, false if it fails.
	 */
	static inline bool beginsWith_NoCase(const SmplBuf_WChar& str, const wchar_t* prefix, unsigned prefixlen) {
		if (str.size() > prefixlen) {
			bool b = true;
			for (unsigned i = 0, j = 0; j < prefixlen; i++, j++) {
				b &= (towupper(str[i]) == towupper(prefix[j]));
			}
			return b;
		}
		else {
			return false;
		}
	}
	template <size_t N>
	static inline bool beginsWith_NoCase(const SmplBuf_WChar& str, const wchar_t(&prefix)[N]) {
		return beginsWith_NoCase(str, (const wchar_t*)prefix, N - 1);
	}

	/*******************************************************************************
	 * HERE IS SORT ALGORITHM
	 *   - for string array (e.g. SimpleBuffer<SmplBuf_WChar>).
	 *   - for class object (e.g. SimpleBuffer<someclass>).
	 * 
	 *   the compare/conversion is provided lambda expression.
	 *    for example, the following call is using custom compare expression,
	 *    where it refers to member variable `.member1'.
	 * 
	 * SmpleBuffer<sometype> buf;
	 * SmplBuf_Sort(buf, [](sometype& a, sometype &b) {
	 * 		return a.member1 > b.member1; }
	 * );
	 *******************************************************************************/
	template <typename T, typename TF>
	int _SmplBuf_SCompare(const T& s1, const T& s2, TF CONV) { // [](typename T::value_type &x){return x;}) {
		auto i1 = s1.cbegin();
		auto i2 = s2.cbegin();

		while (1) {
			auto c1 = CONV(*i1);
			auto c2 = CONV(*i2);

			if (c1 < c2) {
				return -1; // e.g) s1=abc[a]bc s2=abc[c]de
			}
			else if (c1 > c2) {
				return 1; // e.g) s1=abc[c]de s2=abc[a]bc
			}

			// incr iterator
			++i1;
			++i2;

			if (i1 == s1.cend()) {
				if (i2 == s2.cend()) {
					return 0; //same
				}
				else {
					return -1; // e.g.) s1=abc, s2=abcd
				}
			}

			if (i2 == s2.cend()) {
				return 1; // e.g.) s1=abcd, s2=abc
			}

			// still the same, keep looping.
		}

		return 0; // never reach here!
	}

	template <typename T>
	int _SmplBuf_SCompare(const T& s1, const T& s2) {
		return _SmplBuf_SCompare(s1, s2, [](typename T::value_type x){return x;} );
	} 

	template <typename T>
	inline void _SmplBuf_Swap(typename T::iterator a, typename T::iterator b) {
		typename T::value_type tmp = as_moving(*b);
		*b = as_moving(*a);
		*a = as_moving(tmp);
	}

	template <typename T, typename TF>
	bool SmplBuf_Sort(T& a, TF COMP) {
		bool b_cond = true;
		if (a.size() < 2) return false; // not necessary

		// bubble sort...
		while (b_cond) {
			auto it_a = a.begin();
			auto it_a_next = it_a + 1;

			b_cond = false;

			do {
				if (COMP(*it_a, *it_a_next)) {
					_SmplBuf_Swap<T>(it_a, it_a_next);
					b_cond = true;
				}

				++it_a; ++it_a_next;
			} while (it_a_next != a.end());
		}

		return true;
	}

	template <typename T>
	bool SmplBuf_Sort(T& a) {
		return SmplBuf_Sort(a, [](typename T::value_type &a, typename T::value_type &b){return a>b;});
	}

	template <typename T>
	bool SmplBufStrA_Sort(T& a) {
		return _SmplBuf_Sort(a,
				[](typename T::value_type& x, typename T::value_type& y) {
					return _SmplBuf_SCompare(x, y) > 0;
				});
	}

	template <typename T>
	bool SmplBufStrA_Sort_NoCase(T& a) {
		return _SmplBuf_Sort(a,
				[](typename T::value_type& x, typename T::value_type& y) {
					return _SmplBuf_SCompare(x, y
						, [](typename T::value_type::value_type z) { return toupper(z); }) > 0;
				});
	}

	/**
	 * @brief	Sort an array. This sort algorithm sorts two arrays (T& a) and (S& b).
	 * 			- the sort is performed on (T& a), and (S& b) is sorted with same order of (T& a).
	 * 			- the sort order is determined by COMP function provided by template argument.
	 *
	 * @tparam	T			   	Main Array type
	 * @tparam	S			   	Sub Array type
	 * @tparam	bool COMP(T::value_type&, T::value_type&) 	Type of the value type&amp;
	 * @param [in,out]	a	The main array, which is sorted by ORDER.
	 * @param [in,out]	b	The sub array sorted with same order of 'a'.
	 *
	 * @returns	True if it succeeds, false if it fails.
	 */
	template <typename T, typename S, typename TF>
	bool _SmplBuf_Sort2(T& a, S& b, TF COMP) {
		bool b_cond = true;
		if (a.size() < 2) return false; // not necessary

		// bubble sort...
		while (b_cond) {
			auto it_a = a.begin();
			auto it_a_next = it_a + 1;
			auto it_b = b.begin();
			auto it_b_next = it_b + 1;

			b_cond = false;

			do {
				if (COMP(*it_a, * it_a_next)) {
					_SmplBuf_Swap<T>(it_a, it_a_next);
					_SmplBuf_Swap<S>(it_b, it_b_next);
					b_cond = true;
				}

				++it_a; ++it_a_next;
				++it_b; ++it_b_next;
			} while (it_a_next != a.end());
		}

		return true;
	}
	template <typename T, typename S>
	bool SmplBuf_Sort2(T& a1, S& a2) {
		return SmplBuf_Sort2(a1, a2, [](typename T::value_type& a, typename T::value_type& b) {return a > b; });
	}
	/**
	 * @fn	template <typename T, typename S> bool SmplBuf_Sort2_NoCase(T& a, S& b)
	 *
	 * @brief	Sorting two arrays similar to SmplBuf_Sort2 with case insensitive.
	 *
	 * @tparam	T	Generic type parameter.
	 * @tparam	S	Type of the s.
	 * @param [in,out]	a	A T to process.
	 * @param [in,out]	b	A S to process.
	 *
	 * @returns	True if it succeeds, false if it fails.
	 */
	template <typename T, typename S>
	bool SmplBufStrA_Sort2(T& a, S& b) {
		return _SmplBuf_Sort2(a, b
				, [](typename T::value_type& x, typename T::value_type& y) {
						return _SmplBuf_SCompare(x, y) > 0;});
	}


	/**
	 * @fn	template <typename T, typename S> bool SmplBuf_Sort2_NoCase(T& a, S& b)
	 *
	 * @brief	Sorting two arrays similar to SmplBuf_Sort2 with case insensitive.
	 *
	 * @tparam	T	Generic type parameter.
	 * @tparam	S	Type of the s.
	 * @param [in,out]	a	A T to process.
	 * @param [in,out]	b	A S to process.
	 *
	 * @returns	True if it succeeds, false if it fails.
	 */
	template <typename T, typename S>
	bool SmplBufStrA_Sort2_NoCase(T& a, S& b) {
		return _SmplBuf_Sort2(a, b,
				[](typename T::value_type& x, typename T::value_type& y) {
					return _SmplBuf_SCompare(x, y
						, [](typename T::value_type::value_type z) { return toupper(z); }) > 0;
				});
	}

	static inline bool operator <(const SmplBuf_WChar& lhs, const SmplBuf_WChar& rhs) {
		return _SmplBuf_SCompare< SmplBuf_WChar >(lhs, rhs) < 0;
	}

	static inline bool operator >(const SmplBuf_WChar& lhs, const SmplBuf_WChar& rhs) {
		return _SmplBuf_SCompare< SmplBuf_WChar >(lhs, rhs) > 0;
	}

	static inline bool operator ==(const SmplBuf_WChar& lhs, const SmplBuf_WChar& rhs) {
		return _SmplBuf_SCompare< SmplBuf_WChar >(lhs, rhs) == 0;
	}


	/** 
     * Iterator Pair
	 */
	template <typename ITER>
	static inline SmplBuf_WChar& operator << (SmplBuf_WChar& lhs, std::pair<ITER, ITER> range) {
		auto p = range.first;
		while (p != range.second) {
			lhs.push_back(*p);
			++p;
		}
		return lhs;
	}

	static inline SmplBuf_WChar& operator << (SmplBuf_WChar& lhs, const wchar_t c) {
		lhs.push_back(c);
		return lhs;
	}

	/**
	 * @fn	static inline SmplBuf_WChar& operator << (SmplBuf_WChar& lhs, SmplBuf_WChar& rhs)
	 *
	 * @brief	copy rhs from lhs
	 *
	 * @param [in,out]	lhs	The left hand side.
	 * @param [in,out]	rhs	The right hand side.
	 *
	 * @returns	The result of the operation.
	 */

	
	// static inline SmplBuf_WChar& operator << (SmplBuf_WChar& lhs, const SmplBuf_WChar& rhs) {
	template <typename SOUT, int ISSTR>
	static inline SmplBuf_WChar& operator << (SmplBuf_WChar& lhs, const TWEUTILS::SimpleBuffer<wchar_t, SOUT, ISSTR>& rhs) {
		auto p = rhs.cbegin();
		auto e = rhs.cend();

		while (p != e) {
			lhs.push_back(*p);
			++p;
		}

		return lhs;
	}

	template <typename SOUT, int N, int ISSTR>
	static inline SmplBuf_WChar& operator << (SmplBuf_WChar& lhs, const TWEUTILS::SimpleBufferL<wchar_t, N, SOUT, ISSTR>& rhs) {
		auto p = rhs.cbegin();
		auto e = rhs.cend();

		while (p != e) {
			lhs.push_back(*p);
			++p;
		}

		return lhs;
	}

	/**
	 * @fn	static inline SmplBuf_WChar& operator << (SmplBuf_WChar& lhs, const wchar_t* rhs)
	 *
	 * @brief	copy rhs from lhs
	 *
	 * @param [in,out]	lhs	The left hand side.
	 * @param 		  	rhs	The right hand side.
	 *
	 * @returns	The result of the operation.
	 */
	static inline SmplBuf_WChar& operator << (SmplBuf_WChar& lhs, const wchar_t* rhs) {
		auto p = rhs;

		while (*p != 0x0000) {
			lhs.push_back(*p);
			++p;
		}

		return lhs;
	}

	/**
	 * @fn	template <typename T1, typename T2> static inline SmplBuf_WChar operator+ (T1 lhs, T2 rhs)
	 *
	 * @brief	Addition operator
	 *
	 * @param	lhs	The first value.
	 * @param	rhs	A value to add to it.
	 *
	 * @returns	The result of the operation.
	 */
	template <typename T1, typename T2>
	static inline void _SmplBuf_WChar_add_ops(SmplBuf_WChar& buf, T1&& lhs, T2&& rhs) {
		buf << lhs;
		buf << rhs;
	}

	static inline SmplBuf_WChar operator + (SmplBuf_WChar&& lhs, SmplBuf_WChar&& rhs) {
		SmplBuf_WChar buf;
		_SmplBuf_WChar_add_ops(buf, lhs, rhs);
		return std::move(buf);
	}
	static inline SmplBuf_WChar operator + (SmplBuf_WChar&& lhs, SmplBuf_WChar& rhs) {
		SmplBuf_WChar buf;
		_SmplBuf_WChar_add_ops(buf, lhs, rhs);
		return std::move(buf);
	}
	static inline SmplBuf_WChar operator + (SmplBuf_WChar& lhs, SmplBuf_WChar&& rhs) {
		SmplBuf_WChar buf;
		_SmplBuf_WChar_add_ops(buf, lhs, rhs);
		return std::move(buf);
	}
	static inline SmplBuf_WChar operator + (SmplBuf_WChar& lhs, SmplBuf_WChar& rhs) {
		SmplBuf_WChar buf;
		_SmplBuf_WChar_add_ops(buf, lhs, rhs);
		return std::move(buf);
	}
	static inline SmplBuf_WChar operator + (SmplBuf_WChar&& lhs, const wchar_t* rhs) {
		SmplBuf_WChar buf;
		_SmplBuf_WChar_add_ops(buf, lhs, rhs);
		return std::move(buf);
	}
	static inline SmplBuf_WChar operator + (const wchar_t* lhs, SmplBuf_WChar&& rhs) {
		SmplBuf_WChar buf;
		_SmplBuf_WChar_add_ops(buf, lhs, rhs);
		return std::move(buf);
	}
	static inline SmplBuf_WChar operator + (const wchar_t* lhs, SmplBuf_WChar& rhs) {
		SmplBuf_WChar buf;
		_SmplBuf_WChar_add_ops(buf, lhs, rhs);
		return std::move(buf);
	}
	template <unsigned N>
	static inline SmplBuf_WChar operator + (SmplBuf_WChar&& lhs, const wchar_t(&rhs)[N]) {
		SmplBuf_WChar buf;
		_SmplBuf_WChar_add_ops(buf, lhs, rhs);
		return std::move(buf);
	}
	template <unsigned N>
	static inline SmplBuf_WChar operator + (SmplBuf_WChar& lhs, const wchar_t(&rhs)[N]) {
		SmplBuf_WChar buf;
		_SmplBuf_WChar_add_ops(buf, lhs, rhs);
		return std::move(buf);
	}
	template <unsigned N>
	static inline SmplBuf_WChar operator + (const wchar_t(&lhs)[N], SmplBuf_WChar&& rhs) {
		SmplBuf_WChar buf;
		_SmplBuf_WChar_add_ops(buf, lhs, rhs);
		return std::move(buf);
	}
	template <unsigned N>
	static inline SmplBuf_WChar operator + (const wchar_t(&lhs)[N], SmplBuf_WChar& rhs) {
		SmplBuf_WChar buf;
		_SmplBuf_WChar_add_ops(buf, lhs, rhs);
		return std::move(buf);
	}



	/**
	 * @fn	static inline SmplBuf_WChar& operator << (SmplBuf_WChar& lhs, const char_t* rhs)
	 *
	 * @brief	copy rhs from lhs
	 * 			note: the rhs assumes UTF8 chars sequence
	 *
	 * @param [in,out]	lhs	The left hand side.
	 * @param 		  	rhs	The right hand side.
	 *
	 * @returns	The result of the operation.
	 */
	static inline SmplBuf_WChar& operator << (SmplBuf_WChar& lhs, const char_t* rhs) {
		auto p = rhs;

		Unicode_UTF8Converter uc;

		while (*p != 0x00) {
			if (auto c = uc(*p)) {
				lhs.push_back(c);
			}
			++p;
		}

		return lhs;
	}

	/**
	 * @fn	static inline SmplBuf_WChar& operator << (SmplBuf_WChar& lhs, SmplBuf_Byte& rhs)
	 *
	 * @brief	copy rhs from lhs
	 * 			note: the rhs assumes UTF8 chars sequence
	 *
	 * @param [in,out]	lhs	The left hand side.
	 * @param 		  	rhs	The right hand side.
	 *
	 * @returns	The result of the operation.
	 */
	template <typename SOUT, int ISSTR>
	static inline SmplBuf_WChar& operator << (SmplBuf_WChar& lhs, TWEUTILS::SimpleBuffer<uint8_t, SOUT, ISSTR>& rhs) {
		Unicode_UTF8Converter uc;

		for (auto x : rhs) {
			auto w = uc(x);
			if (w) lhs.push_back(w);
		}

		return lhs;
	}

	template <typename SOUT, int N, int ISSTR>
	static inline SmplBuf_WChar& operator << (SmplBuf_WChar& lhs, TWEUTILS::SimpleBufferL<uint8_t, N, SOUT, ISSTR>& rhs) {
		Unicode_UTF8Converter uc;

		for (auto x : rhs) {
			auto w = uc(x);
			if (w) lhs.push_back(w);
		}

		return lhs;
	}

	/**
	 * @fn	static inline void unicodeMacNormalize(SmplBuf_WChar& str)
	 *
	 * @brief	Converts combined unicode chars with Dakuten/Handakuten into a single char.
	 * 			e.g. 'HA' '[HANDAKU]' -> 'PA'.
	 *
	 * @param [in,out]	str	The string.
	 */
	static inline void unicodeMacNormalize(SmplBuf_WChar& str) {
		SmplBuf_WChar work;
		work.reserve(str.size());

		std::copy(str.begin(), str.end(), std::back_inserter(work));

		str.resize(0);
		for (auto x : work) {
			if (str.size() == 0) {
				str.push_back(x);
			}
			else
				if (x == 12441) { // Dakuten
					str[-1] += 1;
				}
				else if (x == 12442) { // HanDakuten
					str[-1] += 2;
				}
				else {
					str.push_back(x);
				}
		}
	}

#if 0 // not tested
	inline uint16_t SJIStoJIS(uint8_t sjis_h, uint8_t sjis_l)
	{
		uint8_t jis_ku = sjis_h;
		uint8_t jis_ten = sjis_l;

		jis_ku <<= 1;
		if (jis_ten < 0x9F) {
			if (jis_ku < 0x3F) jis_ku += 0x1F; else jis_ku -= 0x61;
			if (jis_ten > 0x7E) jis_ten -= 0x20; else jis_ten -= 0x1F;
		}
		else {
			if (jis_ku < 0x3F) jis_ku += 0x20; else jis_ku -= 0x60;
			jis_ten -= 0x7E;
		}

		return jis_ku << 8 | jis_ten;
	}
#endif

	/** @brief	The size table jis to unicode */
	const uint16_t SIZE_tblJisToUnicode = 94 * 94;

	/**
	 * @brief	JIS to Unicode table.
	 * 			(ESP32) should not set size of array, but compiler sets adequate one considered aligning.
	 * 		   	        INDEED 94*94 causes system crash!
	 */
	extern const uint16_t tblJisToUnicode[];

	/**
	 * @fn	wchar_t SJIStoUnicode(uint8_t sjis_h, uint8_t sjis_l)
	 *
	 * @brief	Sjis to unicode
	 *
	 * @param	sjis_h	The sjis high byte.
	 * @param	sjis_l	The sjis low byte.
	 *
	 * @returns	A wchar_t.
	 */
	static inline wchar_t SJIStoUnicode(uint8_t sjis_h, uint8_t sjis_l) {
		uint8_t jis_ku = sjis_h;
		uint8_t jis_ten = sjis_l;

		jis_ku <<= 1;
		if (jis_ten < 0x9F) {
			if (jis_ku < 0x3F) jis_ku += 0x1F; else jis_ku -= 0x61;
			if (jis_ten > 0x7E) jis_ten -= 0x20; else jis_ten -= 0x1F;
		}
		else {
			if (jis_ku < 0x3F) jis_ku += 0x20; else jis_ku -= 0x60;
			jis_ten -= 0x7E;
		}

		int idx = (jis_ku - 0x21) * 94 + (jis_ten - 0x21);

		wchar_t wc = 0;
		if (idx >= 0 && idx < 94 * 94) {
			wc = tblJisToUnicode[idx];
		}

		return wc;
	}


	/**
	 * @fn	extern int appendSjis(SmplBuf_WChar& str, const char* sjis);
	 *
	 * @brief	Appends the sjis chars into str.
	 *
	 * @param [in,out]	str 	The string.
	 * @param 		  	sjis	The sjis string.
	 *
	 * @returns	An int.
	 */
	extern int appendSjis(SmplBuf_WChar& str, const char* sjis);
}