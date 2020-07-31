#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <utility>
#include <algorithm>
#include <type_traits>

#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_utils_simplebuffer.hpp"
#include "twe_utils_unicode.hpp"
#include "twe_sys.hpp"

#ifdef ESP32
#include <M5Stack.h>
#else
#include <iostream>
#include <fstream>
#include <filesystem>
#endif

#if _DEBUG
extern "C" int printf_(const char*, ...);
#endif

namespace TWE {
#ifdef ESP32
	const size_t TWE_FILE_NAME_TYPLEN = 63; // used for pre alloc for array object (resizable)
	const size_t TWE_FILE_NAME_MAX = 255;
#else
    const size_t TWE_FILE_NAME_TYPLEN = 255; // used for pre alloc for array object (resizable)
    const size_t TWE_FILE_NAME_MAX = 1023;
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
	const wchar_t WCHR_PATH_SEP = L'\\';
#else
	const wchar_t WCHR_PATH_SEP = L'/';
#endif

	/**
	 * @fn	static inline bool is_pathsep(wchar_t x)
	 *
	 * @brief	Query if 'x' is path separator char.
	 *
	 * @param	x	A wchar_t to process.
	 *
	 * @returns	True if pathsep, false if not.
	 */
	static inline bool is_pathsep(wchar_t x) {
		return (x == L'/' || x == L'\\');
	}

	/**
	 * @fn	static inline TWEUTILS::SmplBuf_WChar::iterator find_last_pathsep(TWEUTILS::SmplBuf_WChar& path)
	 *
	 * @brief	Searches for the last pathsep
	 *
	 * @param [in,out]	path	Full pathname of the file.
     *                          NOTE: should be const input, but crbegin() and crend() is not confirmed.
	 *
	 * @returns	The found pathsep.
	 */
	static inline TWEUTILS::SmplBuf_WChar::const_iterator find_last_pathsep(const TWEUTILS::SmplBuf_WChar& path) {
		// find the last path separator
		//TWEUTILS::SmplBuf_WChar::riterator
        auto it_bkw = std::find_if(
			path.crbegin(), path.crend(),
			[](const wchar_t& x) {
				return is_pathsep(x);
			});

		return (it_bkw != path.crend()) ? 
			it_bkw.get_forward() 
			: path.cend();
	}

    // split path into dir part and file part
    static inline void split_fullpath_into_dir_file(
              const TWEUTILS::SmplBuf_WChar& _full
            , TWEUTILS::SmplBuf_WChar& dir_
            , TWEUTILS::SmplBuf_WChar& file_)
    {        
        dir_.clear();
        file_.clear();

        TWEUTILS::SmplBuf_WChar full; // NOTE: copy constructor of SimpleBuffer<> is not defined so far.
        full = as_copying(_full);
        if(full.empty()) return;

        // remove last PATHSEP
        if (is_pathsep(full[-1])) {
            full.pop_back();
        }

        auto it = find_last_pathsep(full);
        if (it != full.cend()) {
            dir_.reserve_and_set_empty(it - full.cbegin());
            dir_ << std::make_pair(full.cbegin(), it);

            file_.reserve_and_set_empty(full.cend() - it);
            file_ << std::make_pair(it + 1, full.cend());
        }
    }

    /**************************************************************
     * make_full_path_ext
     *   - generate full path accepting multiple arguments.
     **************************************************************/

    /** @brief	Check if fname is empty string or not. (for SmplBuf_WChar) */
    template <typename T
		, typename = typename std::enable_if<
			std::is_same<
				TWEUTILS::SmplBuf_WChar,
				typename std::decay<T>::type
			>::value
		>::type
    >
    int _make_full_path_is_empty(T& fname) {
        int r = ((int)fname.size() == 0);

        // check if fname is "."(as empty) or ".."(returns -1).
        if (fname.size() == 1 && fname[0] == 0x2e) r = 1;
        if (fname.size() == 2 && fname[0] == 0x2e && fname[1] == 0x2e) r = -1;

        return r;
    }

