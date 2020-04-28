#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_utils_simplebuffer.hpp"
#include "twe_console.hpp"
#include "twe_cui_keyboard.hpp"

#include <utility>

namespace TWECUI {
	class TWE_ListView {
	public:
		typedef int8_t index_type;
		typedef uint8_t count_type;
		typedef uint8_t bool_type;
		typedef std::pair<TWEUTILS::SmplBuf_WChar&, TWEUTILS::SmplBuf_WChar&> pair_type;

	private:
		TWEUTILS::SimpleBuffer<TWEUTILS::SmplBuf_WChar> _list;
		TWEUTILS::SimpleBuffer<TWEUTILS::SmplBuf_WChar> _list_sub;
		const uint16_t _item_label_len;

		index_type _n_selected;

		bool_type _b_sel_comp_pre; // if selection is performed, set this flag. (display as selected)
		bool_type _b_sel_comp; // after small timer(100ms), this flag will be set.

		bool_type _b_enabled;

		TWETERM::ITerm* _pterm;
		index_type _n_view_start;
		index_type _n_view_selected;
		count_type _n_view_rows;
		count_type _n_view_cols;
		count_type _n_view_rows_disp;

		uint32_t _tick_selected; // millis when selection is performed.
		
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
			, _n_view_selected(0)
			, _n_view_rows(0)
			, _n_view_cols(0)
			, _n_view_rows_disp(0)
			, _b_enabled(true)
			, _tick_selected(0)
		{
			;
		}

		void attach_term(TWETERM::ITerm& trm);

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
		}

		inline index_type get_selected_index() {
			return _n_selected;
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