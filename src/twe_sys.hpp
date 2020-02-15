#pragma once 

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_utils.hpp"

namespace TWESYS {
	/// <summary>
	/// 時間を取得する外部関数を用意しておく
	/// </summary>
	extern uint32_t u32GetTick_ms();

	/// <summary>
	/// タイムアウト管理用
	/// </summary>
	class TimeOut {
	private:
	protected:
		uint16_t _u16Start;
		uint16_t _u16TimeOut;

	public:
		TimeOut() : _u16Start(0), _u16TimeOut(0) {}
		inline void set_timeout(uint16_t t) { _u16TimeOut = t; }
		inline void start() { if (_u16TimeOut > 0) _u16Start = (u32GetTick_ms() & 0xFFFF); }
		inline bool is_timeout() { return ((u32GetTick_ms() & 0xFFFF) - _u16Start > _u16TimeOut); }
		inline bool is_enabled() { return _u16TimeOut != 0; }
	};

	/// <summary>
	/// sleep function in ms
	/// </summary>
	extern void Sleep_ms(int ms);
}
