#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_utils_unicode.hpp"
#include "twe_stream.hpp"
#include "twe_printf.hpp"
#include "twe_utils_simplebuffer.hpp"

/* NOTE for UNICODE handling.

  the following functions should be update acording to supported chars.
  - ITerm::column_idx_to_vis()
  - ITerm::is_printable()
  - TWEUTILS::Unicode_isSingleWidth()

  also the drawing function (like drawChar_mplus12()) should be implemented correctly.
 */

namespace TWETERM {
	// define character type with attributes
	class GChar {
	public:
		typedef uint16_t tChar;
		typedef uint16_t tAttr;

	private:
		tChar _c;
		tAttr _attr;

	public:
		GChar() : _c(0), _attr(0) {}
		GChar(tChar c, tAttr attr = 0) : _c(c), _attr(attr) {}
		inline operator tChar& () { return _c; }
		inline tAttr& attr() { return _attr; }
		inline tChar& chr() { return _c; }
		GChar& operator = (tChar c) { _c = c; _attr = 0; return (*this);  }
		bool operator == (const GChar& c) { return (_c == c._c && _attr == c._attr); }
	};

	// define matrix
	typedef TWEUTILS::SimpleBuffer<GChar> SimpBuf_GChar;
	typedef TWEUTILS::SimpleBuffer<SimpBuf_GChar> SmplMtrx_GChar;

	/// <summary>
	/// ESC seq state, commands, attributes
	/// </summary>
	enum {
		E_ESCSEQ_VOID = 0,
		E_ESCSEQ_STATE_HAS_ESC,
		E_ESCSEQ_STATE_HAS_BRACKET,
		E_ESCSEQ_STATE_HAS_EQUAL,
		E_ESCSEQ_STATE_HAS_SEMICOLON,
		E_ESCSEQ_STATE_READNUM,

		E_ESCSEQ_UNKNOWN_COMMAND = 0x80,	// fails parsing.
		E_ESCSEQ_CURSOR_POSITION,			// ESC[?;?H or ESC[?;?f
		E_ESCSEQ_CURSOR_UP,					// ESC[?A
		E_ESCSEQ_CURSOR_DOWN,				// ESC[?B
		E_ESCSEQ_CURSOR_FWD,				// ESC[?C
		E_ESCSEQ_CURSOR_BWD,				// ESC[?D
		E_ESCSEQ_ERASE_DISPLAY,				// ESC[?J (normally 2 is use)
		E_ESCSEQ_ERASE_LINE,				// ESC[?K
		E_ESCSEQ_CURSOR_POSITION_COLUMN,	// ESC[?G
		E_ESCSEQ_CURSOR_ATTR,				// ESC[?m
		E_ESCSEQ_SCREEN_MODE,				// ESC[=?h

		E_ESCSEQ_BOLD_MASK = 0x100,
		E_ESCSEQ_REVERSE_MASK = 0x200,
		E_ESCSEQ_UNDERLINE_MASK = 0x400,
	};

	/// <summary>
	/// reference for ESC seq commands/
	/// </summary>
	extern const uint8_t U8CMDTBL[];

	/// <summary>
	/// parse esc sequence.
	/// </summary>
	class EscSeq {
		uint8_t _b[16];	// internal buffer to store sequence
		uint8_t _i;		// index
		uint8_t _s;		// state
		uint8_t _s_save; // state

		uint8_t _cmdchr;	// 0: invalid

		uint8_t _vals_n;  // stored values
		uint8_t _vals[8]; // vals array

		inline uint8_t _u8GetCmd(uint8_t c) {
			const uint8_t* p = U8CMDTBL;

			while (*p) {
				if (*p == c) {
					return *(p + 1);
				}
				p += 2;
			}

			return E_ESCSEQ_UNKNOWN_COMMAND;
		}

		inline bool start_value(uint8_t c) {
			if (_vals_n >= sizeof(_vals)) {
				_vals_n = sizeof(_vals); //error, overwrite end index...
			}
			else {
				_vals_n++;
				_vals[_vals_n - 1] = (c - '0');
			}

			_s_save = _s;
			_s = E_ESCSEQ_STATE_READNUM;

			return true;
		}

