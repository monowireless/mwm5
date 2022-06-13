#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_utils_simplebuffer.hpp"
#include "twe_console.hpp"
#include "twe_cui_keyboard.hpp"
#include "twe_cui_button.hpp"

#include <utility>

namespace TWECUI {
	class TWE_ListView {
	public:
		typedef int16_t index_type;
		typedef uint16_t count_type;
		typedef uint8_t bool_type;
		typedef std::pair<TWEUTILS::SmplBuf_WChar&, TWEUTILS::SmplBuf_WChar&> pair_type;

	private:
		TWEUTILS::SimpleBuffer<TWEUTILS::SmplBuf_WChar> _list;
		TWEUTILS::SimpleBuffer<TWEUTILS::SmplBuf_WChar> _list_sub;
		const uint16_t _item_label_len;

		index_type _n_selected;

		bool_type _b_sel_comp_pre; // if selection is performed, set this flag. (display as selected)
		bool_type _b_sel_comp; // after small timer(100ms), this flag will be set.

		bool_type _b_enabled; // so far not used, true if the widget is active/visible.

		bool_type _b_status_line; // true if status line is shown.
		index_type _n_status_line_center_col; // center column of status line

		bool_type _b_rspot_show; // bitmap to show bottons (b0:_btn_r1 present, b1:_btn_r2 present)
		bool_type _b_rspot_clicked; // if info area is pressed, set true.
		TWEUTILS::SmplBuf_WChar _btn_r1_label; // info area's label
		TWEUTILS::SmplBuf_WChar _btn_r2_label; // info area's label

		TWETERM::ITerm* _pterm;
		index_type _n_view_start;
		index_type _n_view_selected;
		index_type _n_view_selected_on_button_down; // last selected item by button down
		count_type _n_view_rows;                    // rows count for the view.
		count_type _n_view_row_start;               // starting line of the listView (0.., where 0 is the top of the screen)
		count_type _n_view_cols;					// cols count for the view.
		count_type _n_view_rows_disp;

		uint32_t _tick_selected; // millis when selection is performed.

		std::unique_ptr<TWE_Button> _btn_s1;
		std::unique_ptr<TWE_Button> _btn_s2;
		std::unique_ptr<TWE_Button> _btn_r1;
		std::unique_ptr<TWE_Button> _btn_r2;

	private:	
		inline index_type calc_item_from_selected() {
			return _n_view_selected == -1 ? -1 : _n_view_start + _n_view_selected;
		}

	public:
		TWE_ListView(index_type nItems, uint16_t nStrLen = 32)
			: _list(nItems), _list_sub(nItems)
			, _item_label_len(nStrLen)
			, _n_selected(-1)
			, _b_sel_comp(0)
			, _b_sel_comp_pre(0)
			, _pterm(nullptr)
			, _n_view_start(0)
			, _n_view_selected(0), _n_view_selected_on_button_down(0)
			, _n_view_rows(0), _n_view_row_start(0)
			, _n_view_cols(0)
			, _n_view_rows_disp(0)
			, _b_enabled(true)
			, _b_status_line(false), _n_status_line_center_col(0)
			, _tick_selected(0)
			, _b_rspot_show(false), _b_rspot_clicked(0x00)
			, _btn_r1_label(), _btn_r2_label()
			, _btn_s1(), _btn_s2()
			, _btn_r1(), _btn_r2()
		{
			;
		}


		/**
		 * @fn	void TWE_ListView::attach_term(TWETERM::ITerm& trm, bool b_status);
		 *
		 * @brief	Attach term
		 *
		 * @param [in,out]	trm			The trm.
		 * @param 		  	b_status	True to have status line
		 */
		void attach_term(TWETERM::ITerm& trm, bool b_status = false) {
			attach_term(trm, 0, trm.get_rows(), b_status);
		}

		void attach_term(TWETERM::ITerm& trm, uint8_t row_start, uint8_t rows, bool b_status = false);

		/* ITEMS */		
		template <typename T1>
		inline int push_back(T1&& pstr) {
			TWEUTILS::SmplBuf_WChar buf(_item_label_len), buf_sub(_item_label_len);
			buf << pstr;
			
			_list.append(std::forward<TWEUTILS::SmplBuf_WChar>(buf));
			_list_sub.append(std::forward<TWEUTILS::SmplBuf_WChar>(buf_sub));
			return _list.size() - 1; // last index
		}

