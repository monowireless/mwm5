#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_utils_simplebuffer.hpp"
#include "twe_utils_fixedque.hpp"
#include "twe_stream.hpp"

#include "twe_sys.hpp"

#include "twesettings/tweinteractive_keycode.h"

namespace TWECUI {
	class KeyInput : public TWEUTILS::InputQueue<TWE::keyinput_type>, public TWE::IStreamIn {
	public:
		typedef TWE::keyinput_type keyinput_type;

	public:
		KeyInput(int qsz = 32) : TWEUTILS::InputQueue<keyinput_type>(qsz) {}

		/**
		 * @fn	virtual void KeyInput::setup() = 0;
		 *
		 * @brief	Setups this object
		 */
		virtual void setup(void *arg = nullptr) {}

		/**
		 * @fn	virtual void KeyInput::update() = 0;
		 *
		 * @brief	Implement the actual read procedure from _ps2key.
		 */
		virtual void update() {}

		/**
		 * @fn	int get_a_byte();
		 *
		 * @brief	Implement the TWE::IStreamIn::get_a_byte()
		 * 
		 * @returns read byte, -1: error
		 */
		int get_a_byte() {
			return TWEUTILS::InputQueue<keyinput_type>::pop_front();
		}

	public: // key table
		static const keyinput_type KEY_UP = TWEINTRCT_KEY_UP;
		static const keyinput_type KEY_DOWN = TWEINTRCT_KEY_DOWN;
		static const keyinput_type KEY_RIGHT = TWEINTRCT_KEY_RIGHT;
		static const keyinput_type KEY_LEFT = TWEINTRCT_KEY_LEFT;
		static const keyinput_type KEY_BUTTON_A = TWEINTRCT_KEY_BUTTON_A;
		static const keyinput_type KEY_BUTTON_B = TWEINTRCT_KEY_BUTTON_B;
		static const keyinput_type KEY_BUTTON_C = TWEINTRCT_KEY_BUTTON_C;
		static const keyinput_type KEY_BUTTON_A_LONG = TWEINTRCT_KEY_BUTTON_A_LONG;
		static const keyinput_type KEY_BUTTON_B_LONG = TWEINTRCT_KEY_BUTTON_B_LONG;
		static const keyinput_type KEY_BUTTON_C_LONG = TWEINTRCT_KEY_BUTTON_C_LONG;
		static const keyinput_type KEY_ENTER = 0x0D;
		static const keyinput_type KEY_BS = 0x08;
		static const keyinput_type KEY_SPACE = 0x20;
		static const keyinput_type KEY_ESC = 0x1B;
		static const keyinput_type KEY_VOID = -1;
	};
}

namespace TWE {
	extern TWECUI::KeyInput& the_keyboard;
	extern TWECUI::KeyInput the_sys_keyboard;
}