	public:
		EscSeq() : 	_b{ 0x0 }, _i(0), _s(0), _cmdchr(0) { }
		
		inline void s_init() {
			_s = 0;
		}

		bool operator << (uint8_t c);

		inline operator bool() { return is_complete();  }

		inline bool is_sequence() {
			return _s;
		}

		inline bool is_complete() {
			return _s & 0x80;
		}
	
		inline uint8_t get_command(uint8_t& c, uint8_t& nvals, uint8_t** pvals) {
			if (_s < 0x80) return 0;
			c = _cmdchr;
			nvals = _vals_n;
			*pvals = _vals;
			return _s;
		}
	};

	/// <summary>
	/// TERMINAL class manages text buffer of the screen.
	/// </summary>
	class ITerm : public TWE::IStreamOut {
		int32_t wrapchar;		// if cursor reaches right end, save the end char, otherwise set -1.
		EscSeq escseq;
		uint8_t wrap_mode;
		
	protected:
		int16_t cursor_l;		// cursor line position
		int16_t cursor_c;		// cursor column position
		int16_t end_l;			// virtual end line number (for efficient scrolling)

		SimpBuf_GChar* buf_astr_screen;
		SmplMtrx_GChar astr_screen;	// screen buffer array
		GChar* _raw_buffer;			// raw GChar buffer of screen text
		uint16_t _raw_buffer_max;  // max size of raw buffer, given by initial idx/lines.
		bool _b_dynamic_alloc;     // dynamic memory allocation

		uint8_t max_line;			// maximum line idx (0 .. (num of lines - 1))
		uint8_t max_col;			// maximum column idx (0 .. (num of cols - 1))

		uint8_t max_term_line;	// maximum line idx
		uint8_t max_term_col;	// maximum column idx

		uint32_t u32Dirty;		// a flag of redraw necessity.
		const uint32_t U32DIRTY_FULL = 0xFFFFFFFF;
		uint8_t u8OptRefresh;	// if 1, hardware clear should be applied.

		GChar::tAttr escseq_attr;	// attribute
		GChar::tAttr escseq_attr_default; // save default color when screen cleared.
		uint8_t screen_mode;	// screen mode (0: not specified)
		uint8_t cursor_mode;	// 0: off, 1: on and blink, 2: on and static

		uint8_t _bvisible;		// visible screen

		// UTF8
		uint8_t _utf8_stat;
		uint32_t _utf8_result;

	private:
		ITerm(const ITerm& obj) = delete;
		void operator =(const ITerm& obj) = delete;
		ITerm(const ITerm&& obj) = delete;

	public:
		ITerm(uint8_t u8c, uint8_t u8l, SimpBuf_GChar* pAryLines, GChar* pBuff) :
			max_line(u8l - 1),					// the end idx of the lines.
			max_term_line(u8l),					// the max term size
			max_col(u8c - 1),					// the end iex of the columns
			max_term_col(u8c),					// the max term size
			buf_astr_screen(nullptr),			
			astr_screen(pAryLines, u8l, u8l),	// ary of lines buffer.
			_raw_buffer(pBuff), 				// the raw uint8 buffer of screen mesg
			_b_dynamic_alloc(false),

			u8OptRefresh(0),
			escseq_attr(0), escseq_attr_default(0),

			u32Dirty(false), cursor_l(0), cursor_c(0), end_l(u8l - 1),
			escseq(), wrapchar(-1), screen_mode(0), cursor_mode(0),
			_utf8_stat(0), _utf8_result(0), wrap_mode(1), _bvisible(1)
		{
			// init screen buff
			// 
			_raw_buffer_max = u8c * u8l;
			_init_buff();
		}

		ITerm(uint8_t u8c, uint8_t u8l) :
			max_line(u8l - 1),					// the end idx of the lines.
			max_term_line(u8l),					// the max term size
			max_col(u8c - 1),					// the end iex of the columns
			max_term_col(u8c),					// the max term size

			u8OptRefresh(0),
			escseq_attr(0), escseq_attr_default(0),

