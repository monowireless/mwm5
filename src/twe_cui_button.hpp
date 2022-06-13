#pragma once

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_utils_simplebuffer.hpp"
#include "twe_console.hpp"

#include "twe_cui_widget.hpp"
#include "twe_cui_keyboard.hpp"

#include <utility>

#ifdef TWE_USE_STD_FUNCTION
# include <functional>
#endif

namespace TWECUI {

	class TWE_Button : TWE_Widget {
	public:
		typedef uint8_t bool_type;

	private:
		uint8_t _c_vis; // the column number of button (visual: counts a wide char as two)
		uint8_t _l_vis; // the line number of button (visual: counts a wide char as two)
		uint8_t _len_vis; // button characters (visual: counts a wide char as two)
		TWEUTILS::SmplBuf_WChar _strlbl; // label chars
		TWETERM::ITerm* _pterm;

		bool_type _b_enabled; // true if this widge is enabled/visble
		bool_type _b_selected; // true if button is focused and ready to press.
		bool_type _b_dirty; // render flag

		bool_type _b_go_by_mouse_down; // if true, selection completed by MOUSE_DOWN event instead of MOUSE_UP.

		bool_type _b_sel_comp_pre; // if selection is performed, set this flag. (display as selected)
		bool_type _b_sel_comp; // after small timer(100ms), this flag will be set.
		uint32_t _tick_selected; // millis when selection is performed.

		uint8_t _col_label, _col_face; // color code of FG or BG
		uint8_t _col_focus_label, _col_focus_face; // color code of FG or BG in focus

		RectCon _additional_hot_area;

		/**
		 * @fn	inline bool TWE_Button::_is_coord_in_range(int x, int y);
		 *
		 * @brief	Is coordinate in range of this button area.
		 *
		 * @param	x	The x coordinate.
		 * @param	y	The y coordinate.
		 *
		 * @returns	True if coordinate in range, false if not.
		 */
		inline bool _is_coord_in_range(int x, int y);

	public:

		/**
		 * @fn	TWE_Button(uint8_t col, uint8_t lin, const wchar_t* lbl)
		 *
		 * @brief	Constructor
		 *
		 * @param	col visual column position of button
		 * @param	lin	visual line position of button
		 * @param	lbl	label string, which will define button length.
		 */
		TWE_Button(uint8_t col, uint8_t lin, const wchar_t* lbl, uint8_t len = 0);

		/**
		 * @fn	void attach_term(TWETERM::ITerm& trm);
		 *
		 * @brief	Attach term
		 *
		 * @param [in,out]	trm	The screen(terminal) object.
		 */
		void attach_term(TWETERM::ITerm& trm);

		/**
		 * @fn	void TWE_Button::relocate(uint8_t col, uint8_t lin);
		 *
		 * @brief	Relocates the button position
		 *
		 * @param	col	The new column number of the screen
		 * @param	lin	The new line number of the screen
		 */
		void relocate(uint8_t col, uint8_t lin);

		/**
		 * @fn	void update_view();
		 *
		 * @brief	Render button on the screen attached.
		 */
		void update_view();


		/**
		 * @fn	bool TWE_Button::key_event(TWE::keyinput_type keycode);
		 *
		 * @brief	handle key event
		 *
		 * @param	keycode	The keycode.
		 *
		 * @returns	True if keycode is handled, false if it's ignored.
		 */
		bool key_event(TWE::keyinput_type keycode);


		/**
		 * @fn	inline bool TWE_Button::is_selection_completed()
		 *
		 * @brief	Query if this object is selection completed
		 *
		 * @returns	True if selection completed, false if not.
		 */
		inline bool is_selection_completed() {
			bool ret = _b_sel_comp;
			_b_sel_comp = false;
			return ret;
		}

		/**
		 * @fn	inline bool TWE_Button::set_mouse_down_to_selcomp(bool b = false)
		 *
		 * @brief	Sets mouse down to complete selection.
		 *
		 * @param	b	(Optional) True to b.
		 *
		 * @returns	True if it succeeds, false if it fails.
		 */
		inline void set_mouse_down_to_selcomp(bool b = true) {
			_b_go_by_mouse_down = b;
		}

		/**
		 * @fn	void TWE_Button::set_color_code_normal(uint8_t fg, uint8_t bg)
		 *
		 * @brief	Sets color code (out of focus)
		 * 			available color code is from 0:black to 7:white and 255:default.
		 *
		 * @param	fg	The foreground.
		 * @param	bg	The background.
		 */
		void set_color_code_normal(uint8_t col_label, uint8_t col_back) {
			_col_label = col_label;
			_col_face = col_back;
		}

		/**
		 * @fn	void TWE_Button::set_color_code_focus(uint8_t fg, uint8_t bg)
		 *
		 * @brief	Sets color code (in focus)
		 * 			available color code is from 0:black to 7:white and 255:default.
		 *
		 * @param	fg	The foreground.
		 * @param	bg	The background.
		 */
		void set_color_code_focus(uint8_t col_label, uint8_t col_back) {
			_col_focus_label = col_label;
			_col_focus_face = col_back;
		}

		/**
		 * @fn	void TWE_Button::set_additional_hot_area(RectCon rct)
		 *
		 * @brief	Sets additional hot area
		 *
		 * @param	rct	The rectangle.
		 */
		void set_additional_hot_area(RectCon rct) {
			_additional_hot_area = rct;
		}

		/**
		 * @fn	RectCon& TWE_Button::get_additional_hot_area()
		 *
		 * @brief	Gets additional hot area
		 *
		 * @returns	The additional hot area.
		 */
		RectCon& get_additional_hot_area() {
			return _additional_hot_area;
		}


