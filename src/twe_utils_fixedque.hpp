#pragma once

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

#include "twe_common.hpp"

#if __GNUC__
#define _GLIBCXX_DEQUE_BUF_SIZE 64 // smaller chunk for deque in GCC, 512bytes are too big for embedded systems.
#endif

#include <deque>
#include <queue>
#include <limits>

namespace TWEUTILS {

	/// <summary>
	/// Fixed size queue.
	/// </summary>
	template<typename T>
	class FixedQueue : public std::queue<T> {
		const int MAXSIZE;
		int count;

	public:
		/// <summary>
		/// construct with max size
		/// </summary>
		/// <param name="size">max size</param>
		FixedQueue(std::size_t size) : 
				count(0), 
				MAXSIZE(size), 
				std::queue<T>(std::deque<T>(MAXSIZE))
		{
			this->c.resize(0); // don't know if it's best to reserve fixed size.
		}

		/// <summary>
		/// push into this queue
		/// </summary>
		/// <param name="value">object to put</param>
		/// <returns>fail if reaches max count.</returns>
		inline bool push(T value) {
			if (is_full()) {
				return false;
			}
			else {
				std::queue<T>::push(value);
				++count;
			}
			return true;
		}

		/// <summary>
		/// pop from this queue
		/// </summary>
		inline void pop() {
			if (!std::queue<T>::empty()) {
				std::queue< T>::pop();
				--count;
			}
		}

	private:
		/// <summary>
		/// check queue full.
		/// </summary>
		/// <returns>true if full</returns>
		inline bool is_full() {
			return !(count < MAXSIZE);
		}
	};

} // TWEUTILS