    /** @brief	Check if fname is empty string or not. (for wchar_t[]) */
    template <typename T, int N
        , typename = typename std::enable_if<
        	std::is_integral<typename std::remove_reference<T>::type>::value &&
		    std::is_same<
				wchar_t,
			    typename std::remove_const<
					typename std::remove_reference<T>::type
			    >::type
		    >::value // is_same
        >::type // enable_if
    > // template 
    int _make_full_path_is_empty(T(&fname)[N]) {
        int r = (N == 1);

        // check if fname is "."(as empty) or ".."(returns -1).
        if (N == 1 + 1 && fname[0] == 0x2e) r = 1;
        if (N == 2 + 1 && fname[0] == 0x2e && fname[1] == 0x2e) r = -1;

        return r;
    }

    /** @brief	Check if fname is empty string or not. (for wchar_t*) */
    template <typename T
        , typename = typename std::enable_if<
            std::is_pointer<typename std::remove_reference<T>::type>::value &&
            std::is_same<
				wchar_t,
	            typename std::remove_const<
		            typename std::remove_pointer<
			            typename std::remove_reference<T>::type
		            >::type
	            >::type
            >::value // is_same
        >::type // enable_if
    > // template
    int _make_full_path_is_empty(T fname) {
        int r = (fname[0] == 0);

        // check if fname is "."(as empty) or ".."(returns -1).
        if (!r && fname[0] == 0x2e && fname[1] == 0) r = 1;
        if (!r && fname[0] == 0x2e && fname[1] == 0x2e && fname[2] == 0) r = -1;

        return r;
    }

    /** @brief parameter pack recursive process, the final call (w/ empty arg)
     *         - remove last PATHSEP char.
     */
    static inline void _make_full_path_copy(TWEUTILS::SmplBuf_WChar& buf, wchar_t sep) {
        buf.pop_back();
        buf.c_str(); // this is NOT mandate, but for better viewing on debugger.
    }

    /** @brief parameter pack recursive process, copy fname into buf. */
    template <class Head, class... Tail>
    static inline void _make_full_path_copy(TWEUTILS::SmplBuf_WChar& buf, wchar_t sep, Head&& fname, Tail&&... tail) {
        int emp = _make_full_path_is_empty(fname);

        if (emp == 1) {
            // fname is empty or ".", just skip
            ; 
        }
        else if (emp == 0) {
            // fname has file or dir name, add fname to buf.
            buf << fname;

            // append '/'or'\\' at the tail.
            if (buf[-1] != sep) {
                buf.push_back(sep);
            }
        }
        else if (emp == -1) {
            // fname is "..", take last name back.
            
            // remove '/'or'\\' at the tail.
            if (buf[-1] == sep) {
                buf.pop_back();
            }

            // find the last ('/'or'\\')
            auto p = find_last_pathsep(buf);
            if (p == buf.cend()) {
                // not find any '/'or'\\', clean buffer.
                buf.emptify(); 
            }
            else {
                // trunk as ..
                int new_len = p - buf.cbegin();
                buf.resize(new_len);

                // add ('/'or'\\')
                buf.push_back(sep);
            }
            
        }
		
        _make_full_path_copy(buf, sep, std::forward<Tail>(tail)...);
    }

    /**
     * @fn	template <class... Tail> static inline TWEUTILS::SmplBuf_WChar make_full_path_ext(TWEUTILS::SmplBuf_WChar& dir, Tail&&... tail)
     *
     * @brief	Makes full path
     *
     * @tparam	Tail	Type of the tail.
     * @param [in,out]	dir 	The dir.
     * @param 		  	tail	Variable arguments providing [in,out] The tail.
     *
     * @returns	A TWEUTILS::SmplBuf_WChar. (move operator =() will be called)
     */
    template <class... Tail>
    static inline TWEUTILS::SmplBuf_WChar make_full_path(Tail&&... tail) {
        TWEUTILS::SmplBuf_WChar buf(TWE_FILE_NAME_TYPLEN);
        _make_full_path_copy(buf, WCHR_PATH_SEP, std::forward<Tail>(tail)...);
        return std::move(buf);
    }

    /** @brief	combile with "." */
    template <class... Tail>
    static inline TWEUTILS::SmplBuf_WChar make_file_ext(Tail&&... tail) {
        TWEUTILS::SmplBuf_WChar buf(TWE_FILE_NAME_TYPLEN);
        _make_full_path_copy(buf, L'.', std::forward<Tail>(tail)...);
        return std::move(buf);
    }
  

#ifndef ESP32
    class TweCmdPipe {
        typedef TweCmdPipe tself;

