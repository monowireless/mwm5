/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_file.hpp"
#include "twe_printf.hpp"
#include "twe_utils_unicode.hpp"

using namespace TWE;
using namespace TWEUTILS;

#ifndef ESP32
namespace fs = std::filesystem;
#include <cstdlib>

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <windows.h>
#elif defined(__APPLE__)
#include <cstdio>
#include <unistd.h>
#include <mach-o/dyld.h>
#include <limits.h>
#elif defined(__linux)
#include <cstdio>
#include <unistd.h>
#include <limits.h>
#endif

#include <regex>
#endif

using namespace TWE;
using namespace TWEUTILS;

// defs
#define STR_FIRM_BIN L"BIN"
#define STR_LOG L"log"
#define STR_DOCS L"docs"
#define STR_MWSDK_CHIPLIB L"ChipLib"
#define STR_ACTSAMPLES L"Act_samples"
#define STR_ACTEXTRAS L"Act_extras"
#define STR_MWSDK_TWENET L"TWENET"
#define STR_MWSDK L"MWSDK"

#define STR_WKS_ACTS L"Wks_Acts"
#define STR_WKS_TWEAPPS L"Wks_TweApps"
#define STR_APP_LOC_DIR L"TWELITE_Stage"
//#define STR_MANIFEST L"000manifest"

#ifndef ESP32
const wchar_t TWE::WSTR_LANG_NAMES[E_TWE_LANG::_COUNT_][16] = {
	L"[JAPANESE]",
	L"[ENGLISH]",
};
#endif

// function name alias
#ifndef ESP32
#if defined(_MSC_VER) || defined(__MINGW32__)
#define PROC_POPEN _popen
#define PROC_PCLOSE _pclose
#define PROC_POPEN_MODE "rt"
#else
#define PROC_POPEN popen
#define PROC_PCLOSE pclose
#define PROC_POPEN_MODE "r"
#endif

TweCmdPipe::TweCmdPipe(const char* cmd, const int max_line_chars) 
		: _fp(nullptr), _exit_code(-1), MAX_LINE_CHARS(max_line_chars)
{
	_fp = PROC_POPEN(cmd, PROC_POPEN_MODE);
}

TweCmdPipe::~TweCmdPipe() {
	_close();
}

void TweCmdPipe::_close() {
	if (_fp) {
		_exit_code = PROC_PCLOSE((FILE*)_fp);
	}
	_fp = nullptr;	
}

bool TweCmdPipe::available() {
	if (_fp != nullptr) {
		if (feof(_fp) != 0) {
			//reaches EOF
			_close();
		} else {
			return true;
		}
	}
	return false;
}

bool TweCmdPipe::readline(TWEUTILS::SmplBuf_Byte& buf) {
	if (_fp) {
		buf.reserve_and_set_empty(MAX_LINE_CHARS); // reservs MAX_LINE_CHARS+1 internally.
		char *p = fgets((char*)buf.data(), MAX_LINE_CHARS, (FILE*)_fp);

		if (p == nullptr) {
			return false;
		} else {
			buf.data()[MAX_LINE_CHARS] = 0; // set NUL char at the buffer end.
			buf.resize_preserving_unused((uint32_t)strlen(p));
			return true;
		}
	} else {
		return true;
	}
}
#endif

bool TweFile::open(TWEUTILS::SmplBuf_WChar& name) {
    if (_b_loaded) close();
    _init_vars();

#if defined(ESP32)
    // preare const char* filename.
	_fname.resize(0);
	for (auto x : name) { _fname.push_back(uint8_t(x)); }

    // open the file (open twice)
    _f = SD.open((const char*)_fname.c_str(), FILE_READ); // fails at the first time (why?)
    if (_f) _f.close();
    _f = SD.open((const char*)_fname.c_str(), FILE_READ);

    if (_f) {
        // success!
        int n = _f.size();
        if(n > 0 && n <= FILE_SIZE_MAX) {
            _size = n;
			_b_loaded = true;
        } else {
            close();
        }
    }
#else
    try {
#if defined(_MSC_VER) || defined(__APPLE__) || defined(__MINGW32__)
        SmplBuf_WCharL<TWE::TWE_FILE_NAME_MAX> l_fname;
        
        auto& fname = l_fname.get();
        fname << name;

        #define GET_FNAME_DATA() (fname.c_str())
#elif defined(__linux)
        // linux does not accept wchar_t* in case when filename is double bytes.
        SmplBuf_ByteL<TWE::TWE_FILE_NAME_MAX> l_fname;
        auto& fname = l_fname.get();

        std::copy(name.begin(), name.end(), std::back_inserter(fname));
        
        #define GET_FNAME_DATA() ((const char*)fname.c_str())
#endif

        fs::file_status status = fs::status(GET_FNAME_DATA());

        if (status.type() == fs::file_type::regular) {
            uintmax_t n = fs::file_size(GET_FNAME_DATA());
            if (n > 0 && n <= FILE_SIZE_MAX) {
                _size = (uint32_t)n;
         	    _ifs.open(std::filesystem::path(GET_FNAME_DATA()), std::ios::binary);
                
                _b_loaded = true;
            } else {
                close();
            }
        } else {

        }
    } catch(...) {
        
    }
#endif

    if (_b_loaded) {
        read_chunk(0xFFFF); // read the first chunk
    }

	return _b_loaded;
}

bool TweFile::read_chunk(uint16_t n) {
    if (_b_loaded) {
        uint32_t pos_new = n * CHUNK_SIZE;

		if (n == 0xFFFF) {
			n = 0; // force reload #1
		}
		else {
			if (n * CHUNK_SIZE > _size) { // range over
				_data.resize(0);
				return false;
			}

			// same chunk
			if (_pos_chunk == pos_new)
				return true;
		}

        // clean the buffer
        _data.resize(0);
        _data.resize(CHUNK_SIZE);

        // read from the file
        _pos_chunk = n * CHUNK_SIZE;

#ifdef ESP32
		// not sure why need to open again...
		if (_f) _f.close();
		_f = SD.open((const char*)_fname.c_str(), FILE_READ);
		if (_f) _f.close();
		_f = SD.open((const char*)_fname.c_str(), FILE_READ);

		_f.seek(_pos_chunk);
		_f.read(_data.data(), CHUNK_SIZE);
#else
		// _ifs.seekg(0, _ifs.beg);
		_ifs.clear();
        _ifs.seekg(_pos_chunk, _ifs.beg);
        _ifs.read((char*)_data.data(), CHUNK_SIZE);
#endif
		return true;
    } else return false;
}

