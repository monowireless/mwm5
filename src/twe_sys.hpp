#pragma once 

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if defined(_MSC_VER) || defined(__MINGW32__)
#include "windows.h"
# pragma comment(lib, "secur32.lib")
# pragma comment(lib, "winmm.lib")
# pragma comment(lib, "dmoguids.lib")
# pragma comment(lib, "wmcodecdspuuid.lib")
# pragma comment(lib, "msdmo.lib")
# pragma comment(lib, "Strmiids.lib")
#endif

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_utils.hpp"

#if defined(ESP32)
 //due to compilation conflict, avoiding include M5Stack.h here.
 //#include <M5Stack.h>
 // 
extern "C" uint32_t _u32GetTick_ms();
#else
extern uint32_t _u32TickCount_ms_on_boot;
#endif

#if defined(__APPLE__) || defined(__linux)
#include <sys/time.h>
#endif

namespace TWESYS {
	/**
	 * @fn	extern void SysInit();
	 *
	 * @brief	System initialize
	 *
	 */
	extern void SysInit();

	/**
	 * @fn	static inline uint32_t u32GetTick_ms()
	 *
	 * @brief	get tick milliseconds
	 *
	 * @returns	An uint32_t.
	 */
	static inline uint32_t u32GetTick_ms() {
#if defined(ESP32)
		return _u32GetTick_ms();
#elif defined(_MSC_VER) || defined(__MINGW32__)
		return (uint32_t)timeGetTime() - _u32TickCount_ms_on_boot;
#elif defined(__APPLE__) || defined(__linux)
		timeval time;
		gettimeofday(&time, NULL);
		long ms = (time.tv_sec * 1000) + (time.tv_usec / 1000);
		return (uint32_t)ms - _u32TickCount_ms_on_boot;
#else
# error "no u32GetTick_ms() implementation."
#endif
	}

	/**
	 * @class	TimeOut
	 *
	 * @brief	simple timeout check
	 */
	class TimeOut {
	private:
	protected:
		uint16_t _u16Start;
		uint16_t _u16TimeOut;

	public:
		TimeOut() : _u16Start(0), _u16TimeOut(0) {}
		
		inline void set_timeout(uint16_t t) {
			_u16TimeOut = t;
		}
		
		inline uint16_t get_timeout() {
			return _u16TimeOut;
		}

		inline void start() {
			if (_u16TimeOut > 0) {
				_u16Start = (u32GetTick_ms() & 0xFFFE);
				_u16Start |= 0x0001; // set start flag
			}
		}
		
		inline void start(uint16_t t) {
			set_timeout(t);
			start();
		}

		inline void stop() {
			_u16Start &= 0xFFFE; // remove start flag
		}

		inline bool is_timeout() {
			if (is_enabled()) {	
				bool b_timeout = (ms_to_fire() > _u16TimeOut);
				if (b_timeout) {
					stop();
					return true;
				}
			}
			return false;
		}

		inline uint16_t ms_to_fire() {
			return uint16_t((u32GetTick_ms() & 0xFFFE) - (_u16Start & 0xFFFE));
		}

		/**
		 * @fn	inline bool TimeOut::is_enabled()
		 *
		 * @brief	Query if this timer is started.
		 *
		 * @returns	True if enabled, false if not.
		 */
		inline bool is_enabled() {
			return ((_u16TimeOut > 0) && (_u16Start & 0x0001));
		}

		/**
		 * @fn	inline bool TimeOut::available()
		 *
		 * @brief	Query if this timer is started and has been time out.
		 * 			After checking, this timer stops.
		 *
		 * @returns	True if it succeeds, false if it fails.
		 */
		inline bool available() {
			return is_timeout();
		}
	};

	/// <summary>
	/// sleep function in ms
	/// </summary>
	extern void Sleep_ms(int ms);

	/**
	 * @fn	extern int Get_Logical_CPU_COUNT();
	 *
	 * @brief	Gets logical CPU count
	 *
	 * @returns	The logical CPU count.
	 */
	extern int Get_Logical_CPU_COUNT();

	/**
	 * @fn	extern int Get_Physical_CPU_COUNT();
	 *
	 * @brief	Gets physical CPU count
	 *
	 * @returns	The physical CPU count.
	 */
	extern int Get_Physical_CPU_COUNT();

	/**
	 * @fn	extern int Get_CPU_COUNT();
	 *
	 * @brief	Gets CPU count
	 *
	 * @returns	The CPU count.
	 */
	extern int Get_CPU_COUNT();


	// Date information with milliseconds.
	struct TweLocalTime {
		int16_t year;
		int16_t month;
		int16_t day;
		int16_t hour;
		int16_t minute;
		int16_t second;
		int16_t ms;
		uint64_t epoch;

		TweLocalTime() : year(0), month(0), day(0), hour(0), minute(0), second(0), ms(0), epoch() {}

		/** get current epoch */
		static uint64_t epoch_now();

		/** get current date,time. */
		uint64_t now();

		/** store unix epoch and update other values. */
		void set_epoch(uint64_t ep);

		/** getting unix epoch from stored value (ignore ms) */
		void get_epoch();
	};
}

#if defined(ESP32)
#elif defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)
// millis()
static inline uint32_t millis() { return TWESYS::u32GetTick_ms(); }
// delay()
static inline void delay(int ms) { TWESYS::Sleep_ms(ms); }

extern "C" volatile uint32_t u32TickCount_ms;

#endif
