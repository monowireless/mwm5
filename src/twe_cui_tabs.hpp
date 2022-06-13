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

				if (pobj && idx < int(_hndlrs.size()) && _hndlrs[idx]) {
					pobj->new_hndlr(_hndlrs[idx]); // add handler
				}
			}
		};

		TWEUTILS::SimpleBuffer<tpf_func_handler> _hndlrs;
	};


	class TWE_WidSet_Tabs {
		TWEUTILS::SimpleBuffer<upTWE_Button> _tabs;
		TWEUTILS::SimpleBuffer<upTWE_Button> _btns_dialogue; // for dialogue button.
		
		TWETERM::ITerm& _trm;
		std::unique_ptr<ITWE_WidEv_Tab> _upapp;

		int8_t _selection_request;
		int8_t _selected_index;
		uint8_t _col_end;

		uint8_t _b_dialogue;

	public:
		template <class T>
		TWE_WidSet_Tabs(T& app, TWETERM::ITerm& trm) 
			: _upapp(new TWE_WidEv_Tab<T>(app))
			, _trm(trm), _col_end(2), _selected_index(0), _selection_request(-1)
			, _b_dialogue(false)
		{
			_tabs.reserve(16);
			static_cast<TWE_WidEv_Tab<T>*>(_upapp.get())->_hndlrs.reserve(16);

			_btns_dialogue.reserve(2);
			_btns_dialogue.push_back(TWECUI::upTWE_Button(new TWE_Button(trm.get_cols() - 12, 0, L"[YES]")));
			_btns_dialogue.push_back(TWECUI::upTWE_Button(new TWE_Button(trm.get_cols() - 5, 0, L"[NO]")));

			_btns_dialogue[0]->attach_term(_trm);
			_btns_dialogue[0]->set_mouse_down_to_selcomp();

			_btns_dialogue[1]->attach_term(_trm);
			_btns_dialogue[1]->set_mouse_down_to_selcomp();
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
			if (_selection_request >= (int)_tabs.size()) _selection_request = 0;
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
		bool check_events();

		/**
		 * update screen object.
		 * 
		 */
		void update_view();

		/**
		 * returns tab columns count.
		 * 
		 * \return  tab columns count.
		 */
		unsigned size() {
			return _tabs.size();
		}
	};
}