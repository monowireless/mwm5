#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_stream.hpp"

#include <cstring>
#include <memory>
#include <iterator>
#include <utility>
#include <type_traits>

#ifdef _DEBUG
extern "C" int printf_(const char* format, ...); 
#endif
namespace TWEUTILS {


	/**
	 * @class	_SimpleBuffer_Dynamic
	 *
	 * @brief	A simple buffer memory allocator.
	 *
	 * @tparam	T	Generic type parameter.
	 */
	template <class T>
	class _SimpleBuffer_Dynamic {
		T* _ptr;

	public:
		_SimpleBuffer_Dynamic() : _ptr(nullptr) {}
		_SimpleBuffer_Dynamic(size_t n) : _ptr(new T[n]) {}

		T* get_pt() { return _ptr; }
	};


	/**
	 * @class	_SimpleBuffer_DummyStreamOut
	 *
	 * @brief	A simple buffer dummy stream out.
	 */
	class _SimpleBuffer_DummyStreamOut {
	public:
		_SimpleBuffer_DummyStreamOut& operator ()(char_t c) {return *this;}
		_SimpleBuffer_DummyStreamOut& write_w(wchar_t c) {return *this;}
	};

	/// <summary>
	/// 簡易バッファ
	/// このクラス自体はメモリの確保をせず、あらかじめ確保済みのメモリ領域を配列としてアクセスするための手続きを提供する。
	/// </summary>
	template <class T, class SOUT=_SimpleBuffer_DummyStreamOut, int is_string_type=0>
	class SimpleBuffer : public SOUT
	{
	public:
		typedef SimpleBuffer self_type;
		typedef uint32_t size_type;
		typedef T value_type;
		typedef T& reference;
		typedef T* pointer;

		class iterator;
		class riterator;

		class iterator
		{
		public:
			typedef iterator self_type;
			typedef T value_type;
			typedef T& reference;
			typedef T* pointer;
			typedef std::forward_iterator_tag iterator_category;
			typedef int difference_type;
			iterator(pointer ptr) : ptr_(ptr) { }
			self_type operator++(int junk) { self_type i = *this; ptr_++; return i; }
			self_type& operator++() { ++ptr_; return *this; }
			reference operator*() { return *ptr_; }
			pointer operator->() { return ptr_; }
			bool operator==(const self_type& rhs) { return ptr_ == rhs.ptr_; }
			bool operator!=(const self_type& rhs) { return ptr_ != rhs.ptr_; }
			difference_type operator-(const self_type& rhs) { return (difference_type)(ptr_ - rhs.ptr_); }
			
			self_type operator-(difference_type rhs) { self_type i(ptr_ - rhs);  return i; }
			self_type operator+(difference_type rhs) { self_type i(ptr_ + rhs);  return i; }

			pointer raw_ptr() { return ptr_; }
			// operator pointer() { return ptr_; } REMOVED, TOO DANGEROUS
		private:
			pointer ptr_;
		};

		class riterator
		{
		public:
			typedef riterator self_type;
			typedef T value_type;
			typedef T& reference;
			typedef T* pointer;
			typedef std::forward_iterator_tag iterator_category;
			typedef int difference_type;
			riterator(pointer ptr) : ptr_(ptr) { }
			self_type operator++(int junk) { self_type i = *this; ptr_--; return i; }
			self_type& operator++() { --ptr_; return *this; }
			reference operator*() { return *ptr_; }
			pointer operator->() { return ptr_; }
			bool operator==(const self_type& rhs) { return ptr_ == rhs.ptr_; }
			bool operator!=(const self_type& rhs) { return ptr_ != rhs.ptr_; }
			difference_type operator-(const self_type& rhs) { return (difference_type)(rhs.ptr_ - ptr_); }
			
			self_type operator-(difference_type rhs) { self_type i(ptr_ + rhs);  return i; }
			self_type operator+(difference_type rhs) { self_type i(ptr_ - rhs);  return i; }

