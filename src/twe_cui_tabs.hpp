#pragma once

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_appdefs.hpp"
#include "twe_utils.hpp"
#include "twe_utils_simplebuffer.hpp"
#include "twe_console.hpp"

#include "twe_cui_widget.hpp"
#include "twe_cui_button.hpp"
#include "twe_cui_keyboard.hpp"

#include <utility>

namespace TWECUI {

	/**
	 * @class	ITWE_WidEv_Button
	 *
	 * @brief	To call event callback function associated with class object.
	 */
	class ITWE_WidEv_Tab {
	protected:
		void* _obj;

	public:
		ITWE_WidEv_Tab(void* obj) : _obj(obj) {}
		virtual ~ITWE_WidEv_Tab() {}
		virtual void call_ev_tab_press(int idx) = 0;
	};


	/**
	 * @class	TWE_WidEv_Button
	 *
	 * @brief	To call event callback function in member function of class T.
	 *
	 * @tparam	T	Generic type parameter.
	 */
	template <class T>
	class TWE_WidEv_Tab : public ITWE_WidEv_Tab {
		typedef void (T::* tpf_func_handler)(typename TWE::APP_HNDLR<T>::event_type ev, typename TWE::APP_HNDLR<T>::arg_type evarg);

	public:
		TWE_WidEv_Tab(T& app) : ITWE_WidEv_Tab((void*)&app) {}

		/**
		 * @fn	void TWE_WidEv_Tab::call_ev_tab_press()
		 *
		 * @brief	When a tab is pressed, switch app handler.
		 */
		void call_ev_tab_press(int idx) {
			if (_obj) {
				T* pobj = reinterpret_cast<T*>(_obj);

				if (pobj && idx < _hndlrs.size() && _hndlrs[idx]) {
					pobj->new_hndlr(_hndlrs[idx]); // add handler
				}
			}
		};

		TWEUTILS::SimpleBuffer<tpf_func_handler> _hndlrs;
	};


	class TWE_WidSet_Tabs {
		TWEUTILS::SimpleBuffer<upTWE_Button> _tabs;
		
		TWETERM::ITerm& _trm;
		std::unique_ptr<ITWE_WidEv_Tab> _upapp;

		int8_t _selection_request;
		int8_t _selected_index;
		uint8_t _col_end;

	public:
		template <class T>
		TWE_WidSet_Tabs(T& app, TWETERM::ITerm& trm) : _upapp(new TWE_WidEv_Tab<T>(app)), _trm(trm), _col_end(2), _selected_index(0), _selection_request(-1)
		{
			_tabs.reserve(16);
			static_cast<TWE_WidEv_Tab<T>*>(_upapp.get())->_hndlrs.reserve(16);
		}
		~TWE_WidSet_Tabs() {}

		/**
		 * @fn	int TWE_WidSet_Tabs::add(const wchar_t* lbl, tpf_func_handler hndlr)
		 *
		 * @brief	Adds a tab
		 *
		 * @param	lbl  	The label string.
		 * @param	hndlr	The hndlr of tab.
		 *
		 * @returns	Tab index number (0..MAXTABs-1)
		 */

		template <class T>
		int add(const wchar_t* lbl, void (T::* hndlr)(typename TWE::APP_HNDLR<T>::event_type ev, typename TWE::APP_HNDLR<T>::arg_type evarg)) {
			// check if _upapp has same class object of T.
			TWE_WidEv_Tab<T>* pobj = nullptr;
#ifdef TWE_USE_RTTI
			// type check (if T is differ, no action)
			pobj = dynamic_cast<TWE_WidEv_Tab<T>*>(_upapp.get());
			if (!pobj) return -1;
#else
			pobj = static_cast<TWE_WidEv_Tab<T>*>(_upapp.get());
#endif

			TWEUTILS::SmplBuf_WCharL<32> wlbl;
			wlbl.get() << L'/' << lbl << L'\\';

			_tabs.push_back(TWECUI::upTWE_Button(new TWE_Button(_col_end, 0, wlbl.c_str())));
			_tabs[-1]->attach_term(_trm);
			_tabs[-1]->set_mouse_down_to_selcomp();

			pobj->_hndlrs.push_back(hndlr);
			if (pobj->_hndlrs.size() == 1) {
				_upapp->call_ev_tab_press(0);
			}

			_col_end += TWEUTILS::strlen_vis(wlbl.c_str());

			return _tabs.size() - 1;
		}

		/**
		 * @fn	void select(int i)
		 *
		 * @brief	Selects tabs
		 * 			actual switching is on check_events()
		 *
		 * @param	i	Zero-based index of the.
		 */
		void select(int i) {
			_selection_request = i;
		}

		void select_next() {
			_selection_request = _selected_index + 1;
			if (_selection_request >= _tabs.size()) _selection_request = 0;
		}

		void select_prev() {
			_selection_request = _selected_index - 1;
			if (_selection_request < 0) _selection_request = _tabs.size() - 1;
		}

		/**
		 * @fn	bool TWE_WidSet_Tabs::check_events()
		 *
		 * @brief	Check keyboard(mouse) event queue and handle them.
		 *
		 * @returns	True if it captures press event.
		 */
		bool check_events() {
			// if tab selection request is placed, switch here.
			if (_selection_request >= 0) {
				if (_selection_request >= 0 && _selection_request < _tabs.size()) {
					_selected_index = _selection_request;
					_upapp->call_ev_tab_press(_selection_request);
					update_view();
				}

				_selection_request = -1;
			}

			// event loop
			bool ret = false;
			bool b_exit_loop = true;

			do {
				int c = TWE::the_keyboard.peek_a_byte();

				for (unsigned i = 0; i < _tabs.size(); i++) {
					if (_tabs[i] && _tabs[i]->key_event(c)) {
						b_exit_loop = false; // `c' is used!
						if (_tabs[i]->is_selection_completed()) {
							// tab pressed
							if (_selected_index != i) {
								_selected_index = i;

								_upapp->call_ev_tab_press(i);

								ret = true;
								update_view();
							}
						}
					}
				}

				if (c != -1 && !b_exit_loop) { // still looping
					TWE::the_keyboard.get_a_byte(); // skip this byte
					c = TWE::the_keyboard.peek_a_byte(); // next byte
				}

				if (c == -1) break;
			} while (!b_exit_loop);

			return ret;
		}

		void update_view() {
			_trm.clear_line(0);

			for (unsigned i = 0; i < _tabs.size(); i++) {
				_tabs[i]->set_color_code_normal((i == _selected_index) ? 7 : 255, (i == _selected_index) ? 1 : 255);
				_tabs[i]->update_view();
			}
		}
	};
}