/* Copyr	ight (C) 2022 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <complex>
#include <cmath>

#include "App_Graph.hpp"

using APP_BASE = App_Graph;

#define BUFFER_COUNT 4          // max supported nodes
#define HISTORY_SIZE (512*10)   // length of history
#define VIEW_SIZE 512			// view width
#define VIEW_SIZE_MIN (64)		// min view samples if magnified.
#define INCOMING_BUFFER_SIZE (HISTORY_SIZE/4) // size of primary data buffer, which is placed into main buffer by loop().

#define DEFAULT_FFT_SIZE 64     // default FFT size
#define MAX_FFT_SIZE 256        // MAX FFT size

#define PACKET_TIMEOUT_ms 3000   // if a new packet is not come with in the time, insert dummy data.
#define FFTHOLD_TIMEOUT_ms 10000 // MAX hold time for last FFT graph view.

#define PIXELS_FOR_DUMMY_DATA 4	 // pixel gap for dummy data

 // export capture data into log file
#define LOG_SNAP_NAME "acc_snap_"
#define LOG_NAME "accel_%08X_"
#define LOG_FILEEXT "csv"

namespace OTFFT {
	// OTFFT - http://wwwa.pikara.ne.jp/okojisan/stockham/optimization1.html
	//    Copyright(c) OK Ojisan(Takuya OKAHISA)
	//    
	using complex_t = std::complex<double>;

	/**
	 * Core procedure of FFT/IFFT, which is recursively called.
	 * 
	 * \param n		length of data
	 * \param s		stride
	 * \param eo	if eo == 0 or falsem, then x is output, else if eo == 1 or true then y is output.
	 * \param x		input data for FFT (output if eo == 0)
	 * \param y		working buffer (output if eo == 1)
	 */
	void fft0(int n, int s, bool eo, complex_t* x, complex_t* y)
	{
		const int m = n / 2;
		const double theta0 = 2 * 3.141592653589793 / n;

		if (n == 2) {
			complex_t* z = eo ? y : x;
			for (int q = 0; q < s; q++) {
				const complex_t a = x[q + 0];
				const complex_t b = x[q + s];
				z[q + 0] = a + b;
				z[q + s] = a - b;
			}
		}
		else if (n >= 4) {
			for (int p = 0; p < m; p++) {
				const complex_t wp = complex_t(cos(p * theta0), -sin(p * theta0));
				for (int q = 0; q < s; q++) {
					const complex_t a = x[q + s * (p + 0)];
					const complex_t b = x[q + s * (p + m)];
					y[q + s * (2 * p + 0)] = a + b;
					y[q + s * (2 * p + 1)] = (a - b) * wp;
				}
			}
			fft0(n / 2, 2 * s, !eo, y, x);
		}
	}

	/**
	 * perform FFT.
	 *
	 * \param N  FFT length (2^N)
	 * \param x  Input/Output buffers (complex_t array of 2^N)
	 */
	void fft(int N, complex_t* x)
	{
		complex_t* y = new complex_t[N];
		fft0(N, 1, 0, x, y);
		delete[] y;
		for (int k = 0; k < N; k++) x[k] /= N;
	}

	/**
	 * perform FFT with prealocatted working buffer.
	 * 
	 * \param N  FFT length (2^N)
	 * \param x  Input/Output buffers (complex_t array of 2^N)
	 * \param y  Working buffers (complex_t array of 2^N)
	 */
	void fft(int N, complex_t* x, complex_t* y) // FFT
	{
		fft0(N, 1, 0, x, y);
		for (int k = 0; k < N; k++) x[k] /= N;
	}

	/**
	 * perform I-FFT.
	 * 
	 * \param N  FFT length (2^N)
	 * \param x  Input/Output buffers (complex_t array of 2^N)
	 */
	void ifft(int N, complex_t* x) // I-FFT
	{
		for (int p = 0; p < N; p++) x[p] = conj(x[p]);
		complex_t* y = new complex_t[N];
		fft0(N, 1, 0, x, y);
		delete[] y;
		for (int k = 0; k < N; k++) x[k] = conj(x[k]);
	}
}

/**
 * Managing horizontal scrollbar.
 */
class TWE_GUI_ScrollBarH {
private:
	Rect _rg_all;				// whole area in the screen
	Rect _rg_bar;				// bar area (excluding frame)
	Rect _rg_btn;				// bar button area
	int32_t _var_min;			// minimum numeric value
	int32_t _var_max;			// maximum numeric value
	int32_t _var_view_start;	// numeric value of screen left.
	int32_t _var_view_last;		// numeric value of screen right.

public:
	TWE_GUI_ScrollBarH()
		: _rg_all(), _rg_bar(), _rg_btn()
		, _var_min(0), _var_max(100)
		, _var_view_start(40), _var_view_last(60)
	{
	}

	/**
	 * calculate bar area from give area.
	 * 
	 * \param rg	whole area of scrollbar.
	 */
	void set_draw_area(Rect rg) {
		_rg_all = rg;

		// bar area : 1px inside of thee whole area.
		_rg_bar.h = _rg_all.h - 2;
		_rg_bar.w = _rg_all.w - 2;
		_rg_bar.x = _rg_all.x + 1;
		_rg_bar.y = _rg_all.y + 1;
		_rg_btn = _rg_bar;
	}

	/**
	 * calculate bar button position from numeric values.
	 * 
	 * \param start		numeric value of screen left.
	 * \param last		numeric value of screen right.
	 * \param v_min		minimum numeric value.
	 * \param v_max		maximum numeric value.
	 */
	void set_value(int32_t start, int32_t last, int32_t v_min, int32_t v_max) {
		if (!(v_min < v_max)) return;

		_var_view_start = start;
		_var_view_last = last;
		_var_min = v_min;
		_var_max = v_max;

		int32_t w = (last - start) * _rg_bar.w / (v_max - v_min);
		int32_t s = (start - v_min) * _rg_bar.w / (v_max - v_min);
		int32_t l = (last - v_min) * _rg_bar.w / (v_max - v_min);

		_rg_btn.w = l - s;
		_rg_btn.h = _rg_bar.h;
		_rg_btn.x = _rg_bar.x + s;
		_rg_btn.y = _rg_bar.y;
	}

#if 0 // SO FAR, NOT IMPLEMENTED (USE SIMPLER WAY FOR SCROLLBAR DRAGGING)
	// if scrollbar is dragged, report the new region...
	std::tuple<int32_t, int32_t> set_drag(int32_t delta) {
		int32_t x_new = _rg_btn.x + delta;
		if (x_new < _rg_bar.x) delta = x_new = _rg_bar.x;
		if (x_new + _rg_btn.w > _rg_bar.x + _rg_bar.w) x_new = _rg_bar.x + _rg_bar.w - _rg_btn.w;

		// update var value
		_var_view_start = (_var_max - _var_min) * (x_new - _rg_bar.x) / _rg_bar.w;
		if (_var_view_start < _var_min) _var_view_start = _var_min;
		_var_view_last = (_var_max - _var_min) * (x_new + _rg_btn.w - 1) / _rg_bar.w;
		if (_var_view_last > _var_max) _var_view_last = _var_max;

		return { _var_view_start, _var_view_last };
	}
#endif

	/**
	 * rendering the scrollbar.
	 * - set_value() should be perfomed before call.
	 */
	void update_view() {
		if (_rg_btn.w < _rg_bar.w) {
			M5.Lcd.drawRect(_rg_all.x, _rg_all.y, _rg_all.w, _rg_all.h, color565(120, 120, 120)); // frame
			M5.Lcd.fillRect(_rg_bar.x, _rg_bar.y, _rg_bar.w, _rg_bar.h, color565(80, 80, 80));	  // bar bg
			M5.Lcd.fillRect(_rg_btn.x, _rg_btn.y, _rg_btn.w, _rg_btn.h, color565(160, 160, 160)); // bar button
		}
		else {
			// inactivated (value range and view range is the same)
			M5.Lcd.drawRect(_rg_all.x, _rg_all.y, _rg_all.w, _rg_all.h, color565(120, 120, 120)); // frame
			M5.Lcd.fillRect(_rg_bar.x, _rg_bar.y, _rg_bar.w, _rg_bar.h, color565(80, 80, 80));	  // bar bg
		}
	}
};


/**
 * basic data structure of an ACCEL sample.
 */
struct XYZT {
	int32_t x; // X AXIS 1000=1G
	int32_t y; // Y
	int32_t z; // Z
	uint32_t t; // Tick[ms] when the sample is acquired (estimate)
	uint32_t t_pkt; // Tick[ms] when the sample packet is received via RF.
	uint16_t seq; // Sequence number of the packet.
};