void TweFile::close() {
    if (_b_loaded) {
        _b_loaded = false;

        // file close
#ifdef ESP32
		if (_f) _f.close();
#else
        _ifs.close();
#endif

        // clean up vars
        _init_vars();
    }
}

int TweFile::read(uint8_t *p, uint32_t siz) {
    uint32_t ct = 0;

	int c;
	while (ct < siz) {
		c = read();
		if (c == -1) break;

		*p = (uint8_t)(c);

		++p;
		++ct;
	}

    return ct;
}


#ifdef ESP32
bool TweDir::open(const wchar_t *p_dirname_w) {
	_b_opened = false;

	// convert wchar_t to uint8_t
	SmplBuf_Byte u8dirname(TWE::TWE_FILE_NAME_TYPLEN);

	const wchar_t* p_w = p_dirname_w;
	uint8_t* q = u8dirname.data();
	while (*p_w != 0) {
		*q = uint8_t(*p_w & 0xFF);
		q++;
		p_w++;
	}
	*q = 0;
//Serial.printf("\r\ndirname=%s", u8dirname.data());
	
	const char *p_dirname = (const char*)u8dirname.data();
	if (p_dirname != nullptr) {
		// open twice (just for ESP32's unpredictable behavior)
		_root = SD.open(p_dirname); // like /sd
		if (_root) _root.close();
		_root = SD.open(p_dirname);

//Serial.printf("\r\ndir open=%d", _root ? 1 : 0);
		bool bOK = true;
		if (!_root) {
			bOK = false;
		}
		if (!_root.isDirectory()) {
			bOK = false;
		}

		if (bOK) {
			_file = _root.openNextFile();
			if (!_file) {
				bOK = false;
				_root.close();
			}
		}

		_b_opened = bOK;
	}

	return _b_opened;
}
#else
bool TweDir::open(const wchar_t *p_dirname) {
	_b_opened = false;

	if (is_dir(p_dirname)) {
		_dir_root = std::filesystem::directory_iterator(p_dirname);
		_dir_p = begin(_dir_root);
		_dir_e = end(_dir_root);

		_b_opened = true;
	}

	return _b_opened;
}
#endif

#ifdef ESP32
bool TweDir::get_next(TWEUTILS::SmplBuf_WChar& name_system, TWEUTILS::SmplBuf_WChar& name_disp, uint32_t opt) {
	if (_file) {
		const uint8_t* p = (const uint8_t*)_file.name();

		name_system.resize(0);
		while (*p != 0) {
			name_system.push_back(wchar_t(*p));
			++p;
		}
		
		name_disp.resize(0);
		TWEUTILS::appendSjis(name_disp, _file.name());
		// name_disp << name_system;

		_file.close();
		_file = _root.openNextFile();
		
		return true;
	}

	return false;
}
#else
bool TweDir::get_next(TWEUTILS::SmplBuf_WChar& name_system, TWEUTILS::SmplBuf_WChar& name_disp, uint32_t opt) {
	if (!_b_opened) return false;

	while(1) {
		if (_dir_p == _dir_e) return false;

		bool b_found = false;

		if ((opt & OPT_LIST_DIRS) ? _dir_p->is_directory() : _dir_p->is_regular_file()) {
			b_found = true;

			name_system.resize(0);
			name_disp.resize(0);

#if defined(_MSC_VER) || defined(__MINGW32__) || (defined(__APPLE__) && defined(__clang__))
			// note: filename is handled by wchar_t
			name_system << _dir_p->path().filename().generic_wstring().c_str();
			std::copy(name_system.begin(), name_system.end(), std::back_inserter(name_disp));

			#ifdef __APPLE__ // osx, needs to convert combined chars (DAKUTEN/HANDAKUTEN) to display.
			unicodeMacNormalize(name_disp); // mac's DAKUTEN Problem
			#endif
#elif defined(__linux) || (defined(__APPLE__) && !defined(__clang__))
			// note: osx/g++-9 does not have proper wchar_t string from .generic_wstring()
			// note: Linux(Ubuntu18.04) may have exception when double-byte char appears. (needs proper locale setting?)
		
			// UTF char_t*
			SmplBuf_ByteL<TWE_FILE_NAME_MAX> l_nameu;
			SmplBuf_Byte& nameu = l_nameu.get();

			// nameu << _dir_p->path().filename().generic_u8string().c_str();
			auto ustr = _dir_p->path().filename().generic_u8string();
			for(auto x : ustr) { nameu.push_back(x); }

			// just copy (no decoding) nameu into name.
			SmplBuf_WCharL<TWE_FILE_NAME_MAX> l_name;
			SmplBuf_WChar& name = l_name.get();

			std::copy(nameu.begin(), nameu.end(), std::back_inserter(name_system));

			// decoded name
			name_disp << (const char_t*)nameu.c_str(); // will do UTF-8 to Unicode conversion.
			#ifdef __APPLE__
			unicodeMacNormalize(name_disp); // mac's Dakuten Problem
			#endif
#endif
		}
		++_dir_p;

		if (b_found) return true;
	}
}
#endif

SimpleBuffer<std::pair<SmplBuf_WChar, SmplBuf_WChar>> TweDir::find_files(
	 const wchar_t* p_dirname, const wchar_t *p_suff) {
	 SimpleBuffer<std::pair<SmplBuf_WChar, SmplBuf_WChar>> files;

	if (!open(p_dirname)) {
		return std::move(files);
	}

	// suffix length
	int sufflen = 0;
	if (p_suff != nullptr) {
		sufflen = (int)wcslen(p_suff);
	}

#ifdef ESP32
	int len_dirname = wcslen(TWE::get_dir_tweapps()) + 1;
#endif

	// list files
	while (1) {		
		SmplBuf_WChar filename_sys(TWE::TWE_FILE_NAME_TYPLEN);
		SmplBuf_WChar filename_disp(TWE::TWE_FILE_NAME_TYPLEN);

		bool b = get_next(filename_sys, filename_disp);
		
		// end of dir entry
		if (!b) break;

#ifdef ESP32
		// cut dirname. /TWEAPPS/XXXX
		filename_sys.self_attach_headcut(len_dirname);
		filename_disp.self_attach_headcut(len_dirname);
#endif

		// check the file name and add into the list.
		if (sufflen == 0 || (sufflen > 0 && endsWith_NoCase(filename_disp, p_suff, sufflen))) {
			auto a = std::make_pair(std::move(filename_sys), std::move(filename_disp));
			files.push_back(std::move(a));
		}
	}

	return std::move(files);
}

