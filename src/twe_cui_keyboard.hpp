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
#ifndef ESP32
	using __keyinput_inputqueue_type = TWEUTILS::InputQueue<TWE::keyinput_type, true>;
#else
	using inputq_type = TWEUTILS::InputQueue<TWE::keyinput_type, false>;
#endif

	class KeyInput : public __keyinput_inputqueue_type, public TWE::IStreamIn {
	public:
		typedef TWE::keyinput_type keyinput_type;
		using inputq_type = __keyinput_inputqueue_type;

	public:
		KeyInput(int qsz = 32) : inputq_type(qsz) {}

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
			return inputq_type::pop_front();
		}

		/**
		 * @fn	int KeyInput::peek_a_byte()
		 *
		 * @brief	Read queue top entry without removing it.
		 *
		 * @returns	An -1: no key data available, else key/event code
		 */
		int peek_a_byte() {
			if (inputq_type::available()) {
				return inputq_type::peek();
			}
			else return -1;
		}

	public: // key table
		static const keyinput_type KEY_UP = TWEINTRCT_KEY_UP;
		static const keyinput_type KEY_DOWN = TWEINTRCT_KEY_DOWN;
		static const keyinput_type KEY_RIGHT = TWEINTRCT_KEY_RIGHT;
		static const keyinput_type KEY_LEFT = TWEINTRCT_KEY_LEFT;
		static const keyinput_type KEY_PAGEUP = 0x121;
		static const keyinput_type KEY_PAGEDN = 0x122;
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
		static const keyinput_type KEY_PRE_ESC = 0x11B;
		
		static const keyinput_type _KEY_MASK_GUI_EVENT = 0x80000000;
		static const keyinput_type _KEY_MASK_GUI_EVENT_CODE = 0x8F000000;
		static const keyinput_type _KEY_MASK_MOUSE_BUTTON = 0x10000000;
		static const keyinput_type _KEY_MOUSE_DOWN = _KEY_MASK_GUI_EVENT | 0x01000000;
		static const keyinput_type _KEY_MOUSE_UP = _KEY_MASK_GUI_EVENT | 0x02000000;
		static const keyinput_type _KEY_MOUSE_DOUBLE = _KEY_MASK_GUI_EVENT | 0x03000000;
		static const keyinput_type _KEY_MOUSE_MOVE = _KEY_MASK_GUI_EVENT | 0x04000000;
		static const keyinput_type _KEY_MOUSE_WHEEL = _KEY_MASK_GUI_EVENT | 0x05000000;

		/**
		 * @class	_MOUSE_EV
		 *
		 * @brief	base class of mouse events.
		 */
		class _MOUSE_EV {
		protected:
			keyinput_type _ev;

		public:
			_MOUSE_EV(keyinput_type ev) : _ev(ev) {}
			_MOUSE_EV() : _ev(_KEY_MASK_GUI_EVENT) {}


			/**
			 * @fn	static keyinput_type _MOUSE_EV::generate_coord(int16_t x, int16_t y)
			 *
			 * @brief	Store (x, y) coord into keyinput_type(uint32_t).
			 * 			The (x, y) coord is converted from signed 16bit to signed 12bit
			 * 			and stored lower 24bit of keyinput_type.
			 *
			 * @param	x	The x coordinate.
			 * @param	y	The y coordinate.
			 *
			 * @returns	The coordinate.
			 */
			static keyinput_type generate_coord(int16_t x, int16_t y, uint8_t btn = 0) {
				uint16_t ux = (x << 4); ux >>= 4; // convert 16bit to 12bit
				uint16_t uy = (y << 4); uy >>= 4; // convert 16bit to 12bit

				if (btn) btn = 0x1;
				
				return ux | (uint32_t(uy) << 12) | (uint32_t(btn) << 28); // store x in 0x00000FFF, y in 0x00FFF000, btn in 0x10000000
			}

			/**
			 * @fn	static int16_t _MOUSE_EV::get_x(keyinput_type ev)
			 *
			 * @brief	Get x coordinate.
			 * 			note: the x coord is stored in ev(0x00000FFF) as signed 12bits. 
			 *
			 * @param	ev	The ev.
			 *
			 * @returns	The x coordinate.
			 */
			static int16_t get_x(keyinput_type ev) {
				uint16_t u = ev & 0x0FFF; u <<= 4; // convert 12bits to full 16bits (now MSB is signed bit) by 4bits shifting.
				int16_t s = u; s >>= 4; // convert unsigned 16bit to signed 16bit and then shift 4bit back.
				return s;
			}

			/**
			 * @fn	int16_t _MOUSE_EV::get_x() const
			 *
			 * @brief	Get x coordinate (member method)
			 *
			 * @returns	The x coordinate.
			 */
			int16_t get_x() const { return get_x(_ev); }
			
			/**
			 * @fn	int16_t _MOUSE_EV::get_y() const
			 *
			 * @brief	Get y coordinate
			 *
			 * 			note: the x coord is stored in ev(0x00FFF000) as signed 12bits.
			 *
			 * @returns	The y coordinate.
			 */
			static int16_t get_y(keyinput_type ev) {
				uint16_t u = (ev & 0xFFF000) >> 12; u <<= 4;
				int16_t s = u; s >>= 4;
				return s;
			}

			/**
			 * @fn	int16_t _MOUSE_EV::get_y() const
			 *
			 * @brief	Get y coordinate (member method)
			 *
			 * @returns	The y coordinate.
			 */
			int16_t get_y() const { return get_y(_ev); }


			/**
			 * @fn	int8_t _MOUSE_EV::get_mouse_btn() const
			 *
			 * @brief	Get mouse button
			 *
			 * 			note: the x coord is stored in ev(0x10000000) as 1bit.
			 *
			 * @returns	Button Code (0: LEFT, 1:RIGHT)
			 */
			uint8_t get_mouse_btn() const {
				return get_mouse_btn(_ev);
			}
			static uint8_t get_mouse_btn(keyinput_type ev) {
				if (ev & _KEY_MASK_MOUSE_BUTTON) return 1;
				else return 0;
			}
			bool is_left_btn() { return get_mouse_btn() == 0; }
			bool is_right_btn() { return get_mouse_btn() == 1; }

			operator keyinput_type() { return _ev; }
		};


		/**
		 * @class	MOUSE_UP
		 *
		 * @brief	A mouse button up event
		 */
		class MOUSE_UP : public _MOUSE_EV {
		public:
			MOUSE_UP(keyinput_type ev) : _MOUSE_EV(ev) {}
			MOUSE_UP(int16_t x, int16_t y, uint8_t btn = 0) { _MOUSE_EV::_ev = _KEY_MOUSE_UP | _MOUSE_EV::generate_coord(x, y, btn); }
			static bool is_type(keyinput_type ev) { return (ev & _KEY_MASK_GUI_EVENT_CODE) == _KEY_MOUSE_UP; }
		};


		/**
		 * @class	MOUSE_DOWN
		 *
		 * @brief	A mouse button down event.
		 */
		class MOUSE_DOWN : public _MOUSE_EV {
		public:
			MOUSE_DOWN(keyinput_type ev) : _MOUSE_EV(ev) {}
			MOUSE_DOWN(int16_t x, int16_t y, uint8_t btn = 0) { _MOUSE_EV::_ev = _KEY_MOUSE_DOWN | _MOUSE_EV::generate_coord(x, y, btn); }
			static bool is_type(keyinput_type ev) { return (ev & _KEY_MASK_GUI_EVENT_CODE) == _KEY_MOUSE_DOWN; }
		};

		/**
		 * @class	MOUSE_DOWN
		 *
		 * @brief	A mouse button down event.
		 */
		class MOUSE_DOUBLE : public _MOUSE_EV {
		public:
			MOUSE_DOUBLE(keyinput_type ev) : _MOUSE_EV(ev) {}
			MOUSE_DOUBLE(int16_t x, int16_t y, uint8_t btn = 0) { _MOUSE_EV::_ev = _KEY_MOUSE_DOUBLE | _MOUSE_EV::generate_coord(x, y, btn); }
			static bool is_type(keyinput_type ev) { return (ev & _KEY_MASK_GUI_EVENT_CODE) == _KEY_MOUSE_DOUBLE; }
		};

		/**
		 * @class	MOUSE_MOVE
		 *
		 * @brief	A mouse pointer move event.
		 */
		class MOUSE_MOVE : public _MOUSE_EV {
		public:
			MOUSE_MOVE(keyinput_type ev) : _MOUSE_EV(ev) {}
			MOUSE_MOVE(int16_t x, int16_t y) { _MOUSE_EV::_ev = _KEY_MOUSE_MOVE | _MOUSE_EV::generate_coord(x, y); }
			static bool is_type(keyinput_type ev) { return (ev & _KEY_MASK_GUI_EVENT_CODE) == _KEY_MOUSE_MOVE; }
		};


		/**
		 * @class	MOUSE_WHEEL
		 *
		 * @brief	A mouse wheel event. Y coord represents vertical wheel.
		 */
		class MOUSE_WHEEL : public _MOUSE_EV {
		public:
			MOUSE_WHEEL(keyinput_type ev) : _MOUSE_EV(ev) {}
			MOUSE_WHEEL(int16_t x, int16_t y) { _MOUSE_EV::_ev = _KEY_MOUSE_WHEEL | _MOUSE_EV::generate_coord(0, y); }
			static bool is_type(keyinput_type ev) { return (ev & _KEY_MASK_GUI_EVENT_CODE) == _KEY_MOUSE_WHEEL; }
		};


		/* short cuts for mouse button */
		static bool is_mouse_left_up(uint32_t ev) {
			if (MOUSE_UP::is_type(ev) && _MOUSE_EV(ev).is_left_btn()) return true;
			else return false;
		}

		static bool is_mouse_left_down(uint32_t ev) {
			if (MOUSE_DOWN::is_type(ev) && _MOUSE_EV(ev).is_left_btn()) return true;
			else return false;
		}

		static bool is_mouse_right_up(uint32_t ev) {
			if (MOUSE_UP::is_type(ev) && _MOUSE_EV(ev).is_right_btn()) return true;
			else return false;
		}

		static bool is_mouse_right_down(uint32_t ev) {
			if (MOUSE_DOWN::is_type(ev) && _MOUSE_EV(ev).is_right_btn()) return true;
			else return false;
		}

		static bool is_mouse_left_double(uint32_t ev) {
			if (MOUSE_DOUBLE::is_type(ev) && _MOUSE_EV(ev).is_left_btn()) return true;
			else return false;
		}

		static bool is_mouse_right_double(uint32_t ev) {
			if (MOUSE_DOUBLE::is_type(ev) && _MOUSE_EV(ev).is_right_btn()) return true;
			else return false;
		}

		static bool is_mouse_move(uint32_t ev) {
			return MOUSE_MOVE::is_type(ev);
		}

		static bool is_mouse_wheel(uint32_t ev) {
			return MOUSE_WHEEL::is_type(ev);
		}

		static const keyinput_type KEY_VOID = -1;
	};
}

namespace TWE {
	extern TWECUI::KeyInput& the_keyboard;
	extern TWECUI::KeyInput the_sys_keyboard;
}