/**
 * Store/output information when log file is exported.
 * - the information is pair of string (label and data)
 * - csv_out() call will output ",{label},{data}".
 */
struct _INFO {
	static const int INFO_MAX_DATA = 16;
	using string_type = SmplBuf_ByteSL<32>;

	/**
	 * append an entry with label(lbl) and returns reference to the data.
	 * 
	 * \param lbl  information label string
	 * \return	   reference to the data.
	 */
	string_type& append(const char* lbl) {
		if (n_count >= INFO_MAX_DATA) return _dummy;
		info_lbl[n_count] << lbl;
		n_count++;
		return info_dat[n_count-1];
	}

	/**
	 * append an entry with label(lbl) and data string(dat).
	 *
	 * \param lbl  information label string
	 * \return	   reference to the data.
	 */
	string_type& append(const char* lbl, const char* dat) {
		if (n_count >= INFO_MAX_DATA) return _dummy;
		info_lbl[n_count] << lbl;
		info_dat[n_count] << dat;
		n_count++;
		return info_dat[n_count -1];
	}

	/**
	 * append an entry with label(lbl) 
	 * and integer data(dat) which will converted into string,
	 *
	 * \param lbl  information label string
	 * \return	   reference to the data.
	 */
	string_type& append(const char* lbl, int dat) {
		if (n_count >= INFO_MAX_DATA) return _dummy;
		info_lbl[n_count] << lbl;
		info_dat[n_count] << printfmt("%d", dat);
		n_count++;
		return info_dat[n_count - 1];
	}

	/**
	 * output an entry indexed by n_idx to the output stream.
	 * 
	 * \param os  the output stream.
	 */
	void csv_out(std::ostream& os) {
		if (n_idx < n_count) {
			os << "," << info_lbl[n_idx].c_str();
			os << "," << info_dat[n_idx].c_str();
		}
		else {
			os << ",,";
		}
		n_idx++;
	}

	/**
	 * rewind the index n_idx.
	 */
	void rewind() { n_idx = 0; }

	/**
	 * get the count of information entries.
	 * 
	 * \return the count of entries.
	 */
	int32_t get_count() { return n_count; }

	_INFO() : n_count(0), n_idx(0), info_lbl(), info_dat() {}

private:
	int n_count;	// count of stored entries
	int n_idx;		// an index of entries when output to the stream.
	SmplBuf_ByteSL<32> info_lbl[INFO_MAX_DATA]; // information label
	SmplBuf_ByteSL<32> info_dat[INFO_MAX_DATA]; // information data
	SmplBuf_ByteSL<32> _dummy; // dummy string is returned when info_dat[] is full.
};

/**
 * The Screen definition of App CUE Graph.
 */
struct APP_BASE::SCR_CUE_FIFO : public APP_HANDLR_DC {
	static const int CLS_ID = APP_BASE::PAGE_ID::PAGE_CUE_FIFO; // used by APP_HANDLER_DC
	int get_class_id() { return CLS_ID; } // used by APP_HANDLER_DC

	APP_BASE& _app;					// reference to parent class App_CUE (friend relationship)
	TWE_WidSet_Buttons _btns;		// button array.
	int _pkt_rcv_ct;				// total count of incoming packets.

	// object references to the App_CUE
	TWETerm_M5_Console& the_screen; // main screen (moving to the right side, which this screen is present)
	ITerm& the_screen_b;			// botton screen for debugging information.
	IParser& parse_ascii;			// serial message parser (parse :001122... encoded message)

	/**
	 * BASIC DATA STRUCTURE for a single TWELITE node.
	 */
	struct _DATA {
		uint32_t _id;											// the module SID
		uint32_t _ct_total;										// count of incoming packets from SID.
		std::unique_ptr<TWEUTILS::FixedQueue<XYZT>> _v;			// history buffer of accel data.
		std::unique_ptr<TWEUTILS::FixedQueue<XYZT>> _w;			// incoming buffer (store accel data firstly)
		std::unique_ptr<TWEUTILS::FixedQueue<uint32_t>> _t;		// packet timestamp
		std::unique_ptr<TWE::TweLogFile> _log;					// logging file (csv style, just dumping out)

		_DATA() : _id(0), _ct_total(0), _v(), _w(), _t(), _log() {}
		~_DATA() {
			if(_log) _log->close();
		}

		/**
		 * allocate memory of each vectors.
		 * 
		 * \param id		the module SID associated with this data structure.
		 */
		void alloc_data(uint32_t id) {
			_id = id;
			_v.reset(new TWEUTILS::FixedQueue<XYZT>(HISTORY_SIZE));
			_w.reset(new TWEUTILS::FixedQueue<XYZT>(INCOMING_BUFFER_SIZE));
			_t.reset(new TWEUTILS::FixedQueue<uint32_t>(10));
		}

		/**
		 * open csv dumping-log file.
		 * 
		 * \return 
		 */
		bool open_log() {
			SmplBuf_ByteSL<256> fname;
			fname << format(LOG_NAME, _id);
			_log.reset(new TweLogFile(fname.c_str(), LOG_FILEEXT));

			return _log->open();
		}

	} _d[BUFFER_COUNT]; // reserve fixed instance of _DATA structure.

	uint32_t _i_view_current; // index of _d[] which is currently shown. (this index should be carefully handled, to assure data is allocated)

	bool b_view_has_data(int i) { return ID(i) != 0; } // check if the current view has data.

	uint32_t& ID(int i) { return _d[i]._id; } // returns SID reference indexed by i, _d[i] is empty when ID(i) == 0.
	uint32_t& CT(int i) { return _d[i]._ct_total; } // returns packet count reference. 
	TWEUTILS::FixedQueue<XYZT>& V(int i) { return *_d[i]._v; } // returns history data reference (must check with b_view_has_data() in advance)
	TWEUTILS::FixedQueue<XYZT>& W(int i) { return *_d[i]._w; } // returns incoming data reference (must check with b_view_has_data() in advance)
	TWEUTILS::FixedQueue<uint32_t>& T(int i) { return *_d[i]._t; } // returns packet timestamp reference (must check with b_view_has_data() in advance)
	_DATA& D(int i) { return _d[i]; } // returns reference to _d[i] of _DATA structure.
	TweLogFile& LOG(int i) {return *_d[i]._log;} // returns reference to log file (must check with b_view_has_data() in advance)

	// Drawing area
	Rect _area_draw;		// the area of graph screen
	Rect _area_scrollbar;	// the area of scrollbar area (with in the graph screen)
	Rect _area_scr_main;	// saves the area of main screen(the_screen), the main screen is moved to right side whie this screen is present.

	TWE_GUI_ScrollBarH _scrollbar; // scroll bar management.

	/**
	 * The _VIEW struct manages the displayed samples on the screen.
	 * 
	 * oldest[first index]                  newest[last index]
	 * |                                    |
	 * |     leftmost           rightmost   |
	 * v     +--------------------------+   v
	 * ......|..........................|.... sample vector
	 *       |<--item_count------------>|
	 *       +--------------------------+<-->shift
	 *        ^        SCREEN          ^    
	 *        start_index(older)       last_index(newer)
	 *    
	 */
	struct _VIEW {
		int32_t start_index; // sample index of the left most (older) of the screen.
		int32_t last_index;  // sample index of the right most (newer) of the screen.
		int32_t item_count;  // number of samples displayed on the screen.

		int32_t shift;       // shift count of samples from the newest one.
		int32_t magnify;     // scale factor of enlarging horizontally. (1..4)

		bool b_dragging;     // when dragging by mouse left button.
		bool b_dragfast;     // when dragging the scroll bar
		int32_t x_drag, y_drag, x_last, y_last, shift_drag; // for drag control.

		/**
		 * set the view of graph area.
		 * 
		 * \param d				the data structure desired to show.
		 * \param shift_		samples shifted older. (if set 0 as default, sampels are shown from the latest at right most to older)
		 * \param magnify_		scale factor of time axis. (e.g. if set 2, time axis is stretched by 2x)
		 * \return				always true
		 */
		bool set_view(_DATA& d, int32_t shift_ = -1, int32_t magnify_ = -1) {
			// update paramater if it's not set -1.
			if (shift_ != -1) shift = shift_;
			if (magnify_ != -1) magnify = magnify_;

			int32_t max_samples_if_magnified = VIEW_SIZE / magnify;

			// last index is last item of v[], which is the latest sample
			if (d._v->size() < VIEW_SIZE_MIN) {
				last_index = d._v->size() - 1;
				shift = 0;
			}
			else {
				last_index = d._v->size() - 1 - shift;
				if (last_index + 1 < VIEW_SIZE_MIN) {
					last_index = VIEW_SIZE_MIN - 1;
					shift = d._v->size() - 1 - last_index;
				}
			}

			// identfy the start index by VIEW_SIZE.
			start_index = last_index - VIEW_SIZE + 1;
			if (start_index < 0) start_index = 0;
			item_count = last_index - start_index + 1;

			// if magnified, recalc it.
			if (magnify > 1) {
				if (item_count > max_samples_if_magnified) {
					start_index = last_index - max_samples_if_magnified + 1;
					item_count = max_samples_if_magnified;
				}
			}

			return true;
		}