			u32Dirty(false), cursor_l(0), cursor_c(0), end_l(u8l - 1),
			escseq(), wrapchar(-1), screen_mode(0), cursor_mode(0),
			_utf8_stat(0), _utf8_result(0), wrap_mode(1), _bvisible(1)
		{
			// alloc buffer dynamically
			buf_astr_screen = new SimpBuf_GChar[u8l];
			GChar* pBuff = new GChar[int(u8c * u8l)];

			_b_dynamic_alloc = true;

			astr_screen.attach(buf_astr_screen, u8l, u8c);
			_raw_buffer = pBuff;

			// init screen buff
			_raw_buffer_max = u8c * u8l;
			_init_buff();
		}

		virtual ~ITerm() {
			// memory release
			if (_b_dynamic_alloc) {
				if (_raw_buffer) delete[] _raw_buffer;
				if (buf_astr_screen) delete[] buf_astr_screen;
			}
		}

	protected:
		// init buff
		void _init_buff();
		
		// ESC seq graphical attributes
		inline GChar::tAttr get_escseq_attr() {
			return escseq_attr;
		}

		// redraw post process (it shall be called from at the end of refresh())
		inline void post_refresh() {
			u32Dirty = 0;
			u8OptRefresh = 0;
		}
		const uint8_t U8OPT_REFRESH_HARDWARE_CLEAR_MASK = 0x01;
		const uint8_t U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK = 0x02;
		const uint8_t U8OPT_REFRESH_WITH_SCREEN_MODE = 0x04;

		// new line
		inline void newline() {
			end_l = end_l + 1;

			if (end_l > max_line) {
				end_l = 0;
			}

			astr_screen[end_l].resize(0);

			cursor_c = 0;
			cursor_l = max_line;

			u32Dirty = U32DIRTY_FULL;
		}

		// calc the buffer index by screen line 'l'.
		inline uint8_t calc_line_index(int l) {
			int16_t i;

			i = end_l - max_line + l;
			if (i < 0) {
				i = i + max_line + 1;
			}

			return (uint8_t)i;
		}


		// rescale the array.
		void resize_screen(uint8_t u8c, uint8_t u8l);


		// check if it's not control chars. (TODO: to be considered more, like TAB handling)
		inline bool is_printable(wchar_t c) {
			return (c >= ' ' && c <= '~') || (c >= 0xa0);
		}


		// for wide char, get visual column position.
		uint16_t column_idx_to_vis(int16_t idx , int16_t lin) {
			unsigned cvis = 0;
			for (unsigned i = 0; i < unsigned(idx) && i <= max_col; i++) {
				if (i >= astr_screen[lin].length()) // blank area
					cvis++;
				else
					cvis += (TWEUTILS::Unicode_isSingleWidth(astr_screen[lin][i].chr()) ? 1 : 2);
			}
			return uint16_t(cvis);
		}

		// for wide char, get char pos from visual column
		uint16_t column_vis_to_idx(int16_t c_vis, int16_t lin) {
			int vis, idx;
			for (vis = 0, idx = 0; idx <= max_col; idx++) {
				int cwid = 1;
				if (   idx < int(astr_screen[lin].length())
					&& !TWEUTILS::Unicode_isSingleWidth(astr_screen[lin][idx].chr()))
				{
					cwid = 2;
				}

				// now idx=idx will have visual column vis or vis+1(when Double width char).
				if (   (vis + cwid > max_col)
					|| (c_vis >= vis && c_vis < vis + cwid))
				{
					break;
				}

				vis += cwid;
			}

			return idx;
		}

	public:
		// clear screen buffer
		inline void clear() {
			for (int i = 0; i <= max_line; i++) {
				astr_screen[i].resize(0); // clear buffer string
			}
			end_l = max_line;
			escseq_attr_default = escseq_attr; // set default when it's cleared.
			wrapchar = -1;

			u32Dirty = U32DIRTY_FULL;
		}

		// cursor set home pos
		inline void home() {
			u32Dirty |= (1UL << cursor_l);
			cursor_c = 0;
			cursor_l = 0;
			u32Dirty |= (1UL << cursor_l);
		}

		// clear screen
		inline void clear_screen() {
			clear();
			home();
		}

		// clear the line
		void clear_line(uint8_t line, bool fill_blank = false);
				
