#pragma once

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
//#include "twe_utils.hpp"
//#include "twe_utils_simplebuffer.hpp"
//#include "twe_console.hpp"
#include "twe_cui_keyboard.hpp"

#include <utility>

namespace TWECUI {
	/**
	 * @class	TWE_Widget
	 *
	 * @brief	A twe widget base class.
	 */
	class TWE_Widget {
	public:
		typedef uint8_t bool_type;

	private:
		bool_type _b_get_focus;
		static TWE_Widget* _ptr_focus_obj;

	protected:
		void _set_focus() {
			_ptr_focus_obj = this;
		}
		
		void _unset_focus() {
			_ptr_focus_obj = nullptr;
		}

		bool _is_focus() {
			return (_ptr_focus_obj == this);
		}
		
	public:
		TWE_Widget() : _b_get_focus(false) {}
		virtual ~TWE_Widget() {}

		virtual bool key_event(TWE::keyinput_type keycode) = 0;
	};

	/**
	 * @struct	RectCon
	 *
	 * @brief	A rectangle for console.
	 */
	struct RectCon {
		uint8_t x;
		uint8_t y;
		uint8_t w;
		uint8_t h;

		bool available() { return ((w > 0) && (h > 0)); }
		bool is_coord_in_range(uint8_t x_, uint8_t y_) {
			return (x_ >= x && x_ < x + w && y_ >= y && y_ < y + h);
		}
	};
}