		/**
		 * reset screen view (call when SID is changed).
		 */
		void reset() {
			shift = 0;
			magnify = 1;
		}

		/**
		 * Check if the coordinate(x,y) is in an area(area).
		 * - for checking mouse pointe is in an area.
		 * 
		 * \param area	the area
		 * \param x		coord x
		 * \param y		coord y
		 * \return		true: in the area, false: out of the area
		 */
		static bool is_in_area(Rect& area, int32_t x, int32_t y) {
			if (x >= area.x && x < area.x + area.w
				&& y >= area.y && y < area.y + area.h) return true;
			else return false;
		}

		_VIEW()
			: start_index(0)
			, last_index(0)
			, item_count(0)
			, shift(0)
			, magnify(1)
			, b_dragging(false), b_dragfast(false)
			, x_drag(0), y_drag(0), x_last(0), y_last(0), shift_drag(0)
		{}
	} _view;

	/**
	 * FFT information, mainly stores parameter and calculated data.
	 */
	struct _FFT {
		int32_t bar_width;		// witdh of vartical  freq-axis bar (1:N=256, 2:N=128, 4:N=64)
		int32_t n_fft;			// FFT samples
		int32_t cHz;			// centi hertz of given sample (guess frequency from timestamps)

		// FFT vect
		OTFFT::complex_t vfft[MAX_FFT_SIZE]; // input/output buffer
		OTFFT::complex_t wfft[MAX_FFT_SIZE]; // work buffer
		double vfft_abs[3][MAX_FFT_SIZE];    // FFT result (magnitude)
		TWEUTILS::FixedQueue<XYZT> v;        // Keep waveform for FFT

		bool b_fft_calculated;	// if true, this data has an effective data.
		bool b_fft_hold;		// if true, hold mode is active (having effective data of last FFT calculation)
		uint32_t tick_fft_hold;	// elapsed milliseconds after the last FFT is performed (in hold mode)

		/**
		 * SET FFT SIZE AND RELATED PARAMETERS.
		 * 
		 * \param n		0: set to the next, 64/128/256: set the size
		 */
		void set_fft_size(unsigned n) {
			if (n == 0) {
				n = n_fft *= 2;
				if (n > 256) n = 64;
			}

			switch (n) {
			case 64:
				bar_width = 4;
				n_fft = 64;
				break;
			case 128:
				bar_width = 2;
				n_fft = 128;
				break;
			case 256:
			default:
				bar_width = 1;
				n_fft = 256;
				break;
			}
		}

		_FFT() :
			bar_width(4),
			n_fft(64),
			cHz(0),
			vfft(),
			vfft_abs(),
			v(MAX_FFT_SIZE),
			b_fft_calculated(false), b_fft_hold(false),
			tick_fft_hold(0)
		{}
	} _fft;

	/*
	 * User Interface
	 */ 
	bool _b_pause; // if true, PAUSED
	
	/**
	 * save the button IDs.
	 */
	struct {
		int fft;
		int nodes;
		int csvout;
		int pause;
	} _id_btns;

	// CONSTRUCTOR
	SCR_CUE_FIFO(APP_BASE& app) : APP_HANDLR_DC(CLS_ID)
		, _app(app), _btns(*this, app.the_screen), _pkt_rcv_ct(0)
		, the_screen(app.the_screen), the_screen_b(app.the_screen_b), parse_ascii(app.parse_ascii)
		, _d(), _i_view_current(0)
		, _fft()
		, _b_pause(false), _id_btns()
		//, _file_buf(), _file_os(), _file_fullpath()
		, _scrollbar()
	{
		_area_scr_main = app.the_screen.get_draw_area(); // save the area of the_screen.

		// define the area of the graph
		_area_draw = _area_scr_main;
		_area_draw.w -= 128; // make a space of 128pix on the right for the main screen.

		_fft.set_fft_size(DEFAULT_FFT_SIZE);

		// move the main screen and clear
		the_screen.set_draw_area({ 640 - 128, _area_scr_main.y, 128, _area_scr_main.h });
		the_screen.set_font(_app.font_IDs.smaller); // use font ID (small font)
		the_screen.clear_screen();
		the_screen.force_refresh();

		// define the area of the scrollbar.
		_area_scrollbar = _area_draw;
		_area_scrollbar.x += 128;
		_area_scrollbar.y = _area_draw.y + _area_draw.h - 1 - 8;
		_area_scrollbar.w -= 128 * 2;
		_area_scrollbar.h = 8;
		_scrollbar.set_draw_area(_area_scrollbar);
	}

	~SCR_CUE_FIFO() {}

	/**
	 * SCALE milli-G to pixel count in the screen.
	 * 
	 * \param v			numeric data of milli-G.
	 * \param clip		returning pixel count is clipped by this value.
	 * \return			the pixel count
	 */
	int32_t conv_mg_to_pixel(int32_t v, uint32_t clip) {
		int32_t pix = v * 64 / 1000; // 64PIX/G
		if (std::abs(pix) > int(clip)) pix = clip;

		return pix;
	}

