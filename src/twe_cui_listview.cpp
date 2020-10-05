/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_cui_listview.hpp"
#include "twe_cui_keyboard.hpp"

class keyc_find {
	int i;

public:
	keyc_find(TWE::keyinput_type keycode) {
		i = 0;
		while (TBL_KEYC_TO_INDEX[i]) { 
			if (TBL_KEYC_TO_INDEX[i] == keycode) break;
			i++;
		}
		if (TBL_KEYC_TO_INDEX[i] == 0) i = -1;
	}
	operator bool() { return !(i == -1); }
	int get() { return i; }

	static const uint8_t TBL_KEYC_TO_INDEX[];
};

const uint8_t keyc_find::TBL_KEYC_TO_INDEX[] {
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 0 };

void TWECUI::TWE_ListView::attach_term(TWETERM::ITerm& trm, bool b_status) {
	_pterm = &trm;

	_n_view_cols = trm.get_cols();
	_n_view_rows = trm.get_rows();

	_b_status_line = b_status;

	if (_b_status_line) {
		//const wchar_t LBL_PGDN[] = L" [次ページ] ";
		//const wchar_t LBL_PGUP[] = L" [前ページ] ";
		#define ATTR_IN L"" // L"\033[31;47m"
		#define ATTR_OUT L"" // L"\033[130;140m"
		#define LBL_PGDN L"▽"
		#define LBL_PGUP L"△"

		_n_view_rows--;

		// clear the line
		trm.clear_line(_n_view_rows + 1, true); // erase the line

		// the center column
		_n_status_line_center_col = _pterm->get_width() / 2 - 1;

		const int N_BLANK = 3;
		_btn_s1.reset(new TWE_Button(
			  _n_status_line_center_col + 1 + N_BLANK
			, _n_view_rows
			, ATTR_IN LBL_PGDN ATTR_OUT
			, TWEUTILS::strlen_vis(LBL_PGUP))
		);
		_btn_s1->set_mouse_down_to_selcomp();
		_btn_s1->attach_term(trm);

		_btn_s2.reset(new TWE_Button(
			  _n_status_line_center_col - TWEUTILS::strlen_vis(LBL_PGUP) - N_BLANK
			, _n_view_rows
			, ATTR_IN LBL_PGUP ATTR_OUT
			, TWEUTILS::strlen_vis(LBL_PGUP))
		);
		_btn_s2->set_mouse_down_to_selcomp();
		_btn_s2->attach_term(trm);

	}
}

void TWECUI::TWE_ListView::update_view(bool bFull, index_type sel_prev, index_type sel_now) {
	if (!_b_enabled) return;

	if (_pterm) {
		TWETERM::ITerm& t = *_pterm;

		
		if (bFull) {
			t << "\033[2J"; // clear whole area

			// need to redraw sub widget
			if (_b_status_line) {
				t.clear_line(_n_view_rows + 1, true);
				if (_btn_s1) _btn_s1->update_view();
				if (_btn_s2) _btn_s2->update_view();
			}

			if (_btn_r1) _btn_r1->update_view();
			if (_btn_r2) _btn_r2->update_view();
		}
		
		unsigned i; // don't move into for() condition.
		for (i = 0, _n_view_rows_disp = 0
			; int(i) < _n_view_rows && int(_n_view_start + i) < _list.size()
			; i++, _n_view_rows_disp++)
		{
			if (bFull || sel_prev == i || sel_now == i) {
				int item_idx = _n_view_start + i;

				t << TWE::printfmt("\033[%d;1H" "\033[K" "\033[7m%c\033[0m: "
							, i + 1 // move cursor
							, keyc_find::TBL_KEYC_TO_INDEX[i]); // shortcut key

				if (i == _n_view_selected) {
					t << (_b_sel_comp_pre ? "\033[7;1m" : "\033[7m"); // reverse
				}

				t << _list[_n_view_start + i]; // disp the text

				if (i == _n_view_selected)
					t << "\033[0m"; // reset attr

				if (_b_rspot_show && i == _n_view_selected) {
					uint8_t c_btn = t.get_width() - 1;

					if (_b_rspot_show & 0x01) {
						c_btn -= TWEUTILS::strlen_vis(_btn_r1_label.c_str());

						if (!_btn_r1) {
							// create a button at right end.
							_btn_r1.reset(new TWE_Button(c_btn, i, _btn_r1_label.c_str()));

							_btn_r1->set_mouse_down_to_selcomp();
							_btn_r1->attach_term(t);
						}
						else {
							// move button to the place.
							_btn_r1->relocate(c_btn, i);
						}
					}

					if (_b_rspot_show & 0x02) {
						c_btn = c_btn - TWEUTILS::strlen_vis(_btn_r2_label.c_str()) - 1;

						if (!_btn_r2) {
							// create a button at right end.
							_btn_r2.reset(new TWE_Button(c_btn, i, _btn_r2_label.c_str()));

							_btn_r2->set_mouse_down_to_selcomp();
							_btn_r2->attach_term(t);
						}
						else {
							// move button to the place.
							_btn_r2->relocate(c_btn, i);
						}
					}

					// redraw
					if (_btn_r1) _btn_r1->update_view();
					if (_btn_r2) _btn_r2->update_view();
				}
			}
		}

		// status line
		if (_b_status_line && (bFull || sel_prev != sel_now)) {
			t.clear_line(_n_view_rows + 1, true);

			// set display position
			t << TWE::printfmt("\033[%d;%dH", _n_view_rows + 1, (_n_status_line_center_col + 1) - 2);

			//t << TWE::printfmt("%2d/%-2d", _n_selected + 1, _list.size()); // item count / max items
			t << TWE::printfmt("%2d/%-2d", _n_selected / _n_view_rows + 1, (_list.size() - 1) / _n_view_rows + 1); // page count / max pages

			if (_btn_s1) _btn_s1->update_view();
			if (_btn_s2) _btn_s2->update_view();
		}
	}
}

