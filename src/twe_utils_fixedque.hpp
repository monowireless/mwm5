#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"

namespace TWEUTILS {
	template <typename T>
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

	public:
		FixedQueue() : _head(0), _tail(0), _ct(0), _size(0), _p() {}

		FixedQueue(size_type n) : FixedQueue() {
			setup(n);
		}
		
		~FixedQueue() { }

		void setup(size_type n) {
			_p.reset(new T[n]);
			_size = n;
		}

		inline bool empty() {
			return _ct == 0;
		}

		inline size_type size() {
			return _ct;
		}

		inline bool is_full() {
			return _ct == _size;
		}

		inline size_type capacity() {
			return _size;
		}

		inline void clear() {
			while (!empty()) pop();
			_head = 0;
			_tail = 0;
			_ct = 0;
		}

		inline T* push_no_assign() {
			if (_ct < _size) {
				T* ptr = &_p[_head++];

				if (_head >= _size) {
					_head = 0;
				}
				_ct++;

				return ptr;
			}
			else {
				return nullptr;
			}
		}

		inline void push(T&& c) {
			if (_ct < _size) {
				_p[_head++] = std::move(c);
				if (_head >= _size) {
					_head = 0;
				}
				_ct++;
			}
		}

		inline void push(const T& c) {
			if (_ct < _size) {
				_p[_head++] = c;
				if (_head >= _size) {
					_head = 0;
				}
				_ct++;
			}
		}

		inline void pop() {
			if (_ct > 0) {
				_p[_tail] = T{};
				_tail++;

				if (_tail >= _size) {
					_tail = 0;
				}

				_ct--;
			}
		}

		inline T& front() {
			return _p[_tail];
		}

		inline T& back() {
			if (!empty()) {
				int i = (int(_head) == 0) ? int(_size) - 1 : int(_head) - 1;
				return _p[i];
			} else {
				_p[0] = T{};
				return _p[0];
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

		inline T pop_front() {
			T x = std::move(_p[_tail]);
			pop();
			return std::move(x);
		}

	};

	template <typename T>
	class InputQueue {
	protected:
		std::unique_ptr<TWEUTILS::FixedQueue<T>> _cue;
		
	public:
		InputQueue() : _cue() {}

		InputQueue(typename TWEUTILS::FixedQueue<T>::size_type n) : _cue() {
			setup(n);
		}

		void setup(typename TWEUTILS::FixedQueue<T>::size_type size) {
			_cue.reset(new TWEUTILS::FixedQueue<T>(size));
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
	};

} // TWEUTILS