	/**
	 * RENDER SCREEN.
	 * 
	 */
	void update_screen() {
		if (!b_view_has_data(_i_view_current)) return; // no data (do nothing)

		const auto& font = TWEFONT::queryFont(_app.font_IDs.smaller);
		auto font_w = font.get_width();
		auto font_h = font.get_height();

		constexpr auto col_gray30 = color565(255 * 30 / 100, 255 * 30 / 100, 255 * 30 / 100);
		constexpr auto col_gray80 = color565(255 * 80 / 100, 255 * 80 / 100, 255 * 80 / 100);
		constexpr auto col_LightR = color565(255, 128, 128);
		constexpr auto col_darkG = color565(0, 128, 0);
		constexpr auto col_LightG = color565(80, 160, 80);
		constexpr auto col_LightB = color565(128, 128, 255);
		constexpr auto col_void = color565(255, 224, 224); // for dummy sample

		int32_t plt_x = _area_draw.x + _area_draw.w - 1;
		int32_t plt_y = _area_draw.y + _area_draw.h / 2;


		// FFT
		double fft_max_x = 0;
		uint16_t n_fft_max_x = 0;
		double fft_max_y = 0;
		uint16_t n_fft_max_y = 0;
		double fft_max_z = 0;
		uint16_t n_fft_max_z = 0;

		int32_t t_period = 0;

		auto& v = V(_i_view_current);
		auto& id = ID(_i_view_current);

		// calculate wave view information.
		_view.set_view(D(_i_view_current));
		unsigned idx_data_end = _view.last_index + 1;
		unsigned idx_data_begin = _view.start_index;

		// check if FFT can be performed.
		_fft.b_fft_hold = false;
		_fft.b_fft_calculated = false;
		if (_view.item_count >= _fft.n_fft) {
			_fft.b_fft_calculated = true;
			for (int32_t i = 0; i < _fft.n_fft; ++i) {
				if (v[_view.last_index - i].t == 0) {
					_fft.b_fft_calculated = false;
					break;
				}
			}
			if (_fft.b_fft_calculated) {
				int32_t j = 0;
				for (int32_t i = 0; i < _fft.n_fft; ++i) {
					_fft.v[i] = v[_view.last_index - _fft.n_fft + 1 + i]; // save waveform
				}
				_fft.tick_fft_hold = millis();
			}
		}
		if (_fft.b_fft_calculated == false) {
			if (_fft.tick_fft_hold != 0 && (millis() - _fft.tick_fft_hold < FFTHOLD_TIMEOUT_ms)) {
				_fft.b_fft_hold = true;
			}
		}

		// clear screen
		if (_fft.b_fft_calculated || _fft.b_fft_hold) {
			M5.Lcd.fillRect(_area_draw.x, _area_draw.y, _area_draw.w - _fft.n_fft, _area_draw.h, ALMOST_WHITE);
			M5.Lcd.fillRect(_area_draw.x + _area_draw.w - _fft.n_fft * _view.magnify, _area_draw.y, _fft.n_fft * _view.magnify, _area_draw.h, _fft.b_fft_hold ? color565(0xe0e0e0) : color565(0xFFFFE0));
		}
		else {
			M5.Lcd.fillRect(_area_draw.x, _area_draw.y, _area_draw.w, _area_draw.h, ALMOST_WHITE);
		}

		// render dummy data (fill col_void vertically)
		for (int i = 0; i < _view.item_count; i++) {
			auto&& val = v[_view.last_index - i];
			const int32_t MAX_V = _area_draw.h / 2;

			if (_fft.b_fft_hold && i < _fft.n_fft) {
				; // do not render dummy pix on hold display of previous FFT.
			}
			else if (val.t == 0) {
				M5.Lcd.drawLine(plt_x - i * _view.magnify, _area_draw.y, plt_x - i * _view.magnify, _area_draw.y + _area_draw.h - 1, col_void);
			}
		}

		// grid
		M5.Lcd.drawLine(_area_draw.x, plt_y, _area_draw.x + _area_draw.w - 1, plt_y, col_gray30);
		drawChar(font, _area_draw.x + 4, plt_y, "0G", col_gray80, WHITE, 0, M5);

		M5.Lcd.drawLine(_area_draw.x, plt_y - 64, _area_draw.x + _area_draw.w - 1, plt_y - 64, col_gray80);
		drawChar(font, _area_draw.x + 4, plt_y - 64, "+1G", col_gray80, WHITE, 0, M5);

		M5.Lcd.drawLine(_area_draw.x, plt_y + 64, _area_draw.x + _area_draw.w - 1, plt_y + 64, col_gray80);
		drawChar(font, _area_draw.x + 4, plt_y + 64, "-1G", col_gray80, WHITE, 0, M5);

		M5.Lcd.drawLine(_area_draw.x, plt_y - 128, _area_draw.x + _area_draw.w - 1, plt_y - 128, col_gray80);
		drawChar(font, _area_draw.x + 4, plt_y - 128, "+2G", col_gray80, WHITE, 0, M5);

		M5.Lcd.drawLine(_area_draw.x, plt_y + 128, _area_draw.x + _area_draw.w - 1, plt_y + 128, col_gray80);
		drawChar(font, _area_draw.x + 4, plt_y + 128, "-2G", col_gray80, WHITE, 0, M5);


		// FFT and render FFT result.
		if (_fft.b_fft_calculated || _fft.b_fft_hold) {
			t_period = _fft.v[_fft.n_fft - 1].t - _fft.v[0].t;
			if (t_period < 100000) t_period = t_period * 1024 / (_fft.n_fft - 1);
			else t_period = 0;

			int32_t xs = 40;
			int32_t ys = _area_draw.y + _area_draw.h - 1 - 24;
			const int32_t MAX_V = _area_draw.h * 7 / 8;

			// draw label FFT REGION
			const char* msg_FFT_REGION = "FFT REGION";
			const char* msg_FFT_HOLD = "FFT HOLD";
			drawChar(font
				, _area_draw.x + _area_draw.w - 1 - font_w * int32_t(strlen(_fft.b_fft_hold ? msg_FFT_HOLD : msg_FFT_REGION))
				, _area_draw.y // + _area_draw.h - 1 - font_h
				, _fft.b_fft_hold ? msg_FFT_HOLD : msg_FFT_REGION, col_gray80, WHITE, 0, M5);

			// X
			for (int i = 0; i < _fft.n_fft; i++) {
				_fft.vfft[i] = { _fft.v[i].x / 1000.0 , 0. };
				_fft.wfft[i] = {};
			}
			OTFFT::fft(_fft.n_fft, _fft.vfft, _fft.wfft);

			drawChar(font, xs, ys + 1, 'X', RED, WHITE, 0, M5);
			for (int i = 0; i < _fft.n_fft / 2; i++) {
				double v_abs = std::abs(_fft.vfft[i]);
				_fft.vfft_abs[0][i] = v_abs;

				if (i && v_abs > fft_max_x) { fft_max_x = v_abs; n_fft_max_x = i; }
				int32_t px = int32_t(v_abs * 256 + 0.5);
				if (px > MAX_V) px = MAX_V;

				M5.Lcd.fillRect(i * _fft.bar_width + xs, ys, _fft.bar_width, -px, i & 1 ? RED : col_LightR);
			}
			M5.Lcd.drawLine(xs, ys + 1, xs + 127, ys + 1, BLACK /* RED */);

			// Y
			xs += 140;
			for (int i = 0; i < _fft.n_fft; i++) {
				_fft.vfft[i] = { _fft.v[i].y / 1000.0 , 0. };
				_fft.wfft[i] = {};
			}
			OTFFT::fft(_fft.n_fft, _fft.vfft, _fft.wfft);

			drawChar(font, xs, ys + 1, 'Y', col_darkG, WHITE, 0, M5);
			for (int i = 0; i < _fft.n_fft / 2; i++) {
				double v_abs = std::abs(_fft.vfft[i]);
				_fft.vfft_abs[1][i] = v_abs;

				if (i && v_abs > fft_max_y) { fft_max_y = v_abs; n_fft_max_y = i; }
				int32_t px = int32_t(v_abs * 256 + 0.5);
				if (px > MAX_V) px = MAX_V;

				M5.Lcd.fillRect(xs + i * _fft.bar_width, ys, _fft.bar_width, -px, i & 1 ? col_darkG : col_LightG);
			}
			M5.Lcd.drawLine(xs, ys + 1, xs + 127, ys + 1, BLACK /* col_darkG */);

			// Z
			xs += 140;
			for (int i = 0; i < _fft.n_fft; i++) {
				_fft.vfft[i] = { _fft.v[i].z / 1000.0 , 0. };
				_fft.wfft[i] = {};
			}
			OTFFT::fft(_fft.n_fft, _fft.vfft, _fft.wfft);

			drawChar(font, xs, ys + 1, 'Z', BLUE, WHITE, 0, M5);
			for (int i = 0; i < _fft.n_fft / 2; i++) {
				double v_abs = std::abs(_fft.vfft[i]);
				_fft.vfft_abs[2][i] = v_abs;

				if (i && v_abs > fft_max_z) { fft_max_z = v_abs; n_fft_max_z = i; }
				int32_t px = int32_t(v_abs * 256 + 0.5);
				if (px > MAX_V) px = MAX_V;

				M5.Lcd.fillRect(xs + i * _fft.bar_width, ys, _fft.bar_width, -px, i & 1 ? BLUE : col_LightB);
			}
			M5.Lcd.drawLine(xs, ys + 1, xs + 127, ys + 1, BLACK /* BLUE */);
		}

		// draw graph
		{
			int32_t last_x = 0, last_y = 0, last_z = 0; // last data (to draw line)
			bool b_last_data = false; // if last data is proper (not skipping dummy)
			uint32_t t_now = millis();
			for (int i = 0; i < _view.item_count; i++) {
				bool b_disp_hold_sample = false;
				XYZT val;

				if (_fft.b_fft_hold && i < _fft.n_fft) {
					b_disp_hold_sample = true;
					val = _fft.v[_fft.n_fft - i - 1];
				}
				else {
					val = v[_view.last_index - i];
				}

				int32_t v_px;
				const int32_t MAX_V = _area_draw.h / 2 - font_h;

				if (b_disp_hold_sample || val.t != 0) {
					uint16_t colR, colG, colB;
					if (!_fft.b_fft_calculated || i < _fft.n_fft) {
						colR = RED;
						colG = col_darkG;
						colB = BLUE;
					}
					else {
						colR = col_LightR;
						colG = col_LightG;
						colB = col_LightB;
					}

					int32_t x_now = plt_x - i * _view.magnify;
					int32_t x_last = plt_x - (i - 1) * _view.magnify;

					v_px = -conv_mg_to_pixel(val.x, MAX_V);
					M5.Lcd.drawPixel(x_now, plt_y + v_px, colR);
					if (i > 0 && b_last_data) M5.Lcd.drawLine(x_now, plt_y + v_px, x_last, plt_y + last_x, colR);
					last_x = v_px;

					v_px = -conv_mg_to_pixel(val.y, MAX_V);
					M5.Lcd.drawPixel(x_now, plt_y + v_px, colG);
					if (i > 0 && b_last_data) M5.Lcd.drawLine(x_now, plt_y + v_px, x_last, plt_y + last_y, colG);
					last_y = v_px;

					v_px = -conv_mg_to_pixel(val.z, MAX_V);
					M5.Lcd.drawPixel(x_now, plt_y + v_px, colB);
					if (i > 0 && b_last_data) M5.Lcd.drawLine(x_now, plt_y + v_px, x_last, plt_y + last_z, colB);
					last_z = v_px;

					b_last_data = true;
				}
				else {
					b_last_data = false;
				}
			}
		}

		// other info
		if (_view.item_count > 0) {
			SmplBuf_ByteSL<128> msg;
			msg.clear();
			msg << format("[%04d/", v.size() - _view.start_index);
			if (v[_view.start_index].t) msg << format("%dms]", v[_view.start_index].t);
			else msg << "---]";

			drawChar(font
				, _area_draw.x + int32_t(font_w / 2)
				, _area_draw.y + _area_draw.h - 1 - font_h
				, msg.c_str(), col_gray30, WHITE, 0x00, M5);

			msg.clear();

			if (_fft.b_fft_hold) {
				msg << format("[HOLD/%dms]", _fft.v[_fft.n_fft - 1].t);
			}
			else {
				msg << format("[%04d/%dms]", v.size() - _view.last_index, v[_view.last_index].t);
			}

			drawChar(font
				, _area_draw.x + _area_draw.w - int32_t(font_w * strlen(msg.c_str()) + font_w / 2)
				, _area_draw.y + _area_draw.h - 1 - font_h
				, msg.c_str(), col_gray30, WHITE, 0x00, M5);


			// scroll bar
			_scrollbar.set_value(_view.start_index, _view.last_index, 0, D(_i_view_current)._v->size() - 1);
			_scrollbar.update_view();
		}

		// update TEXT in the main screen.
		the_screen(0, 1);
		UpdateLbl_Btn_Node(_i_view_current);

		// print extra info (start row)
		int row = 6;

		// print FFT information
		if (_fft.b_fft_calculated || _fft.b_fft_hold) {
			the_screen(0, row) << "[FFT INFO](推定値)" << crlf; // move cursor
			_fft.cHz = t_period ? 100 * 1024 * 1000 / t_period : 0;

			the_screen << printfmt("\033[K 周波数: %3d.%02dHz", _fft.cHz / 100, _fft.cHz % 100) << crlf;
			the_screen << "\033[K ピーク:" << crlf;

			uint32_t cHz = _fft.cHz * n_fft_max_x / _fft.n_fft;
			if (fft_max_x > 0.02) the_screen << printfmt("  \033[31;47mＸ\033[0m %3d.%02dHz %1.2f", cHz / 100, cHz % 100, fft_max_x) << crlf;
			else the_screen << "\033[K  \033[31;47mＸ\033[0m       Hz" << crlf;

			cHz = _fft.cHz * n_fft_max_y / _fft.n_fft;
			if (fft_max_y > 0.02) the_screen << printfmt("  \033[32;47mＹ\033[0m %3d.%02dHz %1.2f", cHz / 100, cHz % 100, fft_max_y) << crlf;
			else the_screen << "\033[K  \033[32;47mＹ\033[0m       Hz" << crlf;

			cHz = _fft.cHz * n_fft_max_z / _fft.n_fft;
			if (fft_max_z > 0.02) the_screen << printfmt("  \033[34;47mＺ\033[0m %3d.%02dHz %1.2f", cHz / 100, cHz % 100, fft_max_z) << crlf;
			else the_screen << "\033[K  \033[34;47mＺ\033[0m       Hz" << crlf;
		}
		else {
			// clear the FFT information area
			for (int i = row; i <= row + 6; i++) the_screen.clear_line(i);
		}

		row += 7;
		if (LOG(_i_view_current)) {
			the_screen(0, row++) << "[取得ﾃﾞｰﾀ保存中]";
			the_screen(0, row++) << " (\033[4ml\033[0m)->保存先開く";
		}
	}