		template <typename T1>
		inline int push_back(T1&& pstr, uint16_t sub_data) {
			TWEUTILS::SmplBuf_WChar buf(_item_label_len), buf_sub(2);
			buf << pstr;
			buf_sub.push_back(wchar_t(sub_data));

			_list.append(std::forward<TWEUTILS::SmplBuf_WChar>(buf));
			_list_sub.append(std::forward<TWEUTILS::SmplBuf_WChar>(buf_sub));
			return _list.size() - 1;
		}

		template <typename T1, typename T2>
		inline int push_back(T1&& pstr, T2&& pstr_sub) {
			TWEUTILS::SmplBuf_WChar buf(_item_label_len), buf_sub(_item_label_len);
			buf << pstr;
			buf_sub << pstr_sub;

			_list.append(std::forward<TWEUTILS::SmplBuf_WChar>(buf));
			_list_sub.append(std::forward<TWEUTILS::SmplBuf_WChar>(buf_sub));

			return _list.size() - 1; // last index
		}

		inline void clear() {
			_list.resize(0);
			_list_sub.resize(0);
			_n_selected = -1;
			_n_view_selected = -1;
			_b_sel_comp = false;
			_b_sel_comp_pre = false;

			_btn_r1.reset();
			_btn_r2.reset();
		}

		inline index_type get_selected_index() {
			return _n_selected;
		}

		inline index_type get_index_of_view() {
			return _n_view_selected;
		}

		inline index_type get_first_index_of_view() {
			return _n_view_start;
		}

		inline pair_type get(index_type i) {
			return std::make_pair(std::ref(_list[i]), std::ref(_list_sub[i]));
		}

		inline pair_type operator [] (index_type i) {
			return get(i);
		}

		inline pair_type get_selected() {
			return get(get_selected_index());
		}

		inline count_type size() {
			return (count_type)_list.size();
		}

		inline bool available() {
			return is_selection_completed();
		}

		inline bool is_selection_completed() {
			bool ret = _b_sel_comp;
			_b_sel_comp = false;
			return ret;
		}


		/**
		 * @fn	void TWE_ListView::set_info_area(const wchar_t *pstr1, const wchar_t *pstr2 = nullptr)
		 *
		 * @brief	Add buttons on the selected item at rightmost.
		 *
		 * @param	pstr1	The first string pointer (if set, enable the first button)
		 * @param	pstr2	(Optional) The second string pointer (if set, enable the second button)
		 */
		void set_info_area(const wchar_t *pstr1, const wchar_t *pstr2 = nullptr) {
			// create bitmap if each button is enabled.
			_b_rspot_show = ((pstr1 == nullptr) ? 0 : 1) | ((pstr2 == nullptr) ? 0 : 2);
			
			// clear the buffer
			_btn_r1_label.clear();
			_btn_r2_label.clear();

			// set label string
			if(pstr1) _btn_r1_label << pstr1;
			if(pstr2) _btn_r2_label << pstr2;
		}

		inline bool_type is_info_selected() {
			bool_type ret = _b_rspot_clicked;
			_b_rspot_clicked = 0x00;
			return ret;
		}

		/* VIEW */
		inline void set_view(index_type view_start = 0, index_type view_selected = -1) {
			_n_view_start = view_start;
			if (view_start + view_selected >= int(_list.size())) {
				_n_view_selected = -1;
			}
			else {
				_n_view_selected = view_selected;
				if (_n_view_selected < -1) _n_view_selected = -1;
			}

			update_view(true);
		}

		/**
		 * prepare view with selecting item.
		 * 
		 * \param idx
		 */
		void set_view_with_select_item(index_type idx) {
			int page = idx / _n_view_rows;
			int idx_on_page = idx % _n_view_rows;

			set_view(page * _n_view_rows, idx_on_page);
		}

		/* SELECTION */
		void update_view(bool bFull, index_type sel_prev = -1, index_type sel_now = -1);
		index_type update_selection(index_type n_view_selection);
		index_type update_selection_absolute(index_type n_selection);
		bool key_event(TWE::keyinput_type keycode);
		void clear_completion() {
			_b_sel_comp_pre = false;
			_b_sel_comp = false;
		}

		// sort items
		void sort_items(bool b_nocase = false);
 	};
}