TWECUI::TWE_ListView::index_type TWECUI::TWE_ListView::update_selection(index_type n_view_selection) {
	if (!_b_enabled) return -1;
	if (!_list.size()) return -1;

	int sel_prev = _n_view_selected;
	bool b_full_update = false;

	// check if it's excess lower bound
	if (n_view_selection >= _n_view_rows) {
		if (_list.size() <= _n_view_rows) { // list is within screen
			n_view_selection = _list.size() - 1;
		}
		else {
			// excess view
			_n_view_start += _n_view_rows;
			if (_n_view_start > int(_list.size() - 1)) _n_view_start = 0;
			n_view_selection = 0;

			b_full_update = true;
		}
	}

	// check if it's excess upper bound
	if (n_view_selection < 0) {
		if (_list.size() <= _n_view_rows) { // list is within screen
			n_view_selection = 0;
		} else {
			_n_view_start -= _n_view_rows;
			if (_n_view_start < 0) {
				_n_view_start = ((_list.size() - 1) / _n_view_rows) * _n_view_rows;
				n_view_selection = _list.size() - _n_view_start - 1; // the last item in the last page
			}
			else {
				n_view_selection = _n_view_rows - 1; // the last item 
			}

			b_full_update = true;
		}
	}

	if (_n_view_start + n_view_selection >= int(_list.size())) {
		// excess item count
		if (_list.size() <= _n_view_rows) { // list is within screen
			n_view_selection = _list.size() - _n_view_start - 1; // stay in the last item
		}
		else {
			n_view_selection = 0;
			_n_view_start = 0;

			b_full_update = true;
		}
	}

	_n_view_selected = n_view_selection;
	_n_selected = _n_view_start + _n_view_selected;

	update_view(b_full_update, sel_prev, _n_view_selected);

	return _n_selected;
}

TWECUI::TWE_ListView::index_type TWECUI::TWE_ListView::update_selection_absolute(index_type n_selection) {
	// calc page
	if (_n_view_rows > 0 && n_selection >= 0 && _list.size() > 0 && n_selection < (index_type)_list.size()) {
		int page = n_selection / _n_view_rows;
		_n_view_start = page * _n_view_rows;
		_n_view_selected = n_selection - _n_view_start;
		_n_selected = n_selection;
		
		// clean full
		update_view(true, -1, _n_view_selected);

		return _n_selected;
	} else {
		return -1;
	}
}


