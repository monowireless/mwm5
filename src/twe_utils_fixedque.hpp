#pragma once

/* Copyright (C) 2019-2022 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"

#if !defined(ESP32)
# include "gen/sdl2_config.h"
# include "gen/sdl2_utils.hpp" 
#else
namespace TWE {
	static inline bool LockGuard(bool b) {
		return true;
	}
}
#endif

namespace TWEUTILS {
	template <typename T, bool bMUTEX=false>
	class FixedQueue {
	public:
		typedef uint16_t size_type;
		typedef T value_type;

	private:
		std::unique_ptr<T[]> _p;
		size_type _head;
		size_type _tail;
		size_type _ct;
		size_type _size;

		// MUTEX
#if defined(ESP32)
		const bool _mtx;
		bool LockGuard(const bool b) { return true; }
#elif MWM5_SDL2_USE_MULTITHREAD_RENDER == 1 && MWM5_USE_SDL2_MUTEX == 1
		TWE::LockGuard::mutex_type _mtx;
#elif MWM5_SDL2_USE_MULTITHREAD_RENDER == 1 && MWM5_USE_SDL2_MUTEX == 0
		TWE::LockGuard::mutex_type _mtx;
#else
		const bool _mtx = true;
#endif

	public:
		FixedQueue() : _head(0), _tail(0), _ct(0), _size(0), _p(), _mtx()
		{
			_init();
		}

		FixedQueue(size_type n) : _head(0), _tail(0), _ct(0), _size(n), _p(new T[n]), _mtx()
		{
			_init();
		}

		FixedQueue(const FixedQueue&) = delete;
		
		~FixedQueue() { }

#if 0
		void _setup(size_type n) {
			_size = n;
			_head = _tail = _ct = 0;
			_p.reset(new T[n]);
		}
#endif

		inline bool empty() {
			if (!bMUTEX) return _empty();

			bool r = false;
			if (auto l = TWE::LockGuard(_mtx)) {
				r = _empty();
			}
			return r;
		}

		inline size_type size() {
			if (!bMUTEX) return _ct;

			size_type r = 0;
			if (auto l = TWE::LockGuard(_mtx)) {
				r = _ct;
			}
			return r;
		}

		inline bool is_full() {
			if (!bMUTEX) return _is_full();

			bool r = false;
			if (auto l = TWE::LockGuard(_mtx)) {
				r = _is_full();
			}
			return r;
		}

		inline size_type capacity() {
			if (!bMUTEX) return _size;

			size_type r = 0;
			if (auto l = TWE::LockGuard(_mtx)) {
				r = _size;
			}
			return r;
		}

		inline void clear() {
			if (!bMUTEX) _clear();
			else {
				auto l = TWE::LockGuard(_mtx);
				_clear();
			}
		}

		inline T* push_no_assign() {
			if (!bMUTEX) {
				return _push_no_assign();
			}
			else {
				auto l = TWE::LockGuard(_mtx);
				return _push_no_assign();
			}
		}

		inline void push(T&& c) {
			if (!bMUTEX) _push(c);
			else {
				auto l = TWE::LockGuard(_mtx);
				_push(c);
			}
		}

		inline void push(const T& c) {
			if (!bMUTEX) _push(c);
			else {
				auto l = TWE::LockGuard(_mtx);
				_push(c);
			}
		}

		inline void push_force(T&& c) {
			if (!bMUTEX) {
				if (_is_full()) _pop();
				_push(c);
			} else {
				auto l = TWE::LockGuard(_mtx);
				if (_is_full()) _pop();
				_push(c);
			}
		}

		inline void push_force(const T& c) {
			if (!bMUTEX) {
				if (_is_full()) _pop();
				_push(c);
			}
			else {
				auto l = TWE::LockGuard(_mtx);
				if (_is_full()) _pop();
				_push(c);
			}
		}

		inline void pop() {
			if (!bMUTEX) {
				_pop();
			}
			else {
				auto l = TWE::LockGuard(_mtx);
				_pop();
			}
		}

		inline T& front() {
			if (!bMUTEX) {
				return _p[_tail];
			}
			else {
				auto l = TWE::LockGuard(_mtx);

				return _p[_tail];
			}
		}

		inline T& back() {
			if (!bMUTEX) {
				return _back();
			}
			else {
				auto l = TWE::LockGuard(_mtx);
				return _back();
			}
		}

		/**
		 * @fn	inline T& FixedQueue::operator[] (int i)
		 *
		 * @brief	Array indexer operator
		 *
		 * @param	i	index (0..size()-1 OR -1..-size())
		 * 				NOTE: no check of over ranging.
		 *
		 * @returns	The indexed value.
		 */
		inline T& operator[] (int i) {
			if (!bMUTEX) {
				return _at(i);
			} 
			else {
				auto l = TWE::LockGuard(_mtx);
				return _at(i);
			}
		}

		inline T pop_front() {
			if (!bMUTEX) {
				T x = std::move(_p[_tail]);
				_pop();
				return std::move(x);
			}
			else {
				auto l = TWE::LockGuard(_mtx);

				T x = std::move(_p[_tail]);
				_pop();
				return std::move(x);
			}
		}

	private:
		inline void _init() {
#if MWM5_SDL2_USE_MULTITHREAD_RENDER == 1 && MWM5_USE_SDL2_MUTEX == 1
			_mtx = SDL_CreateMutex();
#endif
		}

		inline bool _is_full() {
			return _ct == _size;
		}

		inline bool _empty() {
			return _ct == 0;
		}

		inline void _clear() {
			while (!_empty()) _pop();
			_head = 0;
			_tail = 0;
			_ct = 0;
		}

		inline void _pop() {
			if (_ct > 0) {
				_p[_tail] = T{};
				_tail++;

				if (_tail >= _size) {
					_tail = 0;
				}

				_ct--;
			}
		}

		inline void _push(T&& c) {
			if (_ct < _size) {
				_p[_head++] = std::move(c);
				if (_head >= _size) {
					_head = 0;
				}
				_ct++;
			}
		}

		inline void _push(const T& c) {
			if (_ct < _size) {
				_p[_head++] = c;
				if (_head >= _size) {
					_head = 0;
				}
				_ct++;
			}
		}

		inline T& _back() {
			if (!_empty()) {
				int i = (int(_head) == 0) ? int(_size) - 1 : int(_head) - 1;
				return _p[i];
			}
			else {
				_p[0] = T{};
				return _p[0];
			}
		}

		inline T* _push_no_assign() {
			T* r = nullptr;
			if (_ct < _size) {
				T* ptr = &_p[_head++];

				if (_head >= _size) {
					_head = 0;
				}
				_ct++;

				r = nullptr;
			}
			return r;
		}

		inline T& _at(int i) {
			int idx;
			if (i >= 0) { // positive index (0,..,size()-1), get from older ones
				idx = _tail + i;
				if (idx >= _size) {
					idx -= _size;
				}
			}
			else { // negative index (-1,..,-size()), get from newer ones
				idx = int(_head) + i;
				if (idx < 0) idx = _size + idx;
			}
			return _p[idx];
		}

	};

	template <typename T, bool bMUTEX=false>
	class InputQueue {
	protected:
		std::unique_ptr<TWEUTILS::FixedQueue<T, bMUTEX>> _cue;
		
	public:
		InputQueue() : _cue() {}

		InputQueue(typename TWEUTILS::FixedQueue<T, bMUTEX>::size_type n) : _cue() {
			setup(n);
		}

		void setup(typename TWEUTILS::FixedQueue<T, bMUTEX>::size_type size) {
			_cue.reset(new TWEUTILS::FixedQueue<T, bMUTEX>(size));
		}

		inline int pop_front() {
			if (!_cue) return -1;

			int ret = -1;

			if (!_cue->empty()) {
				ret = _cue->front();
				_cue->pop();
			}

			return ret;
		}

		inline void push(T c) {
			if (_cue) {
				if (is_full()) _cue->pop(); // incase buffer is overrun, remove oldest entry.
				_cue->push(c);
			}
		}

		/**
		 * @fn	inline bool InputQueue::is_full()
		 *
		 * @brief	Query if this object is full
		 *
		 * @returns	True if full, false if not.
		 */
		inline bool is_full() {
			return _cue ? _cue->is_full() : true;
		}

	public:

		/**
		 * @fn	inline int InputQueue::available()
		 *
		 * @brief	returns a size of queue. 
		 *
		 * @returns	size of queue, 0 means nothing in the queue.
		 */
		inline int available() {
			return _cue ? _cue->size() : 0;
		}

		/**
		 * @fn	inline int InputQueue::read()
		 *
		 * @brief	Read a byte from queue.
		 *
		 * @returns	An int. -1: error.
		 */
		inline int read() {
			return pop_front();
		}

		/**
		 * @fn	inline int InputQueue::peek()
		 *
		 * @brief	Returns the top-of-stack object without removing it
		 *
		 * @returns	The current top-of-stack object.
		 */
		inline int peek() {
			if (!_cue->empty()) {
				return _cue->front();
			}
			else return -1;
		}

	};

} // TWEUTILS