		// set dirty flag to redraw screen
		inline void force_refresh(uint8_t opt = 0) {
			u32Dirty = U32DIRTY_FULL; 
			u8OptRefresh = U8OPT_REFRESH_HARDWARE_CLEAR_MASK | U8OPT_REFRESH_WHOLE_LINE_REDRAW_MASK | opt;
			refresh(); // do refresh now!
		}

		inline void refresh_text() {
			u32Dirty = U32DIRTY_FULL;
		}

		// add a unicode to the terminal
		// ITerm& operator << (wchar_t c) { write(c); }
		ITerm& write(wchar_t c);

		// add a byte (utf-8)
		ITerm& write(char_t c) {
			int32_t wc = -1;
			// ASCII
			if ((c & 0x80) == 0x00) {
				_utf8_stat = 0;
				wc = c;
			}

			if (_utf8_stat == 0) {
				// 11bit
				if ((c & 0xE0) == 0xC0) {
					_utf8_result = ((c & 0x1F) << 6);
					_utf8_stat = 1;
				}

				// 16bit
				if ((c & 0xF0) == 0xE0)	{
					_utf8_result = ((c & 0x0F) << 12);
					_utf8_stat = 2;
				}

				// not support
				if ((c & 0xF8) == 0xF0) wc = c;
			} else {
				if (_utf8_stat == 2) {
					_utf8_result |= ((c & 0x3F) << 6);
					_utf8_stat--;
				} else {
					_utf8_result |= (c & 0x3F);
					_utf8_stat = 0;
					wc = _utf8_result;
				}
			}
			
			if (wc != -1) return write((wchar_t)wc);
			else return *this;
		}

		// output to others
		void operator >> (TWE::IStreamOut& fo); // dump as text into stream.

		// refresh screen (should be implemented by class)
		virtual void refresh() = 0;

		// information
		inline uint8_t get_height() { return max_line + 1; }
		inline uint8_t get_rows() { return max_line + 1; }
		inline uint8_t get_width() { return max_col + 1; }
		inline uint8_t get_cols() { return max_col + 1; }

		// copy screen buffer
		void get_screen_buf(GChar* ptr);
		// set screen buffer
		void set_screen_buf(GChar* ptr, uint8_t oldlines, uint8_t oldcols);

		// change cursor mode
		inline void set_cursor(uint8_t m) { cursor_mode = m; }

		// move cursor position
		ITerm& move_cursor(uint8_t cols, uint8_t lines);

		ITerm& operator ()(int x, int y) {
			move_cursor(x, y);
			return *this;
		}

		// wrap text
		inline void set_wraptext(bool b) { wrap_mode = b;  }

		bool visible() const { return _bvisible; }
		bool visible(bool bvis) { return _bvisible = bvis; }

		struct _coord_cols_lines {
			int16_t col;
			int16_t lin;
			uint8_t b_in_range;

			operator bool() { return b_in_range; }
		};

		// get terminal col/lines from screen coord.
		virtual _coord_cols_lines get_term_coord_from_screen(int16_t x, int16_t y) {
			return { x, y, true };
		}

	public:
		TWE::IStreamOut& operator << (TWE::IStreamSpecial& sc) { return sc(*this); }
		TWE::IStreamOut& operator ()(char_t c) { write(c); return *this; }
		TWE::IStreamOut& write_w(wchar_t c) { write(c); return *this; }
	};

	 const GChar::tAttr TERM_ATTR_OFF = 0x0;
	 const GChar::tAttr TERM_COLOR_FG_BLACK = 0x08;
	 const GChar::tAttr TERM_COLOR_FG_RED = 0x09;
	 const GChar::tAttr TERM_COLOR_FG_GREEN = 0x0A;
	 const GChar::tAttr TERM_COLOR_FG_YELLOW = 0x0B;
	 const GChar::tAttr TERM_COLOR_FG_BLUE = 0x0C;
	 const GChar::tAttr TERM_COLOR_FG_MAGENTA = 0x0D;
	 const GChar::tAttr TERM_COLOR_FG_CYAN = 0x0E;
	 const GChar::tAttr TERM_COLOR_FG_WHITE = 0x0F;
	 const GChar::tAttr TERM_COLOR_BG_BLACK = 0x80;
	 const GChar::tAttr TERM_COLOR_BG_RED = 0x90;
	 const GChar::tAttr TERM_COLOR_BG_GREEN = 0xA0;
	 const GChar::tAttr TERM_COLOR_BG_YELLOW = 0xB0;
	 const GChar::tAttr TERM_COLOR_BG_BLUE = 0xC0;
	 const GChar::tAttr TERM_COLOR_BG_MAGENTA = 0xD0;
	 const GChar::tAttr TERM_COLOR_BG_CYAN = 0xE0;
	 const GChar::tAttr TERM_COLOR_BG_WHITE = 0xF0;
	 const GChar::tAttr TERM_BOLD = E_ESCSEQ_BOLD_MASK;
	 const GChar::tAttr TERM_REVERSE = E_ESCSEQ_REVERSE_MASK;
	 const GChar::tAttr TERM_UNDERLINE = E_ESCSEQ_UNDERLINE_MASK;