	/**
	 * for return value of locate_data_buffer()
	 */
	struct _LOCATE_BUFFER {
		_LOCATE_BUFFER(int i, int a) : _idx(i), _active(a) {}
		explicit operator bool() { 
			return _idx != -1; 
		}
		operator int() { return _idx; }

	private:
		int _idx;		// the index value when seach the buffer _d[]
		int _active;	// number of active(data is stored) buffers in _d[]
		int get_active_count() { return _active; } // not used so far
	};

	/**
	 * FIND BUFFER INDEX of _d by Node ID..
	 * - if new ID is found, allocate memory and open log file.
	 * 
	 * \param id		the module SDI
	 * \return			if _LOCATE_BUFFER with idx of -1, not found.
	 */
	_LOCATE_BUFFER locate_data_buffer(uint32_t id) {
		int active = 0;
		for (int i = 0; i < BUFFER_COUNT; i++) {
			if (D(i)._id != 0) active++;
		}

		if (id == 0) { // NEXT NODE
			for (int i = 1; i < BUFFER_COUNT; i++) {
				int j = (i + _i_view_current) % BUFFER_COUNT;
				if (D(j)._id != 0) {
					_i_view_current = j; // select next view
					break;
				}
			}
			return _LOCATE_BUFFER(_i_view_current, active);
		}
		else {
			for (int i = 0; i < BUFFER_COUNT; i++) {
				if (id == D(i)._id) {
					return _LOCATE_BUFFER(i, active);
				}
			}
			for (int i = 0; i < BUFFER_COUNT; i++) {
				if (D(i)._id == 0) {
					// the new entry (allocate  buffer memory)
					D(i).alloc_data(id);
					D(i).open_log();

					if (LOG(i)) {
						_INFO info;
						export_info_append_generic_info(info, id, LOG(i).get_time_opened());

						try {							
							export_header_w_info(LOG(i).os(), true);

							XYZT val{}; // dummy data.
							for (int j = 0; j < info.get_count(); j++) {
								export_single_data(LOG(i).os(), -1, val, false);
								export_single_data_info(LOG(i).os(), info, true);
							}
						} catch(...) {
							LOG(i).close();
						}
					}

					active++;
					return _LOCATE_BUFFER(i, active);
				}
			}

			return _LOCATE_BUFFER(-1, active);
		}
	}