		/**
		 * @fn	TWEUTILS::SmplBuf_WChar& TWE_Button::get_label()
		 *
		 * @brief	Gets label string.
		 *
		 * @returns	Reference to the label data.
		 */
		TWEUTILS::SmplBuf_WChar& get_label() { return _strlbl; }

		/**
		 * set visibility.
		 * 
		 * \param b_visible true to visible.
		 */
		void set_visible(bool b_visible);
	};


	/**
	 * @typedef	std::unique_ptr<TWE_Button> upTWE_Button
	 *
	 * @brief	Defines an alias representing the unique pointer of TWE_Button
	 */
	typedef std::unique_ptr<TWE_Button> upTWE_Button;


	/**
	 * @class	ITWE_WidEv_Button
	 *
	 * @brief	To call event callback function associated with class object.
	 */
	class ITWE_WidEv_Button {
	protected:
		void* _obj; // the object pointer of the class instance where button callcack functions are defined.

	public:
		ITWE_WidEv_Button(void *obj) : _obj(obj) {}
		virtual ~ITWE_WidEv_Button() {}
		virtual void call_ev_btn_press(int i, int id, uint32_t opt) = 0;
		virtual void clear() = 0;
		void* _get_obj_ptr() { return _obj; }

		using func_type = std::function<void(int id, uint32_t opt)>;
	};


	/**
	 * @class	TWE_WidEv_Button
	 *
	 * @brief	To call event callback function in member function of class T.
	 *
	 * @tparam	T	Generic type parameter.
	 */
	class TWE_WidEv_Button : public ITWE_WidEv_Button {		
	public:
		template <class T>
		TWE_WidEv_Button(T& app) : ITWE_WidEv_Button((void*)&app) {}

		void call_ev_btn_press(int i, int id, uint32_t opt) {
			if (_obj && i < int(_hnd_pres.size())) {
				auto &f = _hnd_pres[i];
				f(id, opt);
			}
		}

		void clear() {
#ifndef ESP32
			_hnd_pres.clear();
#else
			_hnd_pres.resize_preserving_unused(0);
#endif
		}

		TWEUTILS::SimpleBuffer<func_type> _hnd_pres;
	};

	
	class TWE_WidSet_Buttons {
		TWEUTILS::SimpleBuffer<upTWE_Button> _btns;
		// TWEUTILS::SimpleBuffer<void *> _hnd_pres;
		TWEUTILS::SimpleBuffer<uint32_t> _opt;
		TWETERM::ITerm& _trm;
		std::unique_ptr<TWE_WidEv_Button> _upapp;
		TWE_Button _btn_null;
		
	public:
		template <class T>
		TWE_WidSet_Buttons(T& app, TWETERM::ITerm& trm) : 
			_upapp(new TWE_WidEv_Button(app))
			, _trm(trm)
			, _btn_null(0, 0, L"") // null btn for error handing.
		{
			_btns.reserve(16);
			_upapp->_hnd_pres.reserve(16);
			_opt.reserve(16);
		}
		~TWE_WidSet_Buttons() {}

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

		int add(uint8_t cols, uint8_t lines, const wchar_t* lbl, std::function<void(int id, uint32_t opt)> f, uint32_t opt = 0)
		{
			_upapp->_hnd_pres.push_back(f); // add an handler

			_btns.push_back(TWECUI::upTWE_Button(new TWE_Button(cols, lines, lbl)));
			_btns[-1]->attach_term(_trm);
			_btns[-1]->update_view();

			_opt.push_back(opt);

			return _btns.size() - 1;
		}

		template <typename T>
		int add(uint8_t cols, uint8_t lines, const wchar_t* lbl, void (T::* hndlr)(int id, uint32_t opt), T* pobj, uint32_t opt=0)
		{
				ITWE_WidEv_Button::func_type f = std::bind(hndlr, pobj, std::placeholders::_1, std::placeholders::_2);
				return add(cols, lines, lbl, f, opt);
		}


		/**
		 * @fn	void TWE_WidSet_Buttons::clear()
		 *
		 * @brief	Clears this object to its blank/initial state
		 */
		void clear() {
			_btns.clear();
			_upapp->_hnd_pres.clear();
			_opt.clear();
		}

		/**
		 * @fn	bool TWE_WidSet_Tabs::check_events()
		 *
		 * @brief	Check keyboard(mouse) event queue and handle them.
		 *
		 * @returns	True if it captures press event.
		 */
		bool check_events() {
			bool ret = false;
			bool b_exit_loop = true;

			do {
				int c = TWE::the_keyboard.peek_a_byte();

				for (unsigned i = 0; i < _btns.size(); i++) {
					if (_btns[i] && _btns[i]->key_event(c)) {
						b_exit_loop = false; // `c' is used!
						if (_btns[i]->is_selection_completed()) {
							// buttons pressed
							if (_upapp) {
								_upapp->call_ev_btn_press(i, i, _opt[i]);
							}

							ret = true;
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

		/**
		 * @fn	void TWE_WidSet_Buttons::update_view()
		 *
		 * @brief	redraw terminal
		 */
		void update_view() {
			for (unsigned i = 0; i < _btns.size(); i++) {
				_btns[i]->update_view();
			}
		}

		/**
		 * @fn	TWE_Button& TWE_WidSet_Buttons::operator[](int i)
		 *
		 * @brief	Array indexer operator
		 *
		 * @param	i	Zero-based index of the.
		 *
		 * @returns	The button object. Null button object _btn_null is returned when having an error.
		 */
		TWE_Button& operator[](int i) {
			TWE_Button* ptr = nullptr;
			if (i >= 0 && i < (int)_btns.size()) {
				if (_btns[i]) ptr = _btns[i].get();
			}

			if (ptr) return *ptr;
			else return _btn_null;
		}
	};
}