#ifndef ESP32
bool TweFileDropped::new_drop(const char* fullpath) {
	SmplBuf_WCharL<TWE_FILE_NAME_MAX> l_fullpath;

	// convert it into wchar_t
	l_fullpath.get() << fullpath;

	if (std::filesystem::is_directory(l_fullpath.c_str())) {
		if (is_pathsep(l_fullpath[-1])) {
			l_fullpath.get().resize(l_fullpath.size() - 1); // trim 1 char.
		}

		// dropped a DIR
		_dir.reserve_and_set_empty(l_fullpath.size());
		_file.reserve_and_set_empty(0);
		
		std::copy(l_fullpath.begin(), l_fullpath.end(), std::back_inserter(_dir));
	} else {
		auto it_bkw = std::find_if(
			l_fullpath.rbegin()
			, l_fullpath.rend()
			, [](auto x) { return is_pathsep(x); });

		if (it_bkw != l_fullpath.rend()) {
			auto it_fwd = it_bkw.get_forward();

			int len_file = it_bkw - l_fullpath.rbegin();
			int len_dir = l_fullpath.rend() - it_bkw - 1; // without the last PATHSEP

			_dir.reserve_and_set_empty(len_dir);
			_file.reserve_and_set_empty(len_file);

			// now it_fwd points the last PATHSEP, split dir and file part.
			std::copy(l_fullpath.begin(), it_fwd, std::back_inserter(_dir));
			std::copy(++it_fwd, l_fullpath.end(), std::back_inserter(_file));
			int i = 0;
		}
		else {
			// ONLY FILENAME, could be an error (maybe, it ALWAYS has path separators)
			_dir.reserve_and_set_empty(0);
			_file.reserve_and_set_empty(l_fullpath.size());

			std::copy(l_fullpath.begin(), l_fullpath.end(), std::back_inserter(_file));
		}
	}

	_b_newly_dropped = true;

	return true;
}

TweFileDropped TWE::the_file_drop; // the instance
#endif

#ifndef ESP32 // IMPLEMENTATION OF TweCwd
extern "C" const char* twesettings_save_filepath;
static const char STR_MWSDK_ROOT[] = "MWSDK_ROOT";
static const char STR_MWSDK_TWENET_LIBSRC[] = "MWSDK_TWENET_LIBSRC";
static const char STR_MWSDK_MAKE_JOBS[] = "MWSDK_MAKE_JOBS";
static const char STR_MWSDK_MAKE_DISABLE_LTO[] = "MWSDK_MAKE_DISABLE_LTO";

#define PRINT_VAR(c) _print_var(#c, c)

void TweCwd::begin() {
	// get launch dirs
	_get_cur_dir();
	_dir_cur.c_str();
	
	_dir_launch = _dir_cur;
	_dir_launch.c_str(); // not mandate (set NUL term, having better view on debugger.)

	// store exe location
	_get_exe_dir();

	_dir_exe.c_str();
	_filename_exe.c_str();

	// settings file location (at the exe location)
	if (_dir_exe.size() > 0 && _filename_exe.size() > 0) {
		_save_profile_name.reserve_and_set_empty(_dir_exe.size() + 1 + _filename_exe.size() + 4 + 64); // just in case if UTF-8 encoded.
		_save_profile_name << make_full_path(_dir_exe, make_file_ext(_filename_exe, L"sav"));
		
		twesettings_save_filepath = (const char*)_save_profile_name.c_str();
	}

	// SDK dir
	_get_sdk_dir();
	_get_sdk_twenet_lib();
	_set_sdk_env();
	set_mwsdk_env(0, false);

	// TWEAPP/Wks_Acts dir
	_get_wks_dir();

	// show envs in console
	PRINT_VAR(_dir_sdk);
	PRINT_VAR(_save_profile_name);
	PRINT_VAR(_dir_cur);
	PRINT_VAR(_dir_exe);
	PRINT_VAR(_filename_exe);
	PRINT_VAR(_dir_launch);
	PRINT_VAR(_dir_tweapps);
	PRINT_VAR(_dir_twenet_lib);
	PRINT_VAR(_dir_wks_acts);
	PRINT_VAR(_dir_wks_act_extras);
	PRINT_VAR(_dir_wks_tweapps);
	PRINT_VAR(_dir_log);
	PRINT_VAR(_dir_docs);

	#ifdef _DEBUG
	# if defined(_MSC_VER) || defined(__MINGW32__)
		// system("CMD /C SET |FIND \"MWSDK\""); // check env values output to console
		system("CMD /c SET |%SystemRoot%\\System32\\find.exe \"MWSDK_\""); // check env values output to console
	# else 
		system("env |grep MWSDK_"); // check env values output to console
	# endif
	#endif
}

void TweCwd::change_dir(SmplBuf_WChar& dir) {
#if defined(_MSC_VER) || defined(__MINGW32__)
	SetCurrentDirectoryW(dir.c_str()); // Set Current directory.
#else
	SmplBuf_ByteS dir_b(TWE::TWE_FILE_NAME_MAX); // ucs->utf8 conv through IStreamOut.
	dir_b << dir;
	int ret = chdir((const char*)dir_b.c_str()); (void)ret;
#endif

}

