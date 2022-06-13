/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_cui_widget.hpp"

using namespace TWECUI;

TWE_Widget* TWE_Widget::_ptr_focus_obj = nullptr; //  static member initilization


// implement TAB methods
#include "twe_cui_tabs.hpp"
bool TWE_WidSet_Tabs::check_events() {
	// if tab selection request is placed, switch here.
	if (!_b_dialogue && _selection_request >= 0) {
		if (_selection_request >= 0 && _selection_request < (int)_tabs.size()) {
			_selected_index = _selection_request;

			_selection_request = -1;

			_upapp->call_ev_tab_press(_selected_index);
			update_view();
		}

		_selection_request = -1;
	}

	// event loop
	bool ret = false;
	bool b_exit_loop = true;

	do {
		int c = TWE::the_keyboard.peek_a_byte();

		if (!_b_dialogue) {
			for (unsigned i = 0; i < _tabs.size(); i++) {
				if (_tabs[i] && _tabs[i]->key_event(c)) {
					b_exit_loop = false; // `c' is used!
					if (_tabs[i]->is_selection_completed()) {
						// tab pressed
						if (_selected_index != i) {
							_selection_request = i;

							//_upapp->call_ev_tab_press(i);
							_b_dialogue = true;
							_trm.clear_line(0);

							ret = true;
						}
					}
					update_view();
				}
			}

			if (c != -1 && !b_exit_loop) { // still looping
				TWE::the_keyboard.get_a_byte(); // skip this byte
				c = TWE::the_keyboard.peek_a_byte(); // next byte
			}

			if (c == -1) break;
		}
		else {
			bool b_next = false;
			bool b_cancel = false;
			for (unsigned i = 0; i < _btns_dialogue.size(); i++) {
				if (_btns_dialogue[i] && _btns_dialogue[i]->key_event(c)) {
					b_exit_loop = false; // `c' is used!
					if (_btns_dialogue[i]->is_selection_completed()) {
						// tab pressed
						if (i == 0) { // [YES] 
							b_next = true;
						}
						else
							if (i == 1) { // [NO]
								b_cancel = true;
							}
					}
					_btns_dialogue[i]->update_view();
				}
			}

			if (c == KeyInput::KEY_ENTER || c == 'y' || c == 'Y') {
				b_next = true;
			}
			else
				if (c == KeyInput::KEY_ESC || c == 'n' || c == 'N') {
					b_cancel = true;
				}

			if (b_next || b_cancel) {
				b_exit_loop = false; // `c' is used!

				if (b_cancel) {
					_selection_request = -1;
					_b_dialogue = false;
				}
				else {
					_b_dialogue = false;
					_selected_index = _selection_request;
					_upapp->call_ev_tab_press(_selected_index);
					_selection_request = -1;
				}
				ret = true;
				update_view();
			}

			if (c != -1 && !b_exit_loop) { // still looping
				TWE::the_keyboard.get_a_byte(); // skip this byte
				c = TWE::the_keyboard.peek_a_byte(); // next byte
			}

			if (c == -1) break;
		}
	} while (!b_exit_loop);

	return ret;
}

void TWE_WidSet_Tabs::update_view() {
	if (!_b_dialogue) {
		_trm.clear_line(0);

		for (unsigned i = 0; i < _tabs.size(); i++) {
			_tabs[i]->set_color_code_normal((i == _selected_index) ? 7 : 7/*255*/, (i == _selected_index) ? 1 : 255);
			_tabs[i]->update_view();
		}
	}
	else {
		_trm.clear_line(0);
		_trm(0, 0) << L"\033[37;40;1m\033[K"
			<< "\033[0m\033[40;31m現画面の処理を停止し\033[37;41m"
			<< _tabs[_selection_request]->get_label()
			<< "\033[40;31mに移動します\033[0m"
			;

		_btns_dialogue[0]->set_color_code_normal(1, 7);
		//_btns_dialogue[0]->set_color_code_focus(2, 7);
		_btns_dialogue[0]->update_view();
		_btns_dialogue[1]->set_color_code_normal(0, 7);
		//_btns_dialogue[1]->set_color_code_focus(2, 7);
		_btns_dialogue[1]->update_view();
	}
}

