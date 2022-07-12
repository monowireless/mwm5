/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_cui_widget.hpp"

using namespace TWECUI;

TWE_Widget* TWE_Widget::_ptr_focus_obj = nullptr; //  static member initilization


// implement TAB methods
#include "twe_cui_tabs.hpp"

static bool _b_sel_yes = true;
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
							_b_sel_yes = true;
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
			static uint32_t tick_last;
			static uint32_t i;

			uint32_t tick = millis();
			if (tick - tick_last > 100) {
				
				_trm(_b_sel_yes ? 5 : 11, 0)
					<< "\033[1;40;32m"
					<< "-\\|/"[++i % 4]
					<< "\033[0m"
					;
				_trm(_b_sel_yes ? 11 : 5, 0)
					<< "\033[1;40;32m \033[0m";

				tick_last = tick;
			}

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
						else if (i == 1) { // [NO]
							b_cancel = true;
						}
					}
					_btns_dialogue[i]->update_view();
				}
			}

			if (c == 'y' || c == 'Y') {
				_btns_dialogue[0]->force_press(); // press button
				//b_next = true;
			}
			else if (c == KeyInput::KEY_ESC || c == 'n' || c == 'N') {
				_btns_dialogue[1]->force_press(); // press button
				//b_cancel = true;
			} else
			if (c == KeyInput::KEY_ENTER) {
				int b = _b_sel_yes ? 0 : 1;
				_btns_dialogue[b]->force_press(); // press button
				//if (_b_sel_yes) b_next = true;
				//else b_cancel = true;
			} else
			if (c == KeyInput::KEY_LEFT || c == KeyInput::KEY_RIGHT || c == KeyInput::KEY_UP || c == KeyInput::KEY_DOWN) {
				_b_sel_yes = !_b_sel_yes;
				update_view();
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

				// note: clean selected status (hilighted status remains after selection)
				for (unsigned i = 0; i < _tabs.size(); i++) _tabs[i]->clear_selected_status();

				ret = true;
				update_view();
			}

			// Basically, all the events should grab here (like modal dialogues).
			//  Note: Some events COULD pass to others. (in the case that a new event is posted just after exiting this loop)
			if (c != -1) {
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
			if (i == _selected_index) {
				_tabs[i]->set_color_code_normal(7,  1);
				_tabs[i]->set_color_code_focus(7, 1);
			}
			else {
				_tabs[i]->set_color_code_normal(7, 255);
				_tabs[i]->set_color_code_focus(255, 255);
			}

			_tabs[i]->update_view();
		}
	}
	else {
		_trm.clear_line(0);
		_trm(0, 0) 
			<< L"\033[37;40;1m\033[K";
		_trm(18, 0)
			<< TWE::MLSLW(L"\033[0m\033[40;32m現画面の処理を停止し\033[37;41m", L"\033[0m\033[40;32mStop current process and move to \033[37;41m")
			<< _tabs[_selection_request]->get_label()
			<< TWE::MLSLW(L"\033[40;32mに移動します\033[0m", L"\033[40;32m.\033[0m")
			;

		int btn_id_yes = _b_sel_yes ? 0 : 1;
		
		_btns_dialogue[btn_id_yes]->set_color_code_normal(0, 2); // BLACK TEXT, GREEN FACE
		_btns_dialogue[btn_id_yes]->set_color_code_focus(7, 2);  // WHITE TEXT, GREEN FACE
		_btns_dialogue[btn_id_yes]->update_view();

		_btns_dialogue[1 - btn_id_yes]->set_color_code_normal(0, 7); // BLACK TEXT, WHITE FACE
		_btns_dialogue[1 - btn_id_yes]->update_view();
	}
}