        const int MAX_LINE_CHARS;
        FILE* _fp;
        int _exit_code;
        
        void _close();

    public:
        // copy
        TweCmdPipe(const tself &) = delete;
        void operator = (const tself &ref) = delete;

        // move
        TweCmdPipe(tself &&ref) : MAX_LINE_CHARS(ref.MAX_LINE_CHARS) {
            _close();
            _fp = ref._fp;
            _exit_code = ref._exit_code;
        }
        void operator =(tself &&ref) {
            _close();
            _fp = ref._fp;
            ref._fp = nullptr;
            _exit_code = ref._exit_code;
        };
        
    public:
        TweCmdPipe(const char* cmd, const int max_line_chars = 4095); // open the command with mode="rt"

        TweCmdPipe(const int max_line_chars = 4095)
            : MAX_LINE_CHARS(max_line_chars) 
            , _fp(nullptr)
            , _exit_code(-1)
        {}

        ~TweCmdPipe();
        operator bool() { return available(); } // check if it's opened or not.
        bool available(); // check if it reaches EOF, when reaching EOF, the pipe is closed and set exit code.
        bool readline(TWEUTILS::SmplBuf_Byte& buf); // read line. if having bytes, returns true, otherwise false. (NOTE: false does not mean EOF)
        int exit_code() { return _exit_code; } // get exit code
        void close() { _close(); }
    };
#endif

	class TweDir {
	public:
		static const uint32_t OPT_LIST_DIRS = 1;

	private:
		bool _b_opened;
#ifdef ESP32
		File _root;
		File _file;

	public:
		TweDir() : _root{}, _file{}, _b_opened(false) {}
#else
		// Note: only work C++17 or above.
		std::filesystem::directory_iterator _dir_root, _dir_p, _dir_e;
	public:
		TweDir() : _dir_root(), _dir_p(), _dir_e(), _b_opened(false)  {}
#endif

	public:
		bool open(const wchar_t* p_dirname);
		bool get_next(TWEUTILS::SmplBuf_WChar& name_system, TWEUTILS::SmplBuf_WChar& name_disp, uint32_t opt = 0);
		inline bool is_opened() { return _b_opened;  }
        TWEUTILS::SimpleBuffer<std::pair<TWEUTILS::SmplBuf_WChar, TWEUTILS::SmplBuf_WChar>> find_files(const wchar_t* p_dirname, const wchar_t *p_suff = nullptr);

		static bool is_dir(const wchar_t* p_dirname) {
			if (p_dirname && p_dirname[0] != 0) {
#ifndef ESP32
				return std::filesystem::is_directory(p_dirname);
#endif
			}
			return false;
		}

        static bool create_dir(const wchar_t* p_dirname) {
            if (!is_dir(p_dirname)) {
                std::filesystem::create_directory(p_dirname);
            }
            return is_dir(p_dirname);
        }
	};

    class TweFile {
        static const uint32_t FILE_SIZE_MAX = 512*1024UL;
        const uint32_t CHUNK_SIZE;

        uint32_t _size;
        uint32_t _pos_chunk;
        uint32_t _pos;
        // TWEUTILS::SmplBuf_WChar _name;
        TWEUTILS::SmplBuf_Byte _data;

        uint8_t _b_loaded;

#ifdef ESP32
        File _f;
        TWEUTILS::SmplBuf_Byte _fname;
#else
        std::ifstream _ifs;
#endif

    public:
        TweFile(uint32_t u16_chunk_siz = 4096)
            : CHUNK_SIZE(u16_chunk_siz)
            , _size(0)
            , _pos(0)
            , _pos_chunk(0xFFFFFFFF)
            // , _name(256)
            , _data(u16_chunk_siz)
            , _b_loaded(false)
#ifdef ESP32
            , _f{}
            , _fname(TWE_FILE_NAME_MAX)
#else
            , _ifs()
#endif
        {
            // set size to MAX.
            _data.resize(CHUNK_SIZE);
        }

        ~TweFile() {
            close();
        }

        bool open(TWEUTILS::SmplBuf_WChar& name);

        void close();

        void _init_vars() {
            _size = 0;
            _pos = 0;
            _pos_chunk = 0xFFFFFFFF;
        }

        // n: chunk number
        bool read_chunk(uint16_t n);