	/**
	 * PARSE SERIAL INPUT BYTE BY BYTE.
	 * - once data is loaded, puta accel data into temporary buffer _d._w[].
	 * 
	 * \param u8b		incoming byte from Serial
	 * \return			if true, byte sequence is completed.
	 */
	bool parse_a_byte(char_t u8b) {
		bool b_updated = false;

		// parse.
		parse_ascii << u8b;

		// if complete parsing
		if (parse_ascii) {
			// output as parser format
			the_screen_b.clear_screen();
			the_screen_b << printfmt("PKT(%04d)", _pkt_rcv_ct++);

			// payload
			auto&& p = parse_ascii.get_payload();

			// 1. identify the packet type
			auto&& pkt = newTwePacket(parse_ascii.get_payload());
			the_screen_b << ":Typ=" << int(identify_packet_type(pkt));

			// TAG(2525A) style packets
			// e.g. :80000000 C60FDC81 0A7F8801 35DE058F 0000FA0A FC50FF60FEF8FC48FF58FED8FC30FF68FEE8FC40FF60FED8FC50FF68FEB8FC48FF60FED8FC48FF70FEB8FC48FF60FEB8FC50FF58FEC0FC48FF60FED01F
			if (identify_packet_type(pkt) == E_PKT::PKT_APPTAG) {
				auto&& dat = refTwePacketAppTAG(pkt);

				if (dat.u8sns == 0x35) { // FIFO packet
					unsigned len = dat.payload.length();
					if (len < 12) return false; // too short

					uint8_t mode = dat.payload[4];
					int32_t datcount = dat.payload[5];

					the_screen_b << printfmt(":M=%02X:Ct=%d", mode, datcount);

					if (datcount > 0) {
#if 0
						// TEST CODE IF MULTIPLE BUFFERS
						{
							static int i;
							dat.u32addr_src += (i++ % 5);
						}
#endif

						if (auto idx = locate_data_buffer(dat.u32addr_src)) {
							auto time_packet = millis();
							auto sample_dur = assume_sample_period_us(idx, time_packet, datcount);

							ID(idx) = dat.u32addr_src;

							// store data
							uint8_t* p = dat.payload.begin().raw_ptr() + 6;
							int32_t x_ave = 0;
							int32_t y_ave = 0;
							int32_t z_ave = 0;
							for (int i = 0; i < datcount; i++) {
								int16_t x = (int16_t)TWEUTILS::G_WORD(p);
								int16_t y = (int16_t)TWEUTILS::G_WORD(p);
								int16_t z = (int16_t)TWEUTILS::G_WORD(p);
								x_ave += x;
								y_ave += y;
								z_ave += z;

								XYZT val = { x,y,z,time_packet + ((i * sample_dur) >> 10),time_packet, dat.u16seq };
								W(idx).push(val);

								if (LOG(idx)) {
									export_single_data(LOG(idx).os(), ++CT(idx), val);
								}
							}

							the_screen_b << printfmt("/%5d,%5d,%5d", x_ave / datcount, y_ave / datcount, z_ave / datcount);
							b_updated = true;
						}
					}
				}
			}

			// PAL standard packet (CUE, MOT)
			if (identify_packet_type(pkt) == E_PKT::PKT_PAL) {
				auto&& pal = refTwePacketPal(pkt);
				// put information
				the_screen_b
					<< printfmt(":Lq=%d:Ad=%08X", pal.u8lqi, pal.u32addr_src)
					<< ":PAL=" << printfmt("%d", pal.u8palpcb)
					<< ":ID=" << printfmt("%d", pal.u8addr_src)
					<< crlf
					;

				auto paltype = pal.get_PalDataType();

				if (paltype == E_PAL_DATA_TYPE::EX_CUE_STD)
				{
					// generate TWELITE CUE standard data
					TweCUE cue = pal.get_TweCUE();

					if (cue.has_accel()) {
						the_screen_b << ":SAMPLES=" << int(cue.get_accel_count_u8());
						the_screen_b << ":SR=" << int(cue.u8sample_rate_code);

						if (auto idx = locate_data_buffer(pal.u32addr_src)) {
							auto datcount = cue.get_accel_count_u8();
							auto time_packet = millis();
							auto sample_dur = assume_sample_period_us(idx, time_packet, datcount);

							int ave_x = 0, ave_y = 0, ave_z = 0;
							for (int i = 0; i < cue.get_accel_count_u8(); i++) {
								int16_t x = cue.i16X[i];
								int16_t y = cue.i16Y[i];
								int16_t z = cue.i16Z[i];

								ave_x += x; ave_y += y; ave_z += z;

								XYZT val = { x,y,z,time_packet + ((i * sample_dur) >> 10),time_packet, pal.u16seq };
								W(idx).push(val);

								if (LOG(idx)) {
									export_single_data(LOG(idx).os(), ++CT(idx), val);
								}
							}

							// insert dummuy data
							unsigned t = ((cue.get_accel_count_u8() * sample_dur) >> 10);
							for (int i = 0; i < 4; i++) {
								W(idx).push({ 0,0,0,0 });
							}

							the_screen_b << printfmt("/%5d,%5d,%5d", ave_x / datcount, ave_y / datcount, ave_z / datcount);
							b_updated = true;
						}
					}
				}

				if (paltype == E_PAL_DATA_TYPE::MOT_STD)
				{
					// generate pal board specific data structure.
					PalMot mot = pal.get_PalMot();

					if (auto idx = locate_data_buffer(pal.u32addr_src)) {
						auto datcount = mot.get_accel_count_u8();
						auto time_packet = millis();
						auto sample_dur = assume_sample_period_us(idx, time_packet, datcount);

						the_screen_b << ":SAMPLES=" << int(datcount);
						the_screen_b << ":SR=" << int(mot.u8sample_rate_code);

						// SEQUENCE CHECK
						{
							static uint16_t u16seq_last = 0;
							static int seq_cont = 0;

							if (u16seq_last + 1 == pal.u16seq) {
								seq_cont++;
							}
							else {
								seq_cont = 0;

								// insert dummy data
								for (int i = 0; i < 4; i++) {
									W(idx).push({ 0,0,0,0,0 });
								}
							}

							u16seq_last = pal.u16seq;

							the_screen_b << ":SQT=" << seq_cont;
						}

						int ave_x = 0, ave_y = 0, ave_z = 0;
						for (int i = 0; i < mot.u8samples; i++) {
							if (mot.u32StoredMask & (1UL << (i + 1))) { // check if stored properly.
								int16_t x = mot.i16X[i];
								int16_t y = mot.i16Y[i];
								int16_t z = mot.i16Z[i];

								ave_x += x; ave_y += y; ave_z += z;

								XYZT val = { x,y,z,time_packet + ((i * sample_dur) >> 10), time_packet, pal.u16seq };
								W(idx).push(val);

								if (LOG(idx)) {
									export_single_data(LOG(idx).os(), ++CT(idx), val);
								}
							}
						}

						the_screen_b << printfmt("/%5d,%5d,%5d", ave_x / datcount, ave_y / datcount, ave_z / datcount);
						b_updated = true;
					}
				}
			}
		}

		return b_updated;
	}

	/**
	 * assume packet sample rate for multi-sample packet.
	 *
	 * @param	ts_pkt		 	Message describing the ts.
	 * @param	n_samples_pkt	Message describing the samples.
	 *
	 * @returns	An uint32_t. ms * 1024
	 */
	uint32_t assume_sample_period_us(int idx, uint32_t ts_pkt, uint32_t n_samples_pkt) {
		if (!(unsigned(idx) < BUFFER_COUNT && _d[idx]._id != 0)) return 0;

		auto& t = T(idx);
		t.push_force(ts_pkt);

		int n_samples = 0;
		uint32_t period = 0;
		if (t.size() >= 3) {
			for (int i = 1; i < t.size(); i++) {
				uint32_t td = t[i] - t[i - 1];
				if (td <= 2000) { // if pacekt duration excess 2sec, ignore it.
					n_samples += n_samples_pkt;
					period += td;
				}
			}
		}

		if (n_samples >= 2) period = period * 1024 / (n_samples - 1);
		else period = 10 * 1024; // default is 10ms

		return period;
	}

	/**
	 * reset the view.
	 * - called when viewing SID is changed.
	 */
	void reset_view_param() {
		_fft.tick_fft_hold = 0; // discard hold data
		_view.reset(); // reset view paramter
	}

	/**
	 * called when button was pressed.
	 * 
	 * \param id   the pressed button ID
	 * \param opt  button parameters
	 */
	void Btn_Press_FFT_Toggle(int id, uint32_t opt) {
		_fft.set_fft_size(0); // change fft size to next param.

		_fft.tick_fft_hold = 0; // discard hold data
		validate_view_magnify(); // if magnify is too large, adjust it.

		UpdateLbl_Btn_FFT();
		update_screen();
	}

	/**
	 * Update the label of FFT size button.
	 */
	void UpdateLbl_Btn_FFT() {
		auto& b = _btns[_id_btns.fft];
		SmplBuf_ByteS l;
		l << printfmt("(&f)FFT : %03d", _fft.n_fft);
		b.get_label().emptify() << l.c_str();
		b.update_view(); // need to call this for updating label quickly.
	}

	/**
	 * called when button was pressed.
	 *
	 * \param id   the pressed button ID
	 * \param opt  button parameters
	 */
	void Btn_Press_NODE_Toggle(int id, uint32_t opt) {
		auto idx = locate_data_buffer(0);
		reset_view_param();
		UpdateLbl_Btn_Node(int(idx));
	}

	/**
	 * update the button label of Node button.
	 * 
	 * \param idx  the new index of _d[].
	 */
	void UpdateLbl_Btn_Node(int idx) {
		auto& b = _btns[_id_btns.nodes];
		SmplBuf_ByteS l;
		l << printfmt("(&i)ID#%d: %08X", idx, ID(idx));
		b.get_label().emptify() << l.c_str();
		b.update_view(); // need to call this for updating label quickly.
	}


	/**
	 * called when button was pressed.
	 *
	 * \param id   the pressed button ID
	 * \param opt  button parameters
	 */
	void Btn_Press_CSV_OUT(int id, uint32_t opt) {
		export_sensor_data();
	}

	/**
	 * called when button was pressed.
	 *
	 * \param id   the pressed button ID
	 * \param opt  button parameters
	 */
	void Btn_Press_PAUSE(int id, uint32_t opt) {
		_b_pause = !_b_pause;

		UpdateLbl_PAUSE();
	}

	/**
	 * update the label of PAUSE button.
	 */
	void UpdateLbl_PAUSE() {
		auto& b = _btns[_id_btns.pause];
		SmplBuf_ByteS l;

		if (_b_pause) {
			l << printfmt("＞ PAUSED(& )"); // press to PLAY >
		}
		else {
			l << printfmt("□ PAUSE (& )"); // press to PAUSE []
		}

		b.get_label().emptify() << l.c_str();
		b.update_view(); // need to call this for updating label quickly.
	}

	/**
	 * called when button was pressed.
	 *
	 * \param id   the pressed button ID
	 * \param opt  button parameters
	 */
	void Btn_Press_HELP(int id, uint32_t opt) {
		shell_open_url(L"https://stage.twelite.info/usage/screens/main_menu/viewer/cue_viewer");
	}