void TweCwd::_get_sdk_dir() {
	_dir_sdk.clear();

	// get SDK name from {exe dir}/{TWESTAGE_EXE}.ini 
	SmplBuf_ByteSL<1024> sdk_name;
	TweConf::read_conf("MWSDK", sdk_name);
	/*
	try {
		auto reg_twenet_dir = std::regex(R"(^MWSDK[ \t]*=[ \t]*([a-zA-Z0-9_\-]+))");

		SmplBuf_ByteSL<1024> fname_usever;
		fname_usever << make_full_path(_dir_exe, make_file_ext(_filename_exe, L"ini"));
		
		std::ifstream ifs(fname_usever.c_str());
		std::string buff;

		while (getline(ifs, buff)) {
			// chop it.
			remove_endl(buff);

			// match object
			std::smatch m_pat;

			// parse lines
			if (std::regex_search(buff, m_pat, reg_twenet_dir)) {
				if (m_pat.size() >= 2) {
					sdk_name << m_pat[1].str().c_str();
					break;
				}
			}
		}
	} catch (...) {}
	*/

	// if not found.
	if (sdk_name.size() == 0) sdk_name << STR_MWSDK;
	
	// 0. {cur dir} has MWSDK dir
	if (_dir_sdk.empty() && TweDir::is_dir(make_full_path(get_dir_cur(), sdk_name.c_str(), STR_MWSDK_TWENET).c_str())) {
		if (TweDir::is_dir(make_full_path(get_dir_cur(), sdk_name.c_str(), STR_MWSDK_CHIPLIB).c_str())) {
			_dir_sdk = make_full_path(get_dir_cur(), sdk_name.c_str());
			return;
		}
	}

	// 1. {exe dir} has MWSDK dir
	if (_dir_sdk.empty() && TweDir::is_dir(make_full_path(get_dir_exe(), sdk_name.c_str(), STR_MWSDK_TWENET).c_str())) {
		if (TweDir::is_dir(make_full_path(get_dir_exe(), sdk_name.c_str(), STR_MWSDK_CHIPLIB).c_str())) {
			_dir_sdk = make_full_path(get_dir_exe(), sdk_name.c_str());
			return;
		}
	}

	// 2.1 {exe dir} IS MWSDK dir
	if (_dir_sdk.empty()) {
		if (TweDir::is_dir(make_full_path(get_dir_exe(), STR_MWSDK_CHIPLIB).c_str())) {
			_dir_sdk = make_full_path(get_dir_exe());
			return;
		}
	}

	// 2.2 {exe dir}/.. IS MWSDK dir
	if (_dir_sdk.empty()) {
		if (TweDir::is_dir(make_full_path(get_dir_exe(), L"..", STR_MWSDK_CHIPLIB).c_str())) {
			_dir_sdk = make_full_path(get_dir_exe(), L"..");
			return;
		}
	}

	// 3. {env:MW_SDK_ROOT}
#if defined(_MSC_VER) // || defined(__MINGW32__)
	if (_dir_sdk.empty()) {
		char buff[TWE::TWE_FILE_NAME_MAX];
		size_t reqsiz = 0;

		if (!getenv_s(&reqsiz, buff, TWE::TWE_FILE_NAME_MAX, STR_MWSDK_ROOT)) {
			if (reqsiz > 0) {
				_dir_sdk << buff;
			}
		}
	}
#else
	if (_dir_sdk.empty()) {
		if (auto&& p = ptr_wrapper(std::getenv(STR_MWSDK_ROOT))) {
			_dir_sdk << (const char*)p.ref();
		}
	}
#endif

	// check if _dir_sdk exists and MWSDK dir.
	if (!_dir_sdk.empty()) {
#if defined(_MSC_VER) || defined(__MINGW32__)
		for (auto& x : _dir_sdk) {
			if (is_pathsep(x)) x = WCHR_PATH_SEP;
		}
#endif
		if (_dir_sdk[-1] == WCHR_PATH_SEP) {
			_dir_sdk.pop_back();
		}

		bool bCheck = false;
		if (TweDir::is_dir(make_full_path(_dir_sdk, STR_MWSDK_TWENET).c_str())) {
			if (TweDir::is_dir(make_full_path(_dir_sdk, STR_MWSDK_CHIPLIB).c_str())) {
				bCheck = true;
			}
		}
		if (!bCheck) _dir_sdk.clear(); // not the MWSDK
	}
}

/**
 * @fn	void TweCwd::_get_sdk_twenet_lib()
 *
 * @brief	Gets TWENET library source directory 
 * 			   {MWSDK_ROOT}/TWENET/current or others.
 * 			   
 */
void TweCwd::_get_sdk_twenet_lib() {
	SmplBuf_ByteSL<1024> dir_lib;

	auto reg_twenet_dir = std::regex(R"(^TWENET_DIR[ \t]*=[ \t]*([a-zA-Z0-9_\-]+))");
	bool b_find_ver = false;

	// open TWENET/usever.mk
	try {
		SmplBuf_ByteSL<1024> fname_usever;
		fname_usever << make_full_path(the_cwd.get_dir_sdk(), L"TWENET", L"usever.mk");

		std::ifstream ifs(fname_usever.c_str());
		std::string buff;

		while (getline(ifs, buff)) {
			// chop it.
			remove_endl(buff);

			// match object
			std::smatch m_pat;

			// parse lines
			if (std::regex_search(buff, m_pat, reg_twenet_dir)) {
				if (m_pat.size() >= 2) {
					_dir_twenet_lib = make_full_path(the_cwd.get_dir_sdk(), L"TWENET", m_pat[1].str().c_str());
					if (TweDir::is_dir(_dir_twenet_lib.c_str())) {
						break;
					}
					else {
						_dir_twenet_lib.clear();
					}
				}
			}
		}
	}
	catch (...) {}

	if (_dir_twenet_lib.size() == 0) {
		_dir_twenet_lib << make_full_path(the_cwd.get_dir_sdk(), L"TWENET", L"current"); // force default!
	}
}

/**
 * @fn	void TweCwd::_set_sdk_env()
 *
 * @brief	Sets sdk environment for building or other apps launching from TWELITE STAGE APP.
 * 			  MWSDK_ROOT: points SDK library directory mandate for `make'
 * 			  MWSDK_TWENET_LIBSRC: points TWENET library source directory switching by TWENET/usever.mk.
 * 			  LANG: set "C" assuring all message outputs in English.
 */