			pointer raw_ptr() { return ptr_; }
			iterator get_forward() { return iterator(ptr_); }
			// operator pointer() { return ptr_; } // REMOVED, TOO DANGEROUS
		private:
			pointer ptr_;
		};

		class const_iterator
		{
		public:
			typedef const_iterator self_type;
			typedef const T value_type;
			typedef const T& reference;
			typedef const T* pointer;
			typedef int difference_type;
			typedef std::forward_iterator_tag iterator_category;
			const_iterator(pointer ptr) : ptr_(ptr) { }
			self_type operator++(int junk) { self_type i = *this; ptr_++; return i; }
			self_type& operator++() { ++ptr_; return *this; }
			reference operator*() { return *ptr_; }
			pointer operator->() { return ptr_; }
			bool operator==(const self_type& rhs) { return ptr_ == rhs.ptr_; }
			bool operator!=(const self_type& rhs) { return ptr_ != rhs.ptr_; }
			
			difference_type operator-(const self_type& rhs) { return (difference_type)(ptr_ - rhs.ptr_); }
			
			self_type operator-(difference_type rhs) { self_type i(ptr_ - rhs);  return i; }
			self_type operator+(difference_type rhs) { self_type i(ptr_ + rhs);  return i; }

			pointer raw_ptr() { return ptr_; }
			//operator const pointer() { return ptr_; } // REMOVED, TOO DANGEROUS
		private:
			pointer ptr_;
		};

		class const_riterator
		{
		public:
			typedef const_riterator self_type;
			typedef const T value_type;
			typedef const T& reference;
			typedef const T* pointer;
			typedef int difference_type;
			typedef std::forward_iterator_tag iterator_category;
			const_riterator(pointer ptr) : ptr_(ptr) { }
			self_type operator++(int junk) { self_type i = *this; ptr_--; return i; }
			self_type& operator++() { --ptr_; return *this; }
			reference operator*() { return *ptr_; }
			pointer operator->() { return ptr_; }
			bool operator==(const self_type& rhs) { return ptr_ == rhs.ptr_; }
			bool operator!=(const self_type& rhs) { return ptr_ != rhs.ptr_; }

			difference_type operator-(const self_type& rhs) { return (difference_type)(rhs.ptr_ - ptr_); }
			
			self_type operator-(difference_type rhs) { self_type i(ptr_ + rhs);  return i; }
			self_type operator+(difference_type rhs) { self_type i(ptr_ - rhs);  return i; }

			pointer raw_ptr() { return ptr_; }

			const_iterator get_forward() { return const_iterator(ptr_); }
			// operator const pointer() { return ptr_; } REMOVED, TOO DANGEROUS
		private:
			pointer ptr_;
		};

	private:
		T* _p;

		size_type _u16len;
		size_type _u16maxlen;;

		std::unique_ptr<_SimpleBuffer_Dynamic<T>> _sp;

	public:
		/// <summary>
		/// コンストラクタ
		/// </summary>
		SimpleBuffer() : _p(nullptr), _u16len(0), _u16maxlen(0), _sp() {}

		/// <summary>
		/// コンストラクタ、パラメータ全部渡し
		/// </summary>
		/// <param name="p"></param>
		/// <param name="u16len"></param>
		/// <param name="u16maxlen"></param>
		SimpleBuffer(T* p, size_type u16len, size_type u16maxlen) : _p(p), _u16len(u16len), _u16maxlen(u16maxlen), _sp() {}

		/**
		 * @fn	SimpleBuffer::SimpleBuffer(uint16_t u16maxlen)
		 *
		 * @brief	Constructor using dynamic allocation
		 *
		 * @param	u16maxlen	The maximum buffer length
		 */
		SimpleBuffer(size_type u16maxlen) : _u16len(0), _u16maxlen(u16maxlen), _sp(new _SimpleBuffer_Dynamic<T>(u16maxlen + is_string_type)) {
			_p = _sp->get_pt();
		}