	/**
	 * CSV export: _INFO part
	 * 
	 * \param os		output stream
	 * \param info		info data
	 * \param b_endl	true to output endl.
	 */
	void export_single_data_info(std::ostream& os, _INFO& info, bool b_endl = true) {
		info.csv_out(os);
		if (b_endl) os << std::endl;
	}

	/**
	 * CSV export: FFT part.
	 * 
	 * \param os		output stream
	 * \param idx_fft	the index of FFT result.
	 * \param b_endl	true to output endl.
	 */
	void export_single_data_fft(std::ostream& os, int idx_fft, bool b_endl = true) {
		SmplBuf_ByteSL<255> line;

		if (idx_fft < 0) {
			line << ",,,,,";
		}
		else {
			if (idx_fft == 0) {
				line << printfmt(",DC,0");
			}
			else {
				line << printfmt(",%d,%.2f", idx_fft, _fft.cHz * idx_fft / _fft.n_fft / 100.);
			}
			line << printfmt(",%1.3f,%1.3f,%1.3f", _fft.vfft_abs[0][idx_fft], _fft.vfft_abs[1][idx_fft], _fft.vfft_abs[2][idx_fft]);
		}

		os << line.c_str();

		if (b_endl) os << std::endl;
	}

	/**
	 * CSV export: basic data (XYZT and etc.)
	 * 
	 * \param os		output stream
	 * \param idx		the index of data
	 * \param val		a single XYZT data
	 * \param b_endl	true to output endl.
	 */
	void export_single_data(std::ostream& os, int idx, XYZT& val, bool b_endl = true) {
		SmplBuf_ByteSL<255> line;

		double d_val_x = val.x / 1000.;
		double d_val_y = val.y / 1000.;
		double d_val_z = val.z / 1000.;

		if (val.t != 0) {
			line << printfmt("%d,%d,%d,%d", idx, val.t_pkt, val.seq, val.t);
			line << printfmt(",%1.3f,%1.3f,%1.3f", d_val_x, d_val_y, d_val_z);
		}
		else {
			if (idx >= 0) line << printfmt("%d", idx);
			line << ",,,,,,";
		}

		os << line.c_str();

		if (b_endl) os << std::endl;
	}

	/**
	 * CSV export: header part of _INFO.
	 * 
	 * \param os		output stream.
	 * \param b_endl	true to output endl.
	 */
	void export_header_info(std::ostream& os, bool b_endl = true) {
		os << ",Label,Info";
		if (b_endl) os << std::endl;
	}

	/**
	 * CSV export: header part of FFT.
	 *
	 * \param os		output stream.
	 * \param b_endl	true to output endl.
	 */
	void export_header_fft(std::ostream& os, bool b_endl = true) {
		os << ",FFT#,Hz,X,Y,Z";
		if (b_endl) os << std::endl;
	}

	/**
	 * CSV export: header part of basic data (XYZT and etc.)
	 *
	 * \param os		output stream.
	 * \param b_endl	true to output endl.
	 */
	void export_header(std::ostream& os, bool b_endl = true) {
		os << "#,T_PKT[ms],SEQ,T_SMPL[ms],X[G],Y[G],Z[G]";
		if (b_endl) os << std::endl;
	}

	/**
	 * CSV export: export header of basic and info
	 *
	 * \param os		output stream.
	 * \param b_endl	true to output endl.
	 */
	void export_header_w_info(std::ostream& os, bool b_endl = true) {
		export_header(os, false);
		export_header_info(os, false);

		if (b_endl) os << std::endl;
	}

	/**
	 * CSV export: export header of basic, FFT and info
	 *
	 * \param os		output stream.
	 * \param b_endl	true to output endl.
	 */
	void export_header_w_fft_info(std::ostream& os, bool b_endl = true) {
		export_header(os, false);
		export_header_fft(os, false);
		export_header_info(os, false);

		if (b_endl) os << std::endl;
	}

	/**
	 * CSV export: prepare _INFO data of basic ones.
	 * 
	 * \param info		data structure to be strored.
	 * \param id		module SID
	 * \param lt		the local time when CSV file is created
	 */
	void export_info_append_generic_info(_INFO& info, uint32_t id, TWESYS::TweLocalTime& lt) {
		SmplBuf_ByteSL<128> _txt_day;   // date text "YYYYMMDD_hhmmss"
		SmplBuf_ByteSL<128> _txt_time;   // date text "YYYYMMDD_hhmmss"
		SmplBuf_ByteSL<128> _txt_time_ms;   // date text "YYYYMMDD_hhmmss"

		// convert into text
		_txt_day << printfmt("%04d/%02d/%02d", lt.year, lt.month, lt.day);
		_txt_time << printfmt("%02d:%02d:%02d", lt.hour, lt.minute, lt.second);
		_txt_time_ms << printfmt("%d", lt.ms);

		// append generic info
		info.append("ModuleSID") << format("%08X", id);
		info.append("Tick[ms]", int(millis() & 0x7FFFFFFF));
		info.append("Date", _txt_day.c_str());
		info.append("Time", _txt_time.c_str());
		info.append("Time_Msec_part", _txt_time_ms.c_str());
	}

	/**
	 * CSV export: export data shown at the screen.
	 * 
	 * \return true if completed.
	 */
	bool export_sensor_data() {
		if (!b_view_has_data(_i_view_current)) return false; // no data (do nothing)
		if (_view.item_count == 0) return false;

		// if (!_b_logging) {
		SmplBuf_ByteSL<128> _txt_day;   // date text "YYYYMMDD_hhmmss"
		SmplBuf_ByteSL<128> _txt_time;   // date text "YYYYMMDD_hhmmss"
		SmplBuf_ByteSL<128> _txt_time_ms;   // date text "YYYYMMDD_hhmmss"
		SmplBuf_WChar _file_name;       // "twestage_YYYYMMDD_hhmmss.log"

		// open log file
		TweLogFile logf(LOG_SNAP_NAME, LOG_FILEEXT);

		if (logf.open()) {
			// hist_size : effective sample count
			int32_t hist_size = _view.last_index + 1;
			if (hist_size > VIEW_SIZE) hist_size = VIEW_SIZE;

			// sample index number
			int idx = 1; // sample index number 1..512

			// OUTPUT_SIZE : output samples to the csv file.
			const int32_t OUTPUT_SIZE = VIEW_SIZE;

			// info part
			_INFO info;
			export_info_append_generic_info(info, ID(_i_view_current), logf.get_time_opened());

			info.append("Samples", int(hist_size));

			if (_fft.b_fft_calculated || _fft.b_fft_hold) {
				info.append("FFT_Len", int(_fft.n_fft));
				info.append("FFT_Start#", int(OUTPUT_SIZE - _fft.n_fft + 1));
				info.append("FFT_Freq[Hz]") << format("%.2f", _fft.cHz / 100.);
			}

			try {
				// label
				export_header_w_fft_info(logf.os(), true);

				// export all data
				auto& v = V(_i_view_current);
				auto& id = ID(_i_view_current);

				// insert dummy sample if samples in the history is less than HISTORY_SIZE/
				if (hist_size < OUTPUT_SIZE) {
					XYZT dummy{};
					for (int i = hist_size; i < OUTPUT_SIZE; i++, idx++) {
						export_single_data(logf.os(), idx, dummy, false);
						export_single_data_fft(logf.os(), -1, false);
						export_single_data_info(logf.os(), info, true);
					}
				}

				// put data from v.
				for (int i = 0; i < hist_size; i++, idx++) {
					int idx_fft = i - (hist_size - _fft.n_fft);
					XYZT val = (_fft.b_fft_hold && idx_fft >= 0) ? _fft.v[idx_fft] : v[i];

					// sensor basic data
					export_single_data(logf.os(), idx, val, false);

					// FFT data (if present)
					if (!((_fft.b_fft_calculated || _fft.b_fft_hold) && idx_fft >= 0 && idx_fft < _fft.n_fft / 2)) {
						idx_fft = -1;
					}
					export_single_data_fft(logf.os(), idx_fft, false);

					// info
					export_single_data_info(logf.os(), info, true);
				}

				// close file
				logf.close();

				delay(100);

				// open log file
				logf.shell_open();

				return true;
			}
			catch (std::ios_base::failure& e) {
				std::cerr << std::endl << "SCR_CUE_FIFO::export_sensor_data(): " << e.what();
				logf.close();
				return false;
			}
		}

		return false;
	}