void TweCwd::_set_sdk_env() {
	SmplBuf_ByteS val;
	val.reserve(TWE::TWE_FILE_NAME_MAX * 3);

#if defined(_MSC_VER) || defined(__MINGW32__)
	/// MWSDK_ROOT=...
	val.clear(); val << _dir_sdk;
	for (auto& x : val) { if (x == '\\') x = '/';  }	// replace \ to /
	val.push_back('/'); // last should be /
	_putenv_s(STR_MWSDK_ROOT, (const char*)val.c_str());
	_print_var(STR_MWSDK_ROOT, val);

	/// MWSDK_TWENET_LIBSRC=...
	val.clear(); val << _dir_twenet_lib;
	for (auto& x : val) { if (x == '\\') x = '/'; }
	val.push_back('/'); // last should be /
	_putenv_s(STR_MWSDK_TWENET_LIBSRC, (const char*)val.c_str());
	_print_var(STR_MWSDK_TWENET_LIBSRC, val);

	/// LANG 
	val.clear(); val << "C";
	_putenv_s("LANG", val.c_str());
	_print_var("LANG", val);
	
	/// PATH (add path of cygwin)
	val.clear();
	size_t reqct;
	getenv_s(&reqct, NULL, 0, "PATH"); // check size of env
	if (reqct > (val.capacity() - TWE::TWE_FILE_NAME_MAX)) {
		val.reserve(SmplBuf_ByteS::size_type(TWE::TWE_FILE_NAME_MAX + reqct)); // if excess size, reserve more.
	}
	
	val << make_full_path(_dir_sdk, "..\\Tools\\MinGW\\msys\\1.0\\bin");
	if (TweDir::is_dir(val.c_str())) {
		_print_var("PATH(added)", val);
	}
	else {
		val.clear();
		val << make_full_path(_dir_sdk, "\\Tools\\MinGW\\msys\\1.0\\bin");

		if (!TweDir::is_dir(val.c_str())) {
			val.clear();
		}
		else {
			_print_var("PATH(added,oldsdk)", val);
		}
	}
	if (val.size() != 0) {
		val.append(';');

		getenv_s(&reqct, (char*)val.end().raw_ptr(), reqct, "PATH"); // put data dirctly into Buffer.
		val.resize_preserving_unused(SmplBuf_ByteS::size_type(val.size() + reqct)); // expand buffer end without clearing data.

		_putenv_s("PATH", val.c_str());
		_print_var("PATH", val);
	}
	else {
		// cannot found!
		val.emptify() << "Cygwin tools cannot be found!";
		_print_var("ERROR:", val);
	}


#else
	val.clear(); val << _dir_sdk;
	val.push_back('/'); // last should be /
	setenv(STR_MWSDK_ROOT, (const char*)val.c_str(), 1); // MWSDK_ROOT=...
	_print_var(STR_MWSDK_ROOT, val);
	val.clear(); val << _dir_twenet_lib;
	val.push_back('/'); // last should be /
	setenv(STR_MWSDK_TWENET_LIBSRC, (const char*)val.c_str(), 1); // MWSDK_TWENET_LIBSRC=...
	_print_var(STR_MWSDK_TWENET_LIBSRC, val);
	val.clear(); val << "C";
	setenv("LANG", val.c_str(), 1); // LANG=C (to assure english error message.)
	_print_var("LANG", val);
# ifdef _DEBUG
	system("env"); // check env values output to console
# endif
#endif
}


void TweCwd::set_mwsdk_env(uint8_t n_cpu, bool b_lto_disable) {
	SmplBuf_ByteS val;
	val.reserve(TWE::TWE_FILE_NAME_MAX);

	/// MWSDK_BUILD_CPUS
	if (n_cpu == 0) {
		n_cpu = TWESYS::Get_CPU_COUNT();
		if (n_cpu >= 4) n_cpu--;
		if (n_cpu == 0) n_cpu = 1;
	}

	val.clear();
	val << printfmt("-j%d", n_cpu);

#if defined(_MSC_VER) || defined(__MINGW32__)
	_putenv_s(STR_MWSDK_MAKE_JOBS, (const char*)val.c_str());
#else
	setenv(STR_MWSDK_MAKE_JOBS, (const char*)val.c_str(), 1);
#endif
	_print_var(STR_MWSDK_MAKE_JOBS, val);

	// DISABLE LTO
	if (b_lto_disable) {
		val.emptify() << "DISABLE_LTO=1";
#if defined(_MSC_VER) || defined(__MINGW32__)
		_putenv_s(STR_MWSDK_MAKE_DISABLE_LTO, val.c_str());
#else
		setenv(STR_MWSDK_MAKE_DISABLE_LTO, val.c_str(), 1);
#endif
		_print_var(STR_MWSDK_MAKE_DISABLE_LTO, val);
	}
}