		/// <summary>
		/// デストラクタ
		/// </summary>
		~SimpleBuffer() {
		}


		/**
		 * @fn	SimpleBuffer::SimpleBuffer(const SimpleBuffer<T>& ref) = delete;
		 *
		 * @brief	COPY Constructor
		 * 			not necessary so far...
		 *
		 * @param	ref	The reference.
		 */
		SimpleBuffer(const SimpleBuffer& ref)
#if 0
		{
			operator = (ref);
		}
#else
		= delete;
#endif

		/**
		 * @fn	SimpleBuffer::SimpleBuffer(const SimpleBuffer<T>&& ref)
		 *
		 * @brief	MOVE CONSTRUCTOR
		 * 			not necessary so far...
		 *
		 * @param	ref	.
		 */
		SimpleBuffer(SimpleBuffer&& ref) noexcept
		{
			operator=(std::forward<SimpleBuffer>(ref));
		}

		self_type& operator = (const self_type& ref) {
			if (ref._sp) {
				reserve_and_set_empty(ref._u16len); // if assigned, reserve minimum memory here.

				// copy buffer
				for (size_type i = 0; i < ref._u16len; i++) {
					_p[i] = ref._p[i];
				}

				// create new buffer and copy
				_u16len = ref._u16len;
			}
			else {
				_p = ref._p;
				_u16len = ref._u16len;
				_u16maxlen = ref._u16maxlen;
			}
			
			return *this;
		}


		/**
		 * @fn	template <signed N> SimpleBuffer::SimpleBuffer(const T(&ref)[N])
		 *
		 * @brief	Copy Constructor (const T(&)[])
		 *
		 */
		template <signed N>
		SimpleBuffer(const T(&ref)[N]) : _p(nullptr), _u16len(0), _u16maxlen(0), _sp() {
			operator=(ref);
		}

		/**
		 * @fn	template <signed N> SimpleBuffer(const T*)
		 *
		 * @brief	Copy Constructor (const T*)
		 *
		 */
		template <typename T_
			// ,typename = typename std::enable_if<std::is_same<const T*, typename std::decay<T_>::type>::value>::type
			, typename = typename std::enable_if<
				std::is_pointer<typename std::remove_reference<T_>::type>::value &&
				std::is_same<T,
					typename std::remove_cv<
						typename std::remove_pointer<
							typename std::remove_reference<T_>::type
						>::type
					>::type
				>::value // is_same
			>::type // enable_if
		>
		SimpleBuffer(T_ p_ref) : _p(nullptr), _u16len(0), _u16maxlen(0), _sp()
		{
			operator=(p_ref);
		}


		/**
		 * @fn	self_type& SimpleBuffer::operator= (const T(&)[])
		 *
		 * @brief	Assignment operator (const T(&)[])
		 *
		 * @param	p_ref	The reference.
		 *
		 * @returns	A shallow copy of this object.
		 */
		template <signed N>
		self_type& operator = (const T(&ref)[N]) {
			reserve_and_set_empty(N);

			for (int i = 0; i < N; i++) {
				push_back(ref[i]);
			}

			if (is_string_type) {
				// for string type, array count of ref includes NUL char,
				// so keep the last entry as is.
				// If intended to use last entry, call resize_preserving_unused() to expand last entry.
				//   e.g.) buff.resize_preserving_unused(buff.size() + 1);
				_u16len--;
			}

			return *this;
		}

