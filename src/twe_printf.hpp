/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

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

namespace TWE {
	int fPrintf(TWE::IStreamOut& fp, const char* format, ...);
	int snPrintf(char* buffer, size_t count, const char* format, ...);

	/// <summary>
	/// printfmt stream output class
	///  - will work on 8/16/32/64bit arguments including double.
	///  - save the arguments upto 4 items at constructor.
	/// </summary>
	class printfmt {
		static const int MAXARGS = 4;
		const char* _fmt;
		uint64_t _argv[MAXARGS];
		int _type;
		int _argc;

		// on finish.
		void save_args() {}

		// float should be passed by double.
		template <typename... Tail>
		void save_args(float head, Tail... tail) {
			double d = head;
			_argv[_argc] = *(uint64_t*)&d;
			_type |= 1 << _argc;
			_argc++;

			// get more parameters recursively, split head and tail.
			save_args(std::forward<Tail>(tail)...);
		}

		// for double
		template <typename... Tail>
		void save_args(double head, Tail... tail) {
			_argv[_argc] = *(uint64_t*)&head;
			_type |= 1 << _argc;
			_argc++;

			// get more parameters recursively, split head and tail.
			save_args(std::forward<Tail>(tail)...);
		}
		
		template <typename... Tail>
		void save_args(int64_t head, Tail... tail)
		{
			_argv[_argc] = (uint64_t)head;
			_type |= 1 << _argc;
			save_args(std::forward<Tail>(tail)...);
		}

		template <typename... Tail>
		void save_args(uint64_t head, Tail... tail)
		{
			_argv[_argc] = head;
			_type |= 1 << _argc;
			save_args(std::forward<Tail>(tail)...);
		}

		// for pointer type
		template <typename Head, typename... Tail>
		void save_args(Head* head, Tail... tail) {
			_argv[_argc] = (uint32_t)(void*)head;
			_argc++;
			save_args(std::forward<Tail>(tail)...);
		}

		// get one parameter as head, then process the rest by recursive call.
		template <typename Head, typename... Tail>
		void save_args(Head head, Tail... tail)
		{
			static_assert(
				sizeof(Head) == 4
				|| sizeof(Head) == 2
				|| sizeof(Head) == 1
				, "Unsupported type for printfmt().");

			if (0 < head) {
				// negative value of short/char needs to upscale to int32_t.
				_argv[_argc] = (uint32_t)((int32_t)head);
			}
			else {
				_argv[_argc] = head;
			}

			_argc++;

			// get more parameters recursively, split head and tail.
			save_args(std::forward<Tail>(tail)...);
		}

	public:
		// custom out function
		static inline void _out_fct(char character, void* arg) {
			IStreamOut* pof = (IStreamOut*)arg;
			*pof << (char_t)character;
		}

		// constructor with variable numbers of arguments, using parameter list.
		template <typename... Tail>
		printfmt(const char* fmt, Tail&&... tail) : _argv{ 0 }, _argc(0), _type(0), _fmt(fmt) {
			if (sizeof...(tail)) {
				if (sizeof...(tail) > MAXARGS) {
					_argc = -1;
				}
				else {
					save_args(tail...);
				}
			}
		}

		// the output call (hmmm, there would be better ways ;-()
		IStreamOut& operator ()(IStreamOut& of) {
			if (_argc == -1) {
				fctprintf(&_out_fct, (void*)&of, "(arg err)");
			}
			else {
#ifdef IS32BIT
				// should not pass 32bit or less arguments as 64bit.
				switch (_type) {
				case   0:	fctprintf(&_out_fct, (void*)&of, _fmt,  (int32_t)_argv[0],  (int32_t)_argv[1],  (int32_t)_argv[2],  (int32_t)_argv[3]); break;
				case   1:	fctprintf(&_out_fct, (void*)&of, _fmt,           _argv[0],  (int32_t)_argv[1],  (int32_t)_argv[2],  (int32_t)_argv[3]); break;
				case   2:	fctprintf(&_out_fct, (void*)&of, _fmt,  (int32_t)_argv[0],           _argv[1],  (int32_t)_argv[2],  (int32_t)_argv[3]); break;
				case   3:	fctprintf(&_out_fct, (void*)&of, _fmt,           _argv[0],           _argv[1],  (int32_t)_argv[2],  (int32_t)_argv[3]); break;
				case   4:	fctprintf(&_out_fct, (void*)&of, _fmt,  (int32_t)_argv[0],  (int32_t)_argv[1],           _argv[2],  (int32_t)_argv[3]); break;
				case   5:	fctprintf(&_out_fct, (void*)&of, _fmt,           _argv[0],  (int32_t)_argv[1],           _argv[2],  (int32_t)_argv[3]); break;
				case   6:	fctprintf(&_out_fct, (void*)&of, _fmt,  (int32_t)_argv[0],           _argv[1],           _argv[2],  (int32_t)_argv[3]); break;
				case   7:	fctprintf(&_out_fct, (void*)&of, _fmt,           _argv[0],           _argv[1],           _argv[2],  (int32_t)_argv[3]); break;
				case   8:	fctprintf(&_out_fct, (void*)&of, _fmt,  (int32_t)_argv[0],  (int32_t)_argv[1],  (int32_t)_argv[2],           _argv[3]); break;
				case   9:	fctprintf(&_out_fct, (void*)&of, _fmt,           _argv[0],  (int32_t)_argv[1],  (int32_t)_argv[2],           _argv[3]); break;
				case  10:	fctprintf(&_out_fct, (void*)&of, _fmt,  (int32_t)_argv[0],           _argv[1],  (int32_t)_argv[2],           _argv[3]); break;
				case  11:	fctprintf(&_out_fct, (void*)&of, _fmt,           _argv[0],           _argv[1],  (int32_t)_argv[2],           _argv[3]); break;
				case  12:	fctprintf(&_out_fct, (void*)&of, _fmt,  (int32_t)_argv[0],  (int32_t)_argv[1],           _argv[2],           _argv[3]); break;
				case  13:	fctprintf(&_out_fct, (void*)&of, _fmt,           _argv[0],  (int32_t)_argv[1],           _argv[2],           _argv[3]); break;
				case  14:	fctprintf(&_out_fct, (void*)&of, _fmt,  (int32_t)_argv[0],           _argv[1],           _argv[2],           _argv[3]); break;
				case  15:	fctprintf(&_out_fct, (void*)&of, _fmt,           _argv[0],           _argv[1],           _argv[2],           _argv[3]); break;
				}
#elif defined(IS64BIT)
				// simply pass all arguments as 64bit.
				fctprintf(&_out_fct, (void*)&of, _fmt, _argv[0], _argv[1], _argv[2], _argv[3]);
#endif
			}

			return of;
		}
	};


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