void TweCwd::_get_wks_dir() {
	// finding source/bin dir for firm programming.

	// general search order is:
	//  1. current dir when launched
	//  2. exe located dir
	//  3. ../{MWSDK_ROOT}
	//  4. {MWSDK_ROOT}

	// [BINから選択] as "BIN"
	_dir_tweapps = make_full_path(get_dir_launch(), STR_FIRM_BIN);
	if (!TweDir::is_dir(_dir_tweapps.c_str())) _dir_tweapps = make_full_path(get_dir_exe(), STR_FIRM_BIN);
	if (!TweDir::is_dir(_dir_tweapps.c_str())) _dir_tweapps = make_full_path(get_dir_sdk(), L"..", STR_FIRM_BIN);
	if (!TweDir::is_dir(_dir_tweapps.c_str())) _dir_tweapps = make_full_path(get_dir_sdk(), STR_FIRM_BIN);
	_dir_tweapps.c_str();

	// [ログディレクトリ] as "log"
	_dir_log = make_full_path(get_dir_launch(), STR_LOG);
	if (!TweDir::is_dir(_dir_log.c_str())) _dir_log = make_full_path(get_dir_exe(), STR_LOG);
	if (!TweDir::is_dir(_dir_log.c_str())) _dir_log = make_full_path(get_dir_sdk(), L"..", STR_LOG); // should be STAGE DIR
	if (!TweDir::is_dir(_dir_log.c_str())) _dir_log = make_full_path(get_dir_sdk(), STR_LOG);
	_dir_log.c_str();

	// [ドキュメントディレクトリ] as "docs"
	_dir_docs = make_full_path(get_dir_sdk(), STR_DOCS); // should be at MWSDK dir.
	if (!TweDir::is_dir(_dir_docs.c_str())) _dir_docs = make_full_path(get_dir_exe(), STR_DOCS);
	if (!TweDir::is_dir(_dir_docs.c_str())) _dir_docs = make_full_path(get_dir_launch(), STR_DOCS);
	_dir_docs.c_str();

	// [Actビルド＆書換] firstly searches `Wks_Acts'
	_dir_wks_acts = make_full_path(get_dir_launch(), STR_WKS_ACTS);
	if (!TweDir::is_dir(_dir_wks_acts.c_str())) _dir_wks_acts = make_full_path(get_dir_exe(), STR_WKS_ACTS);
	if (!TweDir::is_dir(_dir_wks_acts.c_str())) _dir_wks_acts = make_full_path(get_dir_sdk(), L"..", STR_WKS_ACTS);
	if (!TweDir::is_dir(_dir_wks_acts.c_str())) _dir_wks_acts = make_full_path(get_dir_sdk(), STR_WKS_ACTS);
	// then `Act_samples'
	if (!TweDir::is_dir(_dir_wks_acts.c_str())) _dir_wks_acts = make_full_path(get_dir_launch(), STR_ACTSAMPLES);
	if (!TweDir::is_dir(_dir_wks_acts.c_str())) _dir_wks_acts = make_full_path(get_dir_exe(), STR_ACTSAMPLES);
	if (!TweDir::is_dir(_dir_wks_acts.c_str())) _dir_wks_acts = make_full_path(get_dir_sdk(), L"..", STR_ACTSAMPLES);
	if (!TweDir::is_dir(_dir_wks_acts.c_str())) _dir_wks_acts = make_full_path(get_dir_sdk(), STR_ACTSAMPLES);
	_dir_wks_acts.c_str();

	// [Actエクストラ] as `Act_extras'
	_dir_wks_act_extras = make_full_path(get_dir_launch(), STR_ACTEXTRAS);
	if (!TweDir::is_dir(_dir_wks_act_extras.c_str())) _dir_wks_act_extras = make_full_path(get_dir_exe(), STR_ACTEXTRAS);
	if (!TweDir::is_dir(_dir_wks_act_extras.c_str())) _dir_wks_act_extras = make_full_path(get_dir_sdk(), L"..", STR_ACTEXTRAS);
	if (!TweDir::is_dir(_dir_wks_act_extras.c_str())) _dir_wks_act_extras = make_full_path(get_dir_sdk(), STR_ACTEXTRAS);
	_dir_wks_act_extras.c_str();

	// [TWE Apps] as `Wks_TweApps'
	_dir_wks_tweapps = make_full_path(get_dir_launch(), STR_WKS_TWEAPPS);
	if (!TweDir::is_dir(_dir_wks_tweapps.c_str())) _dir_wks_tweapps = make_full_path(get_dir_exe(), STR_WKS_TWEAPPS);
	if (!TweDir::is_dir(_dir_wks_tweapps.c_str())) _dir_wks_tweapps = make_full_path(get_dir_sdk(), L"..", STR_WKS_TWEAPPS);
	if (!TweDir::is_dir(_dir_wks_tweapps.c_str())) _dir_wks_tweapps = make_full_path(get_dir_sdk(), STR_WKS_TWEAPPS);
	_dir_wks_tweapps.c_str();
}

void TweCwd::_get_cur_dir() {
	SmplBuf_WChar str(TWE::TWE_FILE_NAME_MAX);
	uint32_t bufsize = TWE::TWE_FILE_NAME_MAX;

#if defined(_MSC_VER) || defined(__MINGW32__)
	bufsize = GetCurrentDirectoryW(TWE::TWE_FILE_NAME_MAX, (LPWSTR)str.data());
	if (bufsize > 0) {
		str.resize_preserving_unused(bufsize);
	}
#else // osx, linux
	{
		char buf[TWE::TWE_FILE_NAME_MAX];
		if (getcwd(buf, TWE::TWE_FILE_NAME_MAX) != nullptr) {
			str << buf;
		}
	}
#endif
	_dir_cur = str;
}


void TweCwd::_get_exe_dir() {
	// get the exe running dir.
	SmplBuf_WChar str(TWE::TWE_FILE_NAME_MAX);
	uint32_t bufsize = 0;

#if defined(_MSC_VER) || defined(__MINGW32__)
	bufsize = (uint32_t)GetModuleFileNameW(NULL, (LPWSTR)str.data(), TWE::TWE_FILE_NAME_MAX);
	if (bufsize > 0) {
		str.resize_preserving_unused(bufsize);
	}
#else
	{
		char buf[TWE::TWE_FILE_NAME_MAX];
		buf[0] = 0;
		bufsize = TWE::TWE_FILE_NAME_MAX;
# if defined(__APPLE__)
		if (_NSGetExecutablePath(buf, &bufsize) != 0) {
			buf[0] = 0; // error
		}
# elif defined(__linux)
		ssize_t r1 = readlink("/proc/self/exe", buf, bufsize);
		if (r1 > 0) buf[r1] = 0;
		else buf[0] = 0;
# else
#  error "to be implemented."
# endif
		str << buf;
	}
#endif

	// find the last path separator and trim it.
	auto it = find_last_pathsep(str);
	if (it != str.cend()) {
		_dir_exe.reserve_and_set_empty(128);
		_dir_exe << std::pair(str.cbegin(), it);

		_filename_exe.reserve_and_set_empty(128);
		_filename_exe << std::pair(it + 1, str.cend());

#if defined(_MSC_VER) || defined(__MINGW32__)
		if (endsWith_NoCase(_filename_exe, L".EXE", 4)) {
			_filename_exe.resize(_filename_exe.size() - 4);
		}
#elif defined(__APPLE__)
		if (endsWith_NoCase(_filename_exe, L".command", 8)) {
			_filename_exe.resize(_filename_exe.size() - 8);
		}
#elif defined(__linux__)
		if (endsWith_NoCase(_filename_exe, L".run", 4)) {
			_filename_exe.resize(_filename_exe.size() - 4);
		}
#endif 
	} else {
		// an error
		_dir_exe.resize(0);
		_filename_exe.resize(0);
	}
}
TweCwd TWE::the_cwd;
#endif


