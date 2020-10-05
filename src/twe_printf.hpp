/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/** @file
 *
 * @defgroup TWESER ヘッダファイル
 *
 * シリアル等の入出力の抽象定義を行う。
 */

#ifndef TWEPRINTF_H_
#define TWEPRINTF_H_

#include "twe_common.hpp"
#include "twe_stream.hpp"

#include "printf/printf.h"
#include <utility>
#include <new>

namespace TWE {
	int fPrintf(TWE::IStreamOut& fp, const char* format, ...);
	int snPrintf(char* buffer, size_t count, const char* format, ...);

	class _printobj {
	protected:
		const char *_fmt;

		// custom out function
		static inline void _out_fct(char character, void* arg) {
			IStreamOut* pof = (IStreamOut*)arg;
			*pof << (char_t)character;
		}

	public:
		_printobj(const char* fmt) : _fmt(fmt) {}
		virtual void do_print(IStreamOut& of) {
			char_t *p = (char_t *)_fmt;
			while (*p != 0) {
				of << *p;
				++p;
			}
		};
	};

	template <typename T1>
	class _printobj_1 : public _printobj {
		T1 _a1;
	public:
		_printobj_1(const char *fmt, T1 a1) : _printobj(fmt), _a1(a1) {}
		void do_print(IStreamOut& of) {	fctprintf(&_out_fct, (void*)&of, _fmt, _a1); }
	};

	template <typename T1, typename T2>
	class _printobj_2 : public _printobj {
		T1 _a1;
		T2 _a2;
	public:
		_printobj_2(const char *fmt, T1 a1, T2 a2) : _printobj(fmt), _a1(a1), _a2(a2) {}
		void do_print(IStreamOut& of) { fctprintf(&_out_fct, (void*)&of, _fmt, _a1, _a2); }
	};

	template <typename T1, typename T2, typename T3>
	class _printobj_3 : public _printobj {
		T1 _a1;
		T2 _a2;
		T3 _a3;
	public:
		_printobj_3(const char *fmt, T1 a1, T2 a2, T3 a3) : _printobj(fmt), _a1(a1), _a2(a2), _a3(a3) {}
		void do_print(IStreamOut& of) { fctprintf(&_out_fct, (void*)&of, _fmt, _a1, _a2, _a3); }
	};

	template <typename T1, typename T2, typename T3, typename T4>
	class _printobj_4 : public _printobj {
		T1 _a1;
		T2 _a2;
		T3 _a3;
		T4 _a4;
	public:
		_printobj_4(const char *fmt, T1 a1, T2 a2, T3 a3, T4 a4) : _printobj(fmt), _a1(a1), _a2(a2), _a3(a3), _a4(a4) {}
		void do_print(IStreamOut& of) { fctprintf(&_out_fct, (void*)&of, _fmt, _a1, _a2, _a3, _a4); }
	};

#if 0
	class printfmt {
		std::unique_ptr<_printobj> _pobj;

	public:
		printfmt(const char *fmt) 
			: _pobj(new _printobj(fmt)) {}
		
		template <typename T1>
		printfmt(const char *fmt, T1 a1)
			: _pobj(new _printobj_1<T1>(fmt,a1)) {}

		template <typename T1, typename T2>
		printfmt(const char *fmt, T1 a1, T2 a2)
			: _pobj(new _printobj_2<T1, T2>(fmt,a1,a2)) {}

		template <typename T1, typename T2, typename T3>
		printfmt(const char *fmt, T1 a1, T2 a2, T3 a3)
			: _pobj(new _printobj_3<T1, T2, T3>(fmt,a1,a2,a3)) {}

		template <typename T1, typename T2, typename T3, typename T4>
		printfmt(const char *fmt, T1 a1, T2 a2, T3 a3, T4 a4)
			: _pobj(new _printobj_4<T1, T2, T3, T4>(fmt,a1,a2,a3,a4)) {}

		IStreamOut& operator ()(IStreamOut& of) {
			_pobj->do_print(of);
			return of;
		}
	};
#else

	const size_t MAX_SIZE_PRINTOBJ = sizeof(_printobj_4<double, double, double, double>);

	class printfmt {
		uint8_t _pobj[MAX_SIZE_PRINTOBJ];

	public:
		printfmt(const char* fmt) {
			(void)new ((void*)_pobj) _printobj(fmt);
		}

		template <typename T1>
		printfmt(const char* fmt, T1 a1) {
			static_assert(sizeof(_printobj_1<T1>(fmt, a1)) <= MAX_SIZE_PRINTOBJ, "Pre-alloc size overflow. Check MAX_SIZE_PRINTOBJ.");
			(void)new ((void*)_pobj) _printobj_1<T1>(fmt, a1);
		}

		template <typename T1, typename T2>
		printfmt(const char* fmt, T1 a1, T2 a2) {
			static_assert(sizeof(_printobj_2<T1, T2>) <= MAX_SIZE_PRINTOBJ, "Pre-alloc size overflow. Check MAX_SIZE_PRINTOBJ.");
			(void)new ((void*)_pobj) _printobj_2<T1, T2>(fmt, a1, a2);
		}

		template <typename T1, typename T2, typename T3>
		printfmt(const char* fmt, T1 a1, T2 a2, T3 a3) {
			static_assert(sizeof(_printobj_3<T1, T2, T3>) <= MAX_SIZE_PRINTOBJ, "Pre-alloc size overflow. Check MAX_SIZE_PRINTOBJ.");
			(void)new ((void*)_pobj) _printobj_3<T1, T2, T3>(fmt, a1, a2, a3);
		}

		template <typename T1, typename T2, typename T3, typename T4>
		printfmt(const char* fmt, T1 a1, T2 a2, T3 a3, T4 a4) {
			static_assert(sizeof(_printobj_4<T1, T2, T3, T4>) <= MAX_SIZE_PRINTOBJ, "Pre-alloc size overflow. Check MAX_SIZE_PRINTOBJ.");
			(void)new ((void*)_pobj) _printobj_4<T1, T2, T3, T4>(fmt, a1, a2, a3, a4);
		}

		IStreamOut& operator ()(IStreamOut& of) {
			reinterpret_cast<_printobj*>(_pobj)->do_print(of);
			return of;
		}
	};
#endif

	/// <summary>
	/// printfmt and IStreamOut operator.
	/// </summary>
	inline IStreamOut& operator << (IStreamOut& s, printfmt sc) { return sc(s); } // implement std::endl like object

	/// <summary>
	/// Integer output
	/// </summary>
	inline IStreamOut& operator << (IStreamOut& s, const int i) { return s << printfmt("%d", i); }

	/// <summary>
	/// Float output
	/// </summary>
	inline IStreamOut& operator << (IStreamOut& s, const double d) {
		return s << printfmt("%.3f", d);
	}
}

#endif // TWEPRINTF_H_
