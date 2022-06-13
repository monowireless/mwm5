/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_utils_simplebuffer.hpp"
#include "twe_console.hpp"
#include "twe_cui_keyboard.hpp"
#include "twe_cui_widget.hpp"
#include "twe_cui_button.hpp"

using namespace TWECUI;

inline bool TWE_Button::_is_coord_in_range(int x, int y) {
	bool ret = false;
	if (auto&& coord = _pterm->get_term_coord_from_screen(x, y)) {
		if (coord.lin == _l_vis && coord.col >= _c_vis && coord.col < _c_vis + _len_vis) {
			ret = true;
		} else 
		if (_additional_hot_area.is_coord_in_range(uint8_t(coord.col), uint8_t(coord.lin))) {
			ret = true;
		}
	}

	return ret;
}

TWE_Button::TWE_Button(uint8_t col, uint8_t lin, const wchar_t* lbl, uint8_t len) :
	  _b_enabled(true)
	, _c_vis(col)
	, _l_vis(lin)
	, _strlbl(lbl)
	, _pterm(nullptr)
	, _b_selected(false)
	, _b_dirty(true)
	, _b_go_by_mouse_down(false)
	, _b_sel_comp_pre(false)
	, _b_sel_comp(false)
	, _tick_selected(0)
	, _col_face(255), _col_label(255)
	, _col_focus_face(255), _col_focus_label(255)
	, _additional_hot_area{0,0,0,0}
{
	_len_vis = (len == 0) ? TWEUTILS::strlen_vis(lbl) : len;
	auto p = lbl;
	unsigned ct = 0;
	while (*p != 0) {
		if (*p == '&') {
			ct++;
			break;
		}
		p++;
	}
	_len_vis -= ct;
}


void TWE_Button::attach_term(TWETERM::ITerm& trm) {
	_pterm = &trm;
}

void TWE_Button::relocate(uint8_t col, uint8_t lin) {
	if (!_pterm || !_b_enabled) return;
	if (col == _c_vis && lin == _l_vis) return; // if not moving, do nothing

	TWETERM::ITerm& t = *_pterm;

	// fill blank at the old position
	t.move_cursor(_c_vis, _l_vis);
	t << "\033[0m";
	//t << TWE::printfmt("\033[%d;%dH", _l_vis + 1, _c_vis + 1);
	for (int i = 0; i < _len_vis; i++) t << ' ';

	_l_vis = lin;
	_c_vis = col;

	update_view();
}


void TWE_Button::set_visible(bool b_vis) {
	if (!_pterm) return;
	TWETERM::ITerm& t = *_pterm;

	if (_b_enabled && !b_vis) {
		// clear the araa
		t.move_cursor(_c_vis, _l_vis);
		t << "\033[0m";
		for (int i = 0; i < _len_vis; i++) t << ' ';
	}
	if (!_b_enabled && b_vis) {
		_b_enabled = true;
		update_view();
	}

	_b_enabled = b_vis;

}

void TWE_Button::update_view() {
	if (!_pterm || !_b_enabled) return;

	TWETERM::ITerm& t = *_pterm;

	// move cursor
	t.move_cursor(_c_vis, _l_vis);
	t << "\033[7";
	if (_col_label != 255) t << TWE::printfmt(";%d", 40 + _col_label);
	if (_col_face != 255) t << TWE::printfmt(";%d", 30 + _col_face);
	t << 'm';

	// reverse, bold attr (if selected)
	if (_b_selected) {
		t << TWE::printfmt("\033[%d", (_col_label == 255) ? 41 : _col_label + 40);
		if (_col_face != 255) t << TWE::printfmt("\033[%d", _col_face + 30);

		if (_b_sel_comp_pre) t << ";1"; // bold
		t << "m";
	}

	// label
	auto p = _strlbl.begin();
	int pos_underline = -1;
	int i = 0;
	while (p != _strlbl.end()) {
		if (*p == '&') {
			i++;
			++p;

			// set underline next char of '&'.
			auto& c = t.get_char_at_cursor();
			t << *p;
			c.attr() |= TWETERM::E_ESCSEQ_UNDERLINE_MASK;
		}
		else {
			t << *p;
		}
		++p;
	}

	// reset attr
	t << "\033[0m";
}

bool TWE_Button::key_event(TWE::keyinput_type keycode) {
	if (!_b_enabled || !_pterm) return false;

	bool bHandled = false;

	// if selected and out of focus...
	if (_b_selected && !TWE_Widget::_is_focus()) {
		_b_selected = false;
		update_view();
	}

	if (_b_sel_comp_pre || _b_sel_comp) {
		/// wait for timeout (pressing)
		
		// timeout checking.
		if (!_b_sel_comp) {
			if (millis() - _tick_selected > 300) {
				_b_sel_comp = true;
				_b_sel_comp_pre = false;

				update_view();
				bHandled = true;
			}
		}

		// ignore other events as handled
		if (   TWECUI::KeyInput::MOUSE_UP::is_type(keycode)
			|| TWECUI::KeyInput::MOUSE_DOWN::is_type(keycode)
			|| KeyInput::MOUSE_MOVE::is_type(keycode)
		) {
			TWECUI::KeyInput::_MOUSE_EV ev(keycode);
			if (_is_coord_in_range(ev.get_x(), ev.get_y())) bHandled = true;
		}
	}
	else {
		/// an event handling
		
		// if pressed, set true
		bool selection_performed = false;

		// each event
		if (keycode == KeyInput::KEY_ENTER) { // PREV
			if (_b_selected) {
				selection_performed = true;
				bHandled = true;
			}
		} else
		if (KeyInput::MOUSE_MOVE::is_type(keycode)) {
			KeyInput::MOUSE_MOVE ev(keycode);

			if (_is_coord_in_range(ev.get_x(), ev.get_y())) {
				if (!_b_selected) {
					_b_selected = true;
					update_view();
				}

				TWE_Widget::_set_focus();
				bHandled = true;
			}
			else {
				// out of button area
				if (_b_selected) {
					_b_selected = false;

					update_view();
				}
			}

		}
		else
		if (TWECUI::KeyInput::MOUSE_UP::is_type(keycode) || TWECUI::KeyInput::MOUSE_DOWN::is_type(keycode)) {
			TWECUI::KeyInput::_MOUSE_EV ev(keycode);
			
			// complete selection
			if (ev.is_left_btn() && _is_coord_in_range(ev.get_x(), ev.get_y())) {
				bHandled = true;

				if ((_b_go_by_mouse_down && TWECUI::KeyInput::MOUSE_DOWN::is_type(keycode))
					|| (!_b_go_by_mouse_down && TWECUI::KeyInput::MOUSE_UP::is_type(keycode))) {
					selection_performed = true;
				}
			}
		}

		// if selected, update flag as pressed.
		if (selection_performed) {
			_b_sel_comp_pre = true;
			_tick_selected = millis();
			update_view();

			bHandled = true;
		}
	}

	return bHandled;
}