	/**
	 * the setup() when the screen is created.
	 */
	void setup() {
		// clean/hide main screen area
		M5.Lcd.fillRect(_area_draw.x, _area_draw.y, _area_draw.w, _area_draw.h, ALMOST_WHITE);

		// bottom screen
		the_screen_b.clear_screen();
		_app.set_title_bar(PAGE_ID::PAGE_CUE_FIFO);
		_app.set_nav_bar();

		// main screen
		the_screen.clear_screen();
		//the_screen << "\033[7m\033[K 加速度グラフ\033[0m" << crlf;
		the_screen << "\033[K\033[1m加速度グラフ\033[0m" << crlf;

		int row = 3, col = 1;

		_id_btns.nodes = _btns.add(col, row, L"(&i)ID#-: --------", &SCR_CUE_FIFO::Btn_Press_NODE_Toggle, this, 0);
		row++;

		_id_btns.fft = _btns.add(col, row, L"(&f)FFT : ###", &SCR_CUE_FIFO::Btn_Press_FFT_Toggle, this, 0);
		UpdateLbl_Btn_FFT();
		row++;

		row = the_screen.get_rows() - 3;
		_id_btns.csvout = _btns.add(col, row, L"(&c)表示ﾃﾞｰﾀ保存", &SCR_CUE_FIFO::Btn_Press_CSV_OUT, this, 0);

		row = the_screen.get_rows() - 1;
		col = 7;
		_id_btns.pause = _btns.add(col, row, L"  PAUSE (& )", &SCR_CUE_FIFO::Btn_Press_PAUSE, this, 0);
		UpdateLbl_PAUSE();

		row = 1;
		col = the_screen.get_cols() - 1 - 9;
		_btns.add(col, row, L"[ﾍﾙﾌﾟ(&H)]", &SCR_CUE_FIFO::Btn_Press_HELP, this, 0);
	}

	void validate_view_magnify() {
		if (_view.magnify < 1) _view.magnify = 1;
		else if (_view.magnify > 4) _view.magnify = 4;

		if (_fft.n_fft == 256 && _view.magnify > 2) _view.magnify = 2;
	}


	/**
	 * the loop(), called periodically from App_CUE parent class.
	 */
	void loop() {
		uint32_t t_now = millis();
		static uint32_t t_last_flush;

		// flush log file every seconds
		if (t_now - t_last_flush > 1000) {
			for (int i = 0; i < BUFFER_COUNT; i++) {
				if (b_view_has_data(i) && LOG(i)) {
					LOG(i).flush();
				}
			}

			t_last_flush = t_now;
		}

		// check events
		_btns.check_events();

		// check incoming buffer
		for (int i = 0; i < BUFFER_COUNT; i++) {
			if (!b_view_has_data(i)) continue; // no data (do nothing) 

			// Packet has not come within PACKET_TIMEOUT_ms (insert dummy data)
			bool b_insert_dummy = false;
			if (!W(i).empty()) {
				if (W(i).back().t_pkt != 0 && (t_now - W(i).back().t_pkt > PACKET_TIMEOUT_ms)) {
					for (int j = 0; j < 4; j++) {
						W(i).push_force({ 0,0,0,0 });
					}
				}
			}
			else if (!V(i).empty()) {
				if (V(i).back().t_pkt != 0 && (t_now - V(i).back().t_pkt > PACKET_TIMEOUT_ms)) {
					for (int j = 0; j < 4; j++) {
						V(i).push_force({ 0,0,0,0 });
					}
				}
			}

			if (!W(i).empty()) {
				bool b_updated = false;

				if (!_b_pause) {
					// Insert new data from W(i) to V(i)
					while (!W(i).empty()) {
						auto t_diff = (W(i).front().t == 0) ? 0 : W(i).front().t - t_now;
						if (t_diff == 0 || t_diff > 0x8000000) {
							b_updated = true;
							V(i).push_force(W(i).pop_front());
						}
						else break;
					}

					if (b_updated && i == _i_view_current) update_screen();
				}
			}
		}

		// check key press
		do {
			int c = the_keyboard.read();

			if (KeyInput::is_mouse_left_down(c)) {
				TWECUI::KeyInput::_MOUSE_EV ev(c);

				int x = ev.get_x();
				int y = ev.get_y();
				
				if (ev.is_left_btn() && _view.is_in_area(_area_draw, x, y)) {
					_view.x_drag = x;
					_view.y_drag = y;
					_view.shift_drag = _view.shift;
					_view.b_dragging = true;
					_view.b_dragfast = false;
					if (_view.is_in_area(_area_scrollbar, x, y)) {
						// align dragging step to the scroll bar.
						_view.b_dragfast = true;
					}
				}
			}
			else if ( KeyInput::is_mouse_right_down(c) ){
				Btn_Press_PAUSE(0, 0);
			}
			else if (KeyInput::is_mouse_left_up(c)) {
				if (_view.b_dragging) {
					_view.b_dragging = false;
					_view.b_dragfast = false;
				}
			}
			else if (KeyInput::is_mouse_move(c)) {
				TWECUI::KeyInput::_MOUSE_EV ev(c);

				int x = ev.get_x();
				int y = ev.get_y();

				// save last coord
				_view.x_last = x;
				_view.y_last = y;

				// update view
				if (_view.b_dragging) {
					int32_t x_fast = 16;
					if (_view.b_dragfast) {
						// align dragging step to the scroll bar.
						// calculate step size(x_fast) by the samples in the screen and total sample size.
						int sz = V(_i_view_current).size();
						if (sz && _view.item_count) x_fast = sz * 16 / (VIEW_SIZE / _view.magnify);
						x_fast = _area_draw.w * x_fast / _area_scrollbar.w; // adjust x_fast by the scroll bar in the screen.
						if (x_fast < 16) x_fast = 16; // minimum check.
						x_fast = -x_fast; // opposite direction from normal drag.
					}
					_view.shift = _view.shift_drag - (_view.x_drag - x) * x_fast / _view.magnify / 16;
					if (_view.shift < 0) _view.shift = 0;
					update_screen();
				}
				// the_screen_b << format("M(%d,%d)", x, y);
			}
			else if (KeyInput::is_mouse_wheel(c)) {
				if (_view.is_in_area(_area_draw, _view.x_last, _view.y_last)) {
					TWECUI::KeyInput::MOUSE_WHEEL ev(c);
					if (ev.get_y() > 0) {
						// scroll down
						_view.magnify--;
						validate_view_magnify();
						update_screen();
					}
					else if (ev.get_y() < 0) {
						// scroll up
						_view.magnify++;
						validate_view_magnify();
						update_screen();
					}
				}
			}
			else switch (c) {
			case KeyInput::KEY_BUTTON_A:
				break;
			case KeyInput::KEY_BUTTON_B:
				break;
			case KeyInput::KEY_BUTTON_C:
				break;

			case 'i': case 'I':
				Btn_Press_NODE_Toggle(0, 0);
				break;

			case 'f': case 'F':
				Btn_Press_FFT_Toggle(0, 0);
				break;

			case 'c': case 'C':
				export_sensor_data();
				break;

			case 'h': case 'H':
				Btn_Press_HELP(0, 0);
				break;

			case 'l': case 'L':
				if (b_view_has_data(_i_view_current)) {
					if (LOG(_i_view_current)) {
						shell_open_folder(the_cwd.get_dir_log());
					}
				}
				break;

			case KeyInput::KEY_RIGHT:
				_view.shift--;
				if (_view.shift < 0) _view.shift = 0;
				update_screen();
				break;

			case KeyInput::KEY_LEFT:
				_view.shift++;
				update_screen();
				break;

			case KeyInput::KEY_UP:
				_view.magnify++;
				validate_view_magnify();
				update_screen();
				break;

			case KeyInput::KEY_DOWN:
				_view.magnify--;
				if (_view.magnify < 1) _view.magnify = 1;
				update_screen();
				break;

			case KeyInput::KEY_ESC:
				Btn_Press_PAUSE(0, 0);
				break;

			case ' ':
				Btn_Press_PAUSE(0, 0);
				break;

			default:
				break;
			}
		} while (the_keyboard.available());

		// read the uart queue
		do {
			int c = the_uart_queue.read();

			if (c >= 0) {
				parse_a_byte(c);
			}

		} while (the_uart_queue.available());
	}

	/**
	 * called when closing the screen.
	 * 
	 */
	void on_close() {
		// restore main screen.
		the_screen.set_draw_area(_area_scr_main);
		the_screen.set_font(_app.font_IDs.main);
		the_screen.visible(true);
		the_screen.clear_screen();
		the_screen.force_refresh();
	}
};

/**
 * generate handler instance (SCR_XXX needs to have setup(), loop(), on_close() methods).
 */
void APP_BASE::hndr_SCR_CUE_FIFO(event_type ev, arg_type arg) { hndr<SCR_CUE_FIFO>(ev, arg); }
