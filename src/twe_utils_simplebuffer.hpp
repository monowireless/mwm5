#pragma once

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

#include "twe_common.hpp"
#include <memory>

namespace TWEUTILS {

	template <class T>
	class _SimpleBuffer_Dynamic {
		T* _ptr;
		size_t _siz;

	public:
		_SimpleBuffer_Dynamic() : _ptr(nullptr), _siz(0) {}
		_SimpleBuffer_Dynamic(size_t n) : _ptr(new T[n]), _siz(n) {}
		T* get_ptr() { return _ptr; }
	};

	/// <summary>
	/// 簡易バッファ
	/// このクラス自体はメモリの確保をせず、あらかじめ確保済みのメモリ領域を配列としてアクセスするための手続きを提供する。
	/// </summary>
	template <class T>
	class SimpleBuffer
	{
	private:
		T* _p;

		uint16_t _u16len;
		uint16_t _u16maxlen;

		std::shared_ptr<_SimpleBuffer_Dynamic<T>> _sp;

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
		SimpleBuffer(T* p, uint16_t u16len, uint16_t u16maxlen) : _p(p), _u16len(u16len), _u16maxlen(u16maxlen), _sp() {}

		/**
		 * @fn	SimpleBuffer::SimpleBuffer(uint16_t u16maxlen)
		 *
		 * @brief	Constructor using dynamic allocation
		 *
		 * @param	u16maxlen	The maximum buffer length
		 */
		SimpleBuffer(uint16_t u16maxlen) : _u16len(0), _u16maxlen(u16maxlen), _sp(new _SimpleBuffer_Dynamic<T>(u16maxlen)) {
			_p = _sp->get_ptr();
		}

		/// <summary>
		/// デストラクタ
		/// </summary>
		~SimpleBuffer() {
		}

		/// <summary>
		/// コピーコンストラクタ
		/// </summary>
		/// <param name="ref"></param>
		SimpleBuffer(const SimpleBuffer<T>& ref) {
			_p = ref._p;
			_u16len = ref._u16len;
			_u16maxlen = ref._u16maxlen;
			_sp = ref._sp;
		}

		/// <summary>
		/// 代入演算子
		/// </summary>
		/// <param name="ref"></param>
		/// <returns></returns>
		SimpleBuffer<T>& operator = (const SimpleBuffer<T>& ref) {
			_p = ref._p;
			_u16len = ref._u16len;
			_u16maxlen = ref._u16maxlen;
			_sp = ref._sp;

			return (*this);
		}

		/// <summary>
		/// バッファを再アタッチ
		/// </summary>
		/// <param name="p"></param>
		/// <param name="l"></param>
		/// <param name="lm"></param>
		/// <returns></returns>
		inline void attach(T* p, uint16_t l, uint16_t lm) {
			_p = p;
			_u16len = l;
			_u16maxlen = lm;
		}

		/// <summary>
		/// 先頭
		/// </summary>
		/// <returns></returns>
		inline T* begin() { return _p; }

		/// <summary>
		/// 終端（実データ）
		/// </summary>
		/// <returns></returns>
		inline T* end() { return _p + _u16len; }

		/// <summary>
		/// １バイト追加
		/// </summary>
		/// <param name="c"></param>
		/// <returns></returns>
		inline bool append(T c) {
			if (_u16len < _u16maxlen) {
				_p[_u16len++] = c;
				return true;
			}
			else {
				return false;
			}
		}
		inline void push_back(T c) { append(c); }
		
		/// <summary>
		/// 配列の有効データ長
		/// </summary>
		/// <returns></returns>
		inline uint16_t length() { return _u16len; }
		inline uint16_t size() { return length(); }
		inline bool empty() { return _u16len == 0; }

		/// <summary>
		/// 配列の最大バッファサイズ
		/// </summary>
		/// <returns></returns>
		inline uint16_t length_max() { return _u16maxlen; }
		inline uint16_t capacity() { return length_max(); }

		/// <summary>
		/// 使用区画のサイズを変更する
		/// </summary>
		/// <param name="len">新しいサイズ</param>
		/// <returns></returns>
		inline bool redim(uint16_t len) {
			if (len < _u16maxlen) {
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
		inline bool reserve(uint16_t len) { return redim(len); }

		/// <summary>
		/// 終端に来た場合（これ以上追加できない）
		/// </summary>
		/// <returns></returns>
		inline bool is_end() {
			return (_u16len >= _u16maxlen) ? true : false;
		}

		/// <summary>
		/// []演算子、インデックスの範囲外チェックはしない。
		///  - (-1)をインデックスに与えると配列の末尾を示す
		/// </summary>
		/// <param name="i"></param>
		/// <returns></returns>
		inline T& operator [] (int i) { return (i < 0) ? _p[_u16len + i] : _p[i]; }

		/// <summary>
		/// 代入演算子(自身の配列の利用長を０に戻す）
		///   redim() にて = 0 にて型 T を初期化している。SimpleBuffer<T> の場合でも、
		///   このクラスを利用できるようにするための、初期化演算子として利用。
		/// </summary>
		inline SimpleBuffer<T>& operator = (int i) { _u16len = 0; return *this;  }
	};


} // TWEUTILS