		/**
		 * @fn	template <typename T_> self_type& operator= (T_ p_ref)
		 *
		 * @brief	Assignment operator of const T*
		 * 			note: if defined `operator = (const T*)', the array type `operator =(const T(&)[N])' is not called. 
		 *
		 * @returns	A shallow copy of this object.
		 */
		template <typename T_
			//,typename = typename std::enable_if<std::is_same<const T*, typename std::decay<T_>::type>::value>::type
			, typename = typename std::enable_if<
				std::is_pointer<typename std::remove_reference<T_>::type>::value &&
				std::is_same<T,
					typename std::remove_cv<
						typename std::remove_pointer<
							typename std::remove_reference<T_>::type
						>::type
					>::type
				>::value // is_same
			>::type // enable_if
		> 
		self_type& operator = (T_ p_ref)
		{
			resize(0);

			while (*p_ref != T{}) {
				push_back(*p_ref);
				++p_ref;
			}

			return *this;
		}

		/**
		 * @fn	self_type& operator = (self_type&& ref) noexcept
		 *
		 * @brief	assginment with move
		 *
		 * @returns	A shallow copy of this object.
		 */
		self_type& operator = (self_type&& ref) noexcept {
			if (ref._sp) {
				int offset = (int)(ref._p - ref._sp->get_pt());
				_sp = std::move(ref._sp);
				_p = _sp->get_pt() + offset;

				_u16len = ref._u16len;
				_u16maxlen = ref._u16maxlen;

				ref._u16len = 0;
				ref._u16maxlen = 0;
				ref._p = nullptr;
			}
			else {
				// without self memory allocation, just copy the pointers.
				_u16len = ref._u16len;
				_u16maxlen = ref._u16maxlen;
				_p = ref._p;
			}

			return (*this);
		}

		/// <summary>
		/// バッファの確保サイズを変更する
		/// </summary>
		void reserve(size_type maxlen) {
			if (maxlen == 0 || _u16maxlen >= maxlen) {
				return;
			} else
			if (_p && !_sp) {
				// attaching existing memory region, no change.
			} else {
				std::unique_ptr<_SimpleBuffer_Dynamic<T>> buff(new _SimpleBuffer_Dynamic<T>(maxlen+is_string_type));
				_u16maxlen = maxlen;

				if (_u16len) {
					auto _p_new = buff->get_pt();
					for (size_type i = 0; i < _u16len; i++) {
						_p_new[i] = std::move(_p[i]);
					}
				}

				_sp = std::move(buff);
				_p = _sp->get_pt();
			}
		}

		void reserve_and_set_empty(size_type maxlen) {
			reserve(maxlen);
			resize(0);
		}

		/**
		 * @fn	inline void attach(T* p, size_type l, size_type lm)
		 *
		 * @brief	reattach the buffer pointer
		 * 			NOTE: it's NOT safer operation.
		 *
		 * @param [in,out]	p 	.
		 * @param 		  	l 	.
		 * @param 		  	lm	.
		 *
		 * ### returns	.
		 */
		inline void attach(T* p, size_type l, size_type lm) {
			_p = p;
			_u16len = l;
			_u16maxlen = lm;
		}

		/**
		 * @fn	inline void self_attach_trim_head(size_type n)
		 *
		 * @brief	Self attach, cutting head
		 * 			NOTE: it's destructive!
		 *
		 * @param	n	A size_type to process.
		 */
		inline iterator self_attach_headcut(size_type n) {
			if (n >= _u16len) {
				_p = _p + _u16len;
				_u16len = 0;
				_u16maxlen -= _u16len;
			}
			else {
				_p = _p + n;
				_u16len -= n;
				_u16maxlen -= n;
			}

			return iterator(_p);
		}

		/*
		 * @fn	inline iterator SimpleBuffer::self_attach_restore()
		 *
		 * @brief	restore self-attached object to the original.
		 *
		 * @returns	An iterator.
		 */
		inline iterator self_attach_restore(iterator _p_orig = iterator(nullptr)) {
			if (_sp) {
				_p_orig = _sp.get_pt();
			}
			int d = _p - _p_orig;

			_p = _p_orig;
			_u16len += d;
			_u16maxlen += d;
			
			return iterator(_p);
		}