const wchar_t* TWE::get_dir_tweapps() {
#ifdef ESP32
	return (L"/" STR_FIRM_BIN);
#else
	return the_cwd.get_dir_tweapps().c_str();
#endif
}

#ifndef ESP32
/**
 * @fn	bool TweDesc::load(const wchar_t* descfile, E_TWE_LANG::value_type lang)
 *
 * @brief	Loads a 000desc.txt file.
 * 			The format of 000desc.txt file is in two ways,
 * 			1) simple format
 * 			   TITLE LINE   / single line
 * 			   DESC LINE(S) / single or multi lines
 * 			   URL LINE     / OPTIONAL, begins with "https://"
 * 			   
 * 			2) complex, w/ sections for LANGUAGE selection.
 * 			   [LANG1]      / section (the first entry must be located at the first line)
 * 			   TITLE=XXXX   / TITLE line, single line
 * 			   DESC=XXXX    / DESC line(s), single/multi lines
 * 			   URL=XXXX     / URL line, optional, single
 * 			   [LANG2]
 * 			   ...
 * 			   
 *          If the first character of a line is ';', the line is skipped to read.
 *
 * @param	descfile	The descfile.
 * @param	lang		The language.
 *
 * @returns	True if it succeeds, false if it fails.
 */

bool TweDesc::load(const wchar_t* descfile, E_TWE_LANG::value_type lang) {
	this->_bloaded = false;

	bool bComplexFormat = false;
	bool bFoundLang = false;

	this->_bloaded = false;
	this->_title.clear();
	this->_desc.clear();
	this->_url.clear();

	if (std::filesystem::is_regular_file(descfile)) {
		SmplBuf_ByteSL<1024> sb_descfile;
		sb_descfile << descfile;
		
		try {
			std::ifstream ifs((const char*)sb_descfile.c_str());
			std::string buff;

			// find the first line (TITLE or [SECTION])
			bool bFoundFirstLine = false;
			while (getline(ifs, buff)) {
				// skipping comment line
				if (buff.size() > 0 && buff[0] == ';') continue;
			
				// remove \r, \n at the end
				remove_endl(buff);

				// remove BOM
				if (buff.size() >= 3 && buff[0] == char(0xEF) && buff[1] == char(0xBB) && buff[2] == char(0xBF)) {
					buff.erase(0, 3);
				}

				// check format of this line
				SmplBuf_WChar l;
				l << buff.c_str(); // with UTF-8 convert.

				if (l.length() > 2 && l[0] == L'[' && l[-1] == L']') {
					bComplexFormat = true;

					if (l == WSTR_LANG_NAMES[lang]) {
						bFoundLang = true;
					}
				}

				// simple format (the first line)
				if (!bComplexFormat) {
					this->_title = l; // set TITLE string
				}
				
				bFoundFirstLine = true;
				break;
			}

			// not found first line, return with an error status
			if (!bFoundFirstLine) return false;

			// find desired language
			if (bComplexFormat && !bFoundLang) {
				while (getline(ifs, buff)) {
					// skipping comment line
					if (buff.size() > 0 && buff[0] == ';') continue;

					// remove \r, \n at the end
					remove_endl(buff);

					// check format of this line
					SmplBuf_WChar l;
					l << buff.c_str(); // with UTF-8 convert.

					// check if `[desired language name]'.
					if (l == WSTR_LANG_NAMES[lang]) {
						bFoundLang = true;
						break;
					}
				}
			}

			// read the followings
			if (!bComplexFormat) {
				int ct = 0;
				while (getline(ifs, buff) && ct < 4) { // four lines max
					// skipping comment line
					if (buff.size() > 0 && buff[0] == ';') continue;

					// remove \r, \n at the end
					remove_endl(buff);

					// check format of this line
					SmplBuf_WChar l;
					l << buff.c_str(); // with UTF-8 convert.

					if (beginsWith_NoCase(l, L"HTTP", 4)) {
						// consider as URL if the line begins with "http".
						this->_url = l;
					} else {
						if (ct > 0) this->_desc << L'\r' << L'\n'; // add line break
						this->_desc << l;
						ct++;
					}
				}

				this->_bloaded = true;
			}
			else if (bComplexFormat && bFoundLang) {
				enum class E_LAST {
					NONE = 0,
					TITLE,
					DESC,
					URL
				} eLastItem;

				eLastItem = E_LAST::NONE;

				while (getline(ifs, buff)) { // four lines max
					// skipping comment line
					if (buff.size() > 0 && buff[0] == ';') continue;

					// remove \r, \n at the end
					remove_endl(buff);

					// check format of this line
					SmplBuf_WChar l;
					l << buff.c_str(); // with UTF-8 convert.
					(void)l.c_str(); // terminate internal buffer

					// found next section "[...]", finish here
					if (l.length() > 2 && l[0] == L'[' && l[-1] == L']') {
						break;
					}

					if (beginsWith_NoCase(l, L"TITLE=", 6)) {
						this->_title << std::pair(l.begin() + 6, l.end());
						eLastItem = E_LAST::TITLE;
					} else 
					if (beginsWith_NoCase(l, L"DESC=", 5)) {
						this->_desc << std::pair(l.begin() + 5, l.end());
						eLastItem = E_LAST::DESC;
					} else 
					if (beginsWith_NoCase(l, L"URL=", 4)) {
						this->_url << std::pair(l.begin() + 4, l.end());
						eLastItem = E_LAST::URL;
					}
					else {
						if (l.length() > 0 && eLastItem == E_LAST::DESC) {
							// append description
							this->_desc << L'\r' << L'\n'; // add line break
							this->_desc << l;
						}
					}
				}

				this->_bloaded = true;
			}
		}
		catch (...) {
			this->_bloaded = false;
		}
	}

	return this->_bloaded;
}

