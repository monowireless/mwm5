/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */
#pragma once

#include <mwm5.h>

namespace TWEM5 {
	// dispatch input function
	class InputCue : public TWE::IStreamIn {
		TWEUTILS::FixedQueue<char_t> _cue;
	public:
		InputCue(size_t size) : _cue(size) {}
		inline int get_a_byte() {
			int ret = -1;
			if (!_cue.empty()) {
				ret = _cue.front();
				_cue.pop();
			}

			return ret;
		}

		inline void push_a_byte(char_t c) {
			_cue.push(c);
		}
	};

	// extern InputCue the_input_key;
	extern InputCue the_input_uart;
}