	/// <summary>
	/// TWESERCMD::color attributes
	/// </summary>
	class TermAttr : public TWE::IStreamSpecial {
	private:
		GChar::tAttr _attr;

	public:
		TermAttr(GChar::tAttr attr = 0) : _attr(attr) {}

		inline TWE::IStreamOut& operator ()(TWE::IStreamOut& of) {
			// just output ESC SEQ
			of('\033');
			of('[');
			if (_attr == 0) {
				of('0');
			}
			else {
				int ct = 0;
				if (_attr & TERM_BOLD) { // FG
					if (ct++) of(';');
					of('1');
				}
				if (_attr & TERM_REVERSE) { // FG
					if (ct++) of(';');
					of('7');
				}
				if (_attr & TERM_UNDERLINE) { // FG
					if (ct++) of(';');
					of('4');
				}
				if (_attr & 0x000F) { // FG
					int c = _attr & 0x7;
					if (ct++) of(';');
					of('3');
					of('0' + c);
				}
				if (_attr & 0x00F0) {
					int c = (_attr & 0x70) >> 4;
					if (ct++) of(';');
					of('4');
					of('0' + c);
				}
			}
			of('m');

			return of;
		}
	};

	// operator for TermAttr
	inline ITerm& operator << (ITerm& t, TermAttr a) { a(t); return t; }
	inline TWE::IStreamOut& operator << (TWE::IStreamOut& t, TermAttr a) { a(t); return t; }

	// resolve ambiguous overload
	inline ITerm& operator << (ITerm& t, char_t c) { *static_cast<TWE::IStreamOut*>(&t) << c; return t; } // call via super class's operator.
	inline ITerm& operator << (ITerm& t, wchar_t c) { *static_cast<TWE::IStreamOut*>(&t) << c; return t; }
	inline ITerm& operator << (ITerm& t, const char *p) { *static_cast<TWE::IStreamOut*>(&t) << p; return t; }
	inline ITerm& operator << (ITerm& t, const wchar_t* p) { *static_cast<TWE::IStreamOut*>(&t) << p; return t; }
	inline ITerm& operator << (ITerm& t, TWEUTILS::SmplBuf_Byte& p) { *static_cast<TWE::IStreamOut*>(&t) << p; return t; }
	inline ITerm& operator << (ITerm& t, TWEUTILS::SmplBuf_WChar& p) { *static_cast<TWE::IStreamOut*>(&t) << p; return t; }
	inline ITerm& operator << (ITerm& t, const TWEUTILS::SmplBuf_Byte& p) { *static_cast<TWE::IStreamOut*>(&t) << p; return t; }
	inline ITerm& operator << (ITerm& t, const TWEUTILS::SmplBuf_WChar& p) { *static_cast<TWE::IStreamOut*>(&t) << p; return t; }

	// OTHERS
	inline ITerm& operator << (ITerm& t, int i) { *static_cast<TWE::IStreamOut*>(&t) << i; return t; }

	template <typename TP>
	inline ITerm& operator << (ITerm& t, std::pair<TP, TP> pair_pt) {
		TP p = pair_pt.first;
		TP e = pair_pt.second;

		while (p != e) {
			*static_cast<TWE::IStreamOut*>(&t) << *p;
			++p;
		}
		return t;
	}

	// system console instance
	extern ITerm& the_sys_console;

} // end of namespace TWEARD