		/// <summary>
		/// 先頭
		/// </summary>
		/// <returns></returns>
		inline iterator begin() { return iterator(_p); }
		inline const_iterator begin() const { return const_iterator(_p); }
		
		inline riterator rbegin() { return riterator(_p + _u16len - 1); }
		inline const_iterator cbegin() const { return const_iterator(_p); }
		inline const_riterator crbegin() const { return const_riterator(_p + _u16len - 1); }

		/// <summary>
		/// 終端（実データ）
		/// </summary>
		/// <returns></returns>
		inline iterator end() { return iterator(_p + _u16len); }
		inline const_iterator end() const { return const_iterator(_p + _u16len); }

		inline riterator rend() { return riterator(_p - 1); }
		inline const_iterator cend() const { return const_iterator(_p + _u16len); }
		inline const_riterator crend() const { return const_riterator(_p - 1); }

		inline T* data() { return _p; }
		inline const T* data() const { return _p; }

		/// <summary>
		/// １バイト追加
		/// </summary>
		/// <param name="c"></param>
		/// <returns></returns>
		inline bool append(T&& c) {
			if (_u16len < _u16maxlen) {
				_p[_u16len++] = std::forward<T>(c);
				return true;
			}
			else {
				return false;
			}
		}
		inline bool append(const T& c) {
			if (_u16len < _u16maxlen) {
				_p[_u16len++] = c;
				return true;
			}
			else {
				return false;
			}
		}
		
		inline void push_back(T&& c) { 
			if (!append(std::forward<T>(c))) {
				reserve(_u16maxlen == 0 ? 64 : _u16maxlen + 64);
				append(std::forward<T>(c));
			}
		}
		inline void push_back(const T& c) {
			if (!append(c)) {
				reserve(_u16maxlen == 0 ? 64 : _u16maxlen + 64);
				append((c));
			}
		}
		inline self_type& operator << (T&& c) { push_back(c); return *this; }
		inline self_type& operator << (const T& c) { push_back(c); return *this; }

		// get & remove the last element.
		inline void pop_back() {
			if (_u16len > 0) {
				_u16len--;
			}
		}

		// range copy
		template <typename ITER>
		inline self_type& operator << (std::pair<ITER, ITER> range) {
			auto p = range.first;
			while (p != range.second) {
				push_back(*p);
				++p;
			}
			return *this;
		}

		/**
		 * @fn	inline bool SimpleBuffer::append(self_type& a)
		 *
		 * @brief	Appends an array (same type)
		 *
		 * @param [in,out]	a	a to append.
		 *
		 * @returns	True if it succeeds, false if it fails.
		 */
		inline bool append(self_type& a) {
			bool ret = true;
			for (T&& x : a) {
				ret &= append(x);
			}
			return ret;
		}

		/// <summary>
		/// 配列の有効データ長
		/// </summary>
		/// <returns></returns>
		inline size_type length() const { return _u16len; }
		inline size_type size() const { return length(); }
		inline bool empty() const { return _u16len == 0; }

		/// <summary>
		/// 配列の最大バッファサイズ
		/// </summary>
		/// <returns></returns>
		inline size_type length_max() const { return _u16maxlen; }
		inline size_type capacity() const { return length_max(); }

		/// <summary>
		/// 使用区画のサイズを変更する
		/// </summary>
		/// <param name="len">新しいサイズ</param>
		/// <returns></returns>
		inline bool redim(size_type len) {
			if (len <= _u16maxlen) {
				if (len >= _u16len) {
					// 後ろをクリア
					T* p = _p + _u16len;
					T* e = _p + len;
					while (p < e) {
						(*p) = T{};
						p++;
					}
				}
				_u16len = len;
				return true;
			}
			else {
				return false;
			}
		}

		inline bool resize(size_type len) { return redim(len); }
		inline bool resize_preserving_unused(size_type len) {
			reserve(len);

			if (len <= _u16maxlen) {
				_u16len = len;
				return true;
			}
			else {
				return false;
			}
		}