        inline int read() {
            if (_b_loaded) {
                if (_pos >= _size) {
                    // reaches EOF
                    return -1;
                } else
                if(!(_pos >= _pos_chunk && _pos < _pos_chunk + CHUNK_SIZE)) {
                    // if not in the range
                    if(!read_chunk(uint16_t(_pos / CHUNK_SIZE))) {
                        return -1;
                    }   
                }

                // returns
                int val = _data[_pos - _pos_chunk];
                _pos++;
                return val;

            } return -1;
        }

        int read(uint8_t *p, uint32_t siz);

        inline bool seek(uint32_t pos) {
            if (_b_loaded && pos < _size) {
                uint16_t chunk = pos / CHUNK_SIZE;

                if (read_chunk(chunk)) {
                    _pos = pos;
                    return true;
                } else return false;

            } else return false;
        }

        inline uint32_t size() {
            return _size;
        }

		inline bool is_opened() {
			return _b_loaded;
		}
    };

#ifndef ESP32
    /**
     * @class	TweFileDropped
     *
     * @brief	Managing drag & dropped file information.
     */
    class TweFileDropped {
        TWEUTILS::SmplBuf_WChar _dir;
        TWEUTILS::SmplBuf_WChar _file;
        bool _b_newly_dropped;

    public:
        TweFileDropped()
            : _dir()
            , _file()
            , _b_newly_dropped(false)
        {}


        /**
         * @fn	bool TweFileDropped::new_drop(const char* file);
         *
         * @brief	To store dropped file/dir information.
         * 			This is called from SDL2 event loop.
         *
         * @param	file	The file name in UTF-8 encoding.
         *
         * @returns	True if it succeeds, false if it fails.
         */
        bool new_drop(const char* file);


        /**
         * @fn	bool TweFileDropped::available()
         *
         * @brief	When dropped, it turns true.
         * 			After this call, reset to false until new file drop.
         *
         * @returns	True if it succeeds, false if it fails.
         */
        bool available() {
            if (_b_newly_dropped) {
                _b_newly_dropped = false;
                return true;
            }
            else {
                return false;
            }
        }
        TWEUTILS::SmplBuf_WChar& get_dir() { return _dir; }
        TWEUTILS::SmplBuf_WChar& get_file() { return _file; }
    };

    /** @brief	The unique instance of TweFileDropped. */
    extern TweFileDropped the_file_drop;


	/**
	 * @class	TweCwd
	 *
	 * @brief	Managing working directory.
	 */
	class TweCwd {
		TWEUTILS::SmplBuf_WChar _dir_launch;
		TWEUTILS::SmplBuf_WChar _dir_exe;
        TWEUTILS::SmplBuf_WChar _filename_exe;
		TWEUTILS::SmplBuf_WChar _dir_cur;
		TWEUTILS::SmplBuf_WChar _dir_sdk;

        TWEUTILS::SmplBuf_WChar _dir_tweapps;
        TWEUTILS::SmplBuf_WChar _dir_wks_acts;
        TWEUTILS::SmplBuf_WChar _dir_wks_tweapps;

		TWEUTILS::SmplBuf_ByteS _save_profile_name;

		void _get_exe_dir();
		void _get_cur_dir();
		void _get_sdk_dir();
		void _set_sdk_env();
        void _get_wks_dir();
	public:
		void begin();
		void change_dir(TWEUTILS::SmplBuf_WChar& dir);
		TWEUTILS::SmplBuf_WChar& get_dir_exe() { return _dir_exe; }
		TWEUTILS::SmplBuf_WChar& get_filename_exe() { return _filename_exe; }
		TWEUTILS::SmplBuf_WChar& get_dir_sdk() { return _dir_sdk; }
		TWEUTILS::SmplBuf_WChar& get_dir_cur() { return _dir_cur; }
		TWEUTILS::SmplBuf_WChar& get_dir_tweapps() { return _dir_tweapps; }
		TWEUTILS::SmplBuf_WChar& get_dir_wks_acts() { return _dir_wks_acts; }
		TWEUTILS::SmplBuf_WChar& get_dir_wks_tweapps() { return _dir_wks_tweapps; }
		TWEUTILS::SmplBuf_WChar& get_dir_launch() { return _dir_launch; }
	};

	extern TweCwd the_cwd;
#endif
    extern const wchar_t* get_dir_tweapps();
}