bool TWECUI::TWE_ListView::key_event(TWE::keyinput_type keycode) {
	if (!_b_enabled) return false;

	bool bHandled = false;
	int idx_keyc = -1;

	// put a event to sub widget(button)
	if (_btn_s1) {
		if (_btn_s1->key_event(keycode)) {
			// perform page down
			if (_btn_s1->is_selection_completed()) update_selection(_n_view_rows_disp + 1);
			return true;
		}
	}
	if (_btn_s2) {
		if (_btn_s2->key_event(keycode)) {
			// perform page up
			if (_btn_s2->is_selection_completed()) update_selection(-1);
			return true;
		}
	}
	if (_btn_r1) {
		if (_btn_r1->key_event(keycode)) {
			if (_btn_r1->is_selection_completed()) _b_rspot_clicked = 0x01;
			return true;
		}
	}
	if (_btn_r2) {
		if (_btn_r2->key_event(keycode)) {
			if (_btn_r2->is_selection_completed()) _b_rspot_clicked = 0x02;
			return true;
		}
	}

	// for this widget
	if (_b_sel_comp_pre || _b_sel_comp) {
		if (!_b_sel_comp) {
			if (millis() - _tick_selected > 300) {
				_b_sel_comp = true;
				bHandled = true;
			}
		}
	} else 
	if (_pterm) {
		bool selection_performed = false;
		if (auto n_sel = keyc_find(keycode)) {
			if (n_sel.get() < _n_view_rows_disp) {
				update_selection(n_sel.get());
				selection_performed = true;
			}
		} else
		if (keycode == TWECUI::KeyInput::KEY_DOWN) { // NEXT
			update_selection(_n_view_selected + 1);
			bHandled = true;
		} else
		if (keycode == TWECUI::KeyInput::KEY_PAGEDN) { // NEXT PAGE FIRST ITEM
			update_selection(_n_view_rows_disp + 1);
			bHandled = true;
		} else
		if (keycode == TWECUI::KeyInput::KEY_UP) { // PREV
			update_selection(_n_view_selected - 1);
			bHandled = true;
		} else
		if (keycode == TWECUI::KeyInput::KEY_PAGEUP) { // PREV PAGE LAST ITEM
			update_selection(-1);
			bHandled = true;
		} else
		if (keycode == TWECUI::KeyInput::KEY_ENTER) { // PREV
			if (_list.size() == 0 || _n_view_selected <= -1) {
				// DO NOTHING
				bHandled = true;
			} else {
				update_selection(_n_view_selected);
				selection_performed = true;
			}
		} else 
		if (TWECUI::KeyInput::MOUSE_MOVE::is_type(keycode)) {
			TWECUI::KeyInput::MOUSE_MOVE ev(keycode);
			if (auto&& coord = _pterm->get_term_coord_from_screen(ev.get_x(), ev.get_y())) {
				//TWETERM::the_sys_console << TWE::printfmt("{MD:(%d,%d)->(%d,%d)/", ev.get_x(), ev.get_y(), coord.col, coord.lin);
				//TWETERM::the_sys_console << TWE::printfmt("%d}", _n_view_rows_disp);

				if (coord.lin < _n_view_rows_disp) {
					index_type i_prev = _n_view_selected; 
					update_selection(index_type(coord.lin));
					if (_n_view_selected != i_prev) bHandled = true;
				}
			}
		} else
		if (TWECUI::KeyInput::MOUSE_DOWN::is_type(keycode)) {
			TWECUI::KeyInput::MOUSE_DOWN ev(keycode);
			// complete selection anyway (better with wheel operation.)

			if (auto&& coord = _pterm->get_term_coord_from_screen(ev.get_x(), ev.get_y())) {
				_n_view_selected_on_button_down = _n_view_selected;
			}

		} else 
		if (TWECUI::KeyInput::MOUSE_UP::is_type(keycode)) {
			TWECUI::KeyInput::MOUSE_UP ev(keycode);
			// complete selection anyway (better with wheel operation.)

			if (auto&& coord = _pterm->get_term_coord_from_screen(ev.get_x(), ev.get_y())) {
				// TWETERM::the_sys_console << TWE::printfmt("{%d,%d}", coord.lin, _n_view_rows_disp);

				if (_b_status_line && coord.lin == _n_view_rows_disp) {
					; // ignore click when hit on status line
				}
				else
				if (_n_view_selected != -1 && _n_view_selected == _n_view_selected_on_button_down) {
					bHandled = true;
					selection_performed = true;
				}
			}
		} else
		if (TWECUI::KeyInput::MOUSE_WHEEL::is_type(keycode)) {
			TWECUI::KeyInput::MOUSE_WHEEL ev(keycode);
			if (ev.get_y() < 0) {
				// scroll down
				update_selection(_n_view_selected + 1);
				_n_view_selected_on_button_down = _n_view_selected;
				bHandled = true;
			} else if (ev.get_y() > 0) {
				// scroll up
				update_selection(_n_view_selected - 1);
				_n_view_selected_on_button_down = _n_view_selected;
				bHandled = true;
			}
		}

		if (selection_performed) {
			if (_n_selected >= 0 && _n_selected < int(_list.size())) {
				_b_sel_comp_pre = true;
				_tick_selected = millis();
				update_view(true, -1, _n_view_selected);

				bHandled = true;
			} else {
				// might be an error
				_n_selected = -1;
				_n_view_selected = -1;
				update_selection(_n_view_selected);
			}
		}
	}
	return bHandled;
}


/**
 * @fn	void TWECUI::TWE_ListView::sort_items(bool b_nocase)
 *
 * @brief	Sort items
 *
 * @param	b_nocase	True to case insensitive.
 */
void TWECUI::TWE_ListView::sort_items(bool b_nocase) {
	if (b_nocase) {
		TWEUTILS::SmplBufStrA_Sort2_NoCase(_list, _list_sub);
	} else {
		TWEUTILS::SmplBufStrA_Sort2(_list, _list_sub);
	}
}