		inline void clear() {
			resize(0);
		}

		/**
		 * @fn	inline self_type& SimpleBuffer::emptify(size_type len_reserve=0)
		 *
		 * @brief	Set this object empty(clear()) and reserve give counts.
		 *
		 * @param	len_reserve	(Optional) The length reserve.
		 *
		 * @returns	A reference to a self_type.
		 */
		inline self_type& emptify(size_type len_reserve=0) {
			resize(0);
			if (len_reserve > 0) reserve(len_reserve);

			return *this;
		}

		/// <summary>
		/// 終端に来た場合（これ以上追加できない）
		/// </summary>
		/// <returns></returns>
		inline bool is_end() const {
			return (_u16len >= _u16maxlen) ? true : false;
		}

		/// <summary>
		/// []演算子、インデックスの範囲外チェックはしない。
		///  - (-1)をインデックスに与えると配列の末尾を示す
		/// </summary>
		/// <param name="i"></param>
		/// <returns></returns>
		inline T& operator [] (int i) { return (i < 0) ? _p[_u16len + i] : _p[i]; }
		inline const T& operator [] (int i) const { return (i < 0) ? _p[_u16len + i] : _p[i]; }

		/**
		 * @fn	inline T* SimpleBuffer::c_str()
		 *
		 * @brief	Gets the string
		 *
		 * @returns	Null if it fails, else a pointer to a T.
		 */
		inline T* c_str() {
			static_assert(is_string_type == 1, "c_str() allows only string type");

			if (_p) {
				_p[_u16len] = T{};
				return _p;
			}
			else return nullptr;
		}


		/**
		 * @fn	IStreamOut& SimpleBuffer::operator()(char_t c)
		 *
		 * @brief	Writes a char to stream.
		 *
		 * @param	c	A char_t to process.
		 *
		 * @returns	The result of the operation.
		 */
		inline SOUT& operator ()(char_t c);

		/**
		 * @fn	IStreamOut& SimpleBuffer::write_w(wchar_t c)
		 *
		 * @brief	Writes a wchar_t to stream using UTF-conversion.
		 *
		 * @param	c	A wchar_t to process.
		 *
		 * @returns	A reference to an IStreamOut.
		 */
		inline SOUT& write_w(wchar_t c);
	};

	template<>
	inline TWE::IStreamOut& SimpleBuffer<uint8_t, TWE::IStreamOut,1>::operator ()(char_t c) {
		push_back(c);
		return *this;
	}

	template<>
	inline TWE::IStreamOut& SimpleBuffer<uint8_t, TWE::IStreamOut,1>::write_w(wchar_t c) {
		// UTF-8 conversion
		if (c <= 0x7F) {
			push_back(uint8_t(c));
		}
		else if (c <= 0x7FF) {
			push_back(0xc0 + (c >> 6));
			push_back(0x80 + (c & 0x3F));
		}
		else {
			push_back(0xe0 + (c >> 12));
			push_back(0x80 + ((c >> 6) & 0x3F));
			push_back(0x80 + (c & 0x3F));
		}
		return *this;
	}


	/**
	 * @class	SimpleBufferL
	 *
	 * @brief	Fixed buffer wrapping SimpleBuffer, which is intended to
	 * 			use as local variable (no heap alloc).
	 *
	 * @tparam	T	Array type
	 * @tparam	N	Array size
	 */
	template <typename T, int N, class SOUT, int STR>
	class SimpleBufferL : public SOUT {
		T _a[N];
		SimpleBuffer<T, SOUT, STR> _b;

	public:
		typedef SimpleBuffer<T, SOUT, STR> cnt_type;
		typedef typename cnt_type::size_type size_type;
		typedef typename cnt_type::value_type value_type;
		typedef typename cnt_type::reference reference;
		typedef typename cnt_type::pointer pointer;