template <typename T, typename S=const wchar_t*, bool HAS_CMD=false>
void _shell_open_default(T obj, S cmd) {
	SmplBuf_ByteSL<1024> strbuff;
	SmplBuf_ByteSL<128> cmdbuff;

	if (HAS_CMD) {
		cmdbuff << cmd;
	} else {
#if defined(__APPLE__)
		cmdbuff << "open";
#elif defined(__linux)
		cmdbuff << "xdg-open";
#endif		
	}

#if defined(_MSC_VER) || defined(__MINGW32__)
	if (HAS_CMD) {
		// use simply system command.
		strbuff << cmdbuff << ' ' << obj;
		system(strbuff.c_str());

		// shellexcute will generate another command prompt window.
		// strbuff << obj;
		//ShellExecuteA(NULL, "open", cmdbuff.c_str(), (LPCSTR)strbuff.c_str(), NULL, SW_SHOWMINIMIZED);
	} else {
		strbuff << obj;
		ShellExecuteA(NULL, "open", (LPCSTR)strbuff.c_str(), NULL, NULL, SW_SHOWDEFAULT);
	}
#elif defined(__APPLE__) || defined(__linux)
	strbuff << cmdbuff << ' ' << obj;
	int apiret = system(strbuff.c_str()); (void)apiret;
#endif
}

void TWE::shell_open_default(const wchar_t* wstr, const wchar_t* file) {
	if (file == nullptr && wstr) {
		_shell_open_default<const wchar_t *>(wstr, nullptr);
	}
	else if (file && wstr) {
		_shell_open_default<const wchar_t*>(make_full_path(wstr, file).c_str(), nullptr);
	}
}

void TWE::shell_open_default(const char_t* str, const char_t* file) {
	if (file == nullptr && str) {
		_shell_open_default<const char_t*>(str, nullptr);
	}
	else if (file && str) {
		SmplBuf_ByteSL<TWE_FILE_NAME_MAX> fullpath;
		fullpath << make_full_path(str, file);

		_shell_open_default<const char_t*>(fullpath.c_str(), nullptr);
	}
}

void TWE::shell_open_default(TWEUTILS::SmplBuf_WChar& str) {
	if (str.length() > 0) {
		TWE::shell_open_url(str.c_str());
	}
}

void TWE::shell_open_default(TWEUTILS::SmplBuf_WChar&& str) {
	if (str.length() > 0) {
		TWE::shell_open_url(str.c_str());
	}
}

void TWE::shell_open_by_command(const wchar_t* wstr_obj, const wchar_t* wstr_cmd) {
	if (wstr_obj && wstr_cmd) {
		_shell_open_default<const wchar_t*, const wchar_t *, true>(wstr_obj, wstr_cmd);
	}
}

void TWE::shell_open_by_command(const char_t* str_obj, const char_t* str_cmd) {
	if (str_obj && str_cmd) {
		_shell_open_default<const char_t*, const char_t *, true>(str_obj, str_cmd);
	}
}

void TWE::shell_open_by_command(TWEUTILS::SmplBuf_WChar& str, const wchar_t* str_cmd) {
	if (str.length() > 0 && str_cmd) {
		_shell_open_default<const wchar_t*, const wchar_t *, true>(str.c_str(), str_cmd);
	}
}

void TWE::shell_open_by_command(TWEUTILS::SmplBuf_WChar&& str, const wchar_t* str_cmd) {
	if (str.length() > 0 && str_cmd) {
		_shell_open_default<const wchar_t*, const wchar_t *, true>(str.c_str(), str_cmd);
	}
}

void TWE::shell_open_help(TWEUTILS::SmplBuf_WChar&& wstr) {
	if (beginsWith_NoCase(wstr, L"HTTP:") || beginsWith_NoCase(wstr, L"HTTPS:")) {
		// open URL
		shell_open_default(wstr);
	} else
	if (beginsWith_NoCase(wstr, L"APP_LOC:")) {
		shell_open_default(make_full_path(the_cwd.get_dir_exe().c_str(), STR_APP_LOC_DIR, wstr.c_str() + 8));
	}
	else {
		// open from doc folder
		shell_open_default(the_cwd.get_dir_docs().c_str(), wstr.c_str());
	}
}

bool TWE::TweLogFile::open(bool b_append) {
	_time_open.now();

	_file_name << _file_base;
	if (_b_add_date) {
		_file_name << TWE::format("%04d%02d%02d-%02d%02d%02d"
			, _time_open.year
			, _time_open.month
			, _time_open.day
			, _time_open.hour
			, _time_open.minute
			, _time_open.second
		);
	}
	_file_name << '.' << _file_suff;
	_file_fullpath << make_full_path(the_cwd.get_dir_log(), _file_name);

	try {
		_file_buf.reset(new std::filebuf());
		_file_buf->open((const char*)_file_fullpath.c_str(), std::ios_base::out | std::ios::binary | (b_append ? std::ios::app : std::ios::trunc));
		_file_os.reset(new std::ostream(_file_buf.get()));
		_is_opened = true;
		return true;
	}
	catch (...) {
		_file_buf.reset();
		_file_os.reset();
		_is_opened = false;

		return false;
	}
}
void TWE::TweLogFile::flush() {
	if (_is_opened) {
		_file_os->flush();
	}
}

void TWE::TweLogFile::shell_open() {
	shell_open_default(_file_fullpath.c_str());
}

void TWE::TweLogFile::close() {
	_file_buf.reset();
	_file_os.reset();
	_is_opened = false;
}

bool TweConf::_read_conf_body(const char* key_name, std::function<void(const char* ptr)> store_func) {
	bool b_ret = false;
	try {
		SmplBuf_ByteS regex_pat(1024), fname_usever(1024);
		regex_pat << format(R"(^%s[ \t]*=[ \t]*([a-zA-Z0-9_\-]+))", key_name);
		auto reg_twenet_dir = std::regex(regex_pat.c_str());

		fname_usever << make_full_path(the_cwd.get_dir_exe(), make_file_ext(the_cwd.get_filename_exe(), L"ini"));

		std::ifstream ifs(fname_usever.c_str());
		std::string buff;

		while (getline(ifs, buff)) {
			// chop it.
			remove_endl(buff);

			// match object
			std::smatch m_pat;

			// parse lines
			if (std::regex_search(buff, m_pat, reg_twenet_dir)) {
				if (m_pat.size() >= 2) {
					store_func(m_pat[1].str().c_str());
					b_ret = true;
					break;
				}
			}
		}
	}
	catch (...) {}

	return b_ret;
}

#endif // ESP32
