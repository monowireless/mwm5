/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_file.hpp"
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

#endif

using namespace TWE;
using namespace TWEUTILS;

// defs
#define STR_FIRM_BIN L"BIN"
#define STR_ACTSAMPLES L"Act_samples"
#define STR_ACTEXTRAS L"Act_extras"
#define STR_TWENET L"TWENET"
#define STR_MWSDK L"MWSDK"

#define STR_WKS_ACTS L"Wks_Acts"
#define STR_WKS_TWEAPPS L"Wks_TweApps"
//#define STR_MANIFEST L"000manifest"

#ifndef ESP32
const wchar_t TWE::WSTR_LANG_NAMES[E_TWE_LANG::_COUNT_][16] = {
	L"[ENGLISH]",
	L"[JAPANESE]"
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
        _ifs.seekg(_pos_chunk);
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
	_set_sdk_env();

	_dir_sdk.c_str();

	// TWEAPP/Wks_Acts dir
	_get_wks_dir();
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
	
	// 0. {cur dir} has MWSDK dir
	if (_dir_sdk.empty() && TweDir::is_dir(make_full_path(get_dir_cur(), STR_MWSDK, STR_TWENET).c_str())) {
		if (TweDir::is_dir(make_full_path(get_dir_cur(), STR_MWSDK, STR_ACTSAMPLES).c_str())) {
			_dir_sdk = make_full_path(get_dir_cur(), STR_MWSDK);
			return;
		}
	}

	// 1. {exe dir} has MWSDK dir
	if (_dir_sdk.empty() && TweDir::is_dir(make_full_path(get_dir_exe(), STR_MWSDK, STR_TWENET).c_str())) {
		if (TweDir::is_dir(make_full_path(get_dir_exe(), STR_MWSDK, STR_ACTSAMPLES).c_str())) {
			_dir_sdk = make_full_path(get_dir_exe(), STR_MWSDK);
			return;
		}
	}

	// 2.1 {exe dir} IS MWSDK dir
	if (_dir_sdk.empty()) {
		if (TweDir::is_dir(make_full_path(get_dir_exe(), STR_ACTSAMPLES).c_str())) {
			_dir_sdk = make_full_path(get_dir_exe());
			return;
		}
	}

	// 2.2 {exe dir}/.. IS MWSDK dir
	if (_dir_sdk.empty()) {
		if (TweDir::is_dir(make_full_path(get_dir_exe(), L"..", STR_ACTSAMPLES).c_str())) {
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
		if (TweDir::is_dir(make_full_path(_dir_sdk, STR_TWENET).c_str())) {
			if (TweDir::is_dir(make_full_path(_dir_sdk, STR_ACTSAMPLES).c_str())) {
				bCheck = true;
			}
		}
		if (!bCheck) _dir_sdk.clear(); // not the MWSDK
	}
}


void TweCwd::_set_sdk_env() {
	SmplBuf_ByteSL<TWE::TWE_FILE_NAME_MAX> val;

#if defined(_MSC_VER) || defined(__MINGW32__)
	val << _dir_sdk;

	// replace \ to /
	for (auto& x : val) {
		if (x == '\\') x = '/';
	}
	val.push_back('/'); // last should be /

	// put it!
	_putenv_s(STR_MWSDK_ROOT, (const char*)val.c_str());
	_putenv_s("LANG", "C");
#else
	val << _dir_sdk;
	val.push_back('/'); // last should be /
	setenv(STR_MWSDK_ROOT, (const char*)val.c_str(), 1); // MWSDK_ROOT=...
	setenv("LANG", "C", 1); // LANG=C (to assure english error message.)
# ifdef _DEBUG
	system("env");
# endif
#endif
}

void TweCwd::_get_wks_dir() {
	// TWEAPPS
	_dir_tweapps = make_full_path(get_dir_launch(), STR_FIRM_BIN);
	if (!TweDir::is_dir(_dir_tweapps.c_str())) {
		_dir_tweapps = make_full_path(get_dir_exe(), STR_FIRM_BIN);
		if (!TweDir::is_dir(_dir_tweapps.c_str())) {
			_dir_tweapps = make_full_path(get_dir_sdk(), STR_FIRM_BIN);
		}
	}

	_dir_tweapps.c_str();

	// Wks Acts or Act_samples (0. cur dir, 1. exe dir, 2. sdk dir, 3. Act_samples at sdk dir)
	_dir_wks_acts = make_full_path(get_dir_launch(), STR_WKS_ACTS);
	if (!TweDir::is_dir(_dir_wks_acts.c_str())) {
		_dir_wks_acts = make_full_path(get_dir_exe(), STR_WKS_ACTS);
		if (!TweDir::is_dir(_dir_wks_acts.c_str())) {
			_dir_wks_acts = make_full_path(get_dir_sdk(), STR_WKS_ACTS);
			if (!TweDir::is_dir(_dir_wks_acts.c_str())) {
				_dir_wks_acts = make_full_path(get_dir_sdk(), STR_ACTSAMPLES);
			}
		}
	}
	_dir_wks_acts.c_str();

	// Act_extras (at sdk dir)
	if (!TweDir::is_dir(_dir_wks_act_extras.c_str())) {
		_dir_wks_act_extras = make_full_path(get_dir_sdk(), STR_ACTEXTRAS);
	}
	_dir_wks_act_extras.c_str();

	// TweApps Acts (0. cur dir, 1. exe dir, 2. sdk dir, 3. sdk dir sample)
	_dir_wks_tweapps = make_full_path(get_dir_launch(), STR_WKS_TWEAPPS);
	if (!TweDir::is_dir(_dir_wks_tweapps.c_str())) {
		_dir_wks_tweapps = make_full_path(get_dir_exe(), STR_WKS_TWEAPPS);
		if (!TweDir::is_dir(_dir_wks_tweapps.c_str())) {
			_dir_wks_tweapps = make_full_path(get_dir_sdk(), STR_WKS_TWEAPPS);
		}
	}
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
				if (buff.size() >= 3 && buff[0] == 0xEF && buff[1] == 0xBB && buff[2] == 0xBF) {
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
					(void*)l.c_str(); // terminate internal buffer

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

void TWE::shell_open_url(const wchar_t* wstr) {
	if (wstr) {
		SmplBuf_ByteSL<1024> strbuff;

#if defined(_MSC_VER) || defined(__MINGW32__)
		strbuff << wstr;
		ShellExecuteA(NULL, "open", (LPCSTR)strbuff.c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__)
		strbuff << "open " << wstr;
		system(strbuff.c_str());
#endif
	}
}

void TWE::shell_open_url(TWEUTILS::SmplBuf_WChar& url) {
	if (url.length() > 0) {
		TWE::shell_open_url(url.c_str());
	}
}

void TWE::shell_open_folder(const wchar_t* wstr_name) {
	SmplBuf_ByteSL<1024> lb;
	
#if defined(_MSC_VER) || defined(__MINGW32__)
	lb << wstr_name;
	ShellExecuteA(NULL, "open", (LPCSTR)lb.c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__)
	lb << "open " << wstr_name;
	system((const char*)lb.c_str());
#endif
}

void TWE::shell_open_folder(TWEUTILS::SmplBuf_WChar& name) {
	if (name.length() > 0) {
		TWE::shell_open_folder(name.c_str());
	}
}

void TWE::shell_open_by_command(const wchar_t* wstr_name, const wchar_t* wstr_cmd) {
	SmplBuf_ByteSL<1024> lb;

#if defined(__APPLE__)
	lb << wstr_cmd << ' '; // add command name
#endif

	lb << wstr_name;

	// open project dir.
#if defined(_MSC_VER) || defined(__MINGW32__)
	SmplBuf_ByteSL<1024> cmd;
	cmd << wstr_cmd;

	ShellExecuteA(NULL, "open", cmd.c_str(), (LPCSTR)lb.c_str(), NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__)
	system((const char*)lb.c_str());
#endif
}

void TWE::shell_open_by_command(TWEUTILS::SmplBuf_WChar& name, const wchar_t* wstr_cmd) {
	if (name.length() > 0) {
		TWE::shell_open_by_command(name.c_str(), wstr_cmd);
	}
}

#endif