	public:
		SimpleBufferL() : _a{}, _b(_a, 0, N) { }

		/**
		 * @fn	SimpleBuffer<T>& SimpleBufferL::get()
		 *
		 * @brief	Gets the reference of wrapped class `SimpleBuffer<T>'.
		 *
		 * @returns	A reference to a SimpleBuffer&lt;T&gt;
		 */
		cnt_type& get() { return _b; }

		inline typename cnt_type::iterator begin() { return _b.begin(); }
		inline typename cnt_type::iterator end() { return _b.end(); }
		inline typename cnt_type::const_iterator begin() const { return _b.begin(); }
		inline typename cnt_type::const_iterator end() const { return _b.end(); }
		inline typename cnt_type::riterator rbegin() { return _b.rbegin(); }
		inline typename cnt_type::riterator rend() { return _b.rend(); }
		inline void push_back(const T&& c) { _b.append(c); }
		inline void push_back(const T& c) { _b.append(c); }
		inline T* data() { return _b.data(); }
		inline size_type size() { return _b.size(); }
		inline T* c_str() { return _b.c_str(); }
		inline size_type capacity() { return _b.capacity(); }
		inline T& operator [] (int i) { return _b[i]; }

		SOUT& operator ()(char_t c) {
			return _b.operator()(c);
		}

		SOUT& write_w(wchar_t c) {
			return _b.write_w(c);
		}
	};

	// typedefs
	typedef SimpleBuffer<uint8_t, _SimpleBuffer_DummyStreamOut, 1> SmplBuf_Byte;
	template <int N> using SmplBuf_ByteL = SimpleBufferL<uint8_t, N, _SimpleBuffer_DummyStreamOut, 1>;

	typedef SimpleBuffer<wchar_t, _SimpleBuffer_DummyStreamOut, 1> SmplBuf_WChar;
	template <int N> using SmplBuf_WCharL = SimpleBufferL<wchar_t, N, _SimpleBuffer_DummyStreamOut, 1>;

	typedef SimpleBuffer<uint8_t, TWE::IStreamOut, 1> SmplBuf_ByteS;
	template <int N> using SmplBuf_ByteSL = SimpleBufferL<uint8_t, N, TWE::IStreamOut, 1>;

	// some operators << to the IStreamOut.
	inline TWE::IStreamOut& operator << (TWE::IStreamOut& lhs, const SmplBuf_Byte& s) {
		for (const auto x : s) { lhs.operator ()((const char_t)x); }
		return lhs;
	}
	template <int N>
	inline TWE::IStreamOut& operator << (TWE::IStreamOut& lhs, const SmplBuf_ByteL<N>& s) {
		for (const auto x : s) { lhs.operator ()((const char_t)x); }
		return lhs;
	}
	inline TWE::IStreamOut& operator << (TWE::IStreamOut& lhs, const SmplBuf_ByteS& s) {
		for (const auto x : s) { lhs.operator ()((const char_t)x); }
		return lhs;
	}
	template <int N>
	inline TWE::IStreamOut& operator << (TWE::IStreamOut& lhs, const SmplBuf_ByteSL<N>& s) {
		for (const auto x : s) { lhs.operator ()((char_t)x); }
		return lhs;
	}
	inline TWE::IStreamOut& operator << (TWE::IStreamOut& lhs, const SmplBuf_WChar& s) {
		for (const auto x : s) { lhs.write_w(x); }
		return lhs;
	}

	inline TWE::IStreamOut& operator << (TWE::IStreamOut& lhs, const SmplBuf_WChar&& s) {
		for (const auto x : s) { lhs.write_w(x); }
		return lhs;
	}
	template <int N>
	inline TWE::IStreamOut& operator << (TWE::IStreamOut& lhs, const SmplBuf_WCharL<N>& s) {
		for (const auto x : s) { lhs.write_w(x); }
		return lhs;
	}
}
