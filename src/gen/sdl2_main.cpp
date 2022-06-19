/* Copyright (C) 2019-2022 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#if (defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__))

 // version
#include "version_weak.h"

/*****************************************************************
 * CONFIGURATION
 *****************************************************************/
// output debug message
#ifdef _DEBUG
# define _DEBUG_MESSAGE
#endif

/*****************************************************************
 * HEADER FILES
 *****************************************************************/
#include <string>

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <windows.h>
#include <conio.h>
#include <signal.h>
#elif defined(__APPLE__) || defined(__linux)
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
# ifdef __APPLE__
# include <mach-o/dyld.h>
# endif
#endif

// for thread
#include <thread>
#include <mutex>

// for check file content
#include <regex>

// for logging
#include <iostream>
#include <fstream>

// read configurations
#include "sdl2_config.h"

// MWM5 library includes
#include "mwm50.h"
#include "M5Stack.h"
#include "twe_sdl_m5.h"

#if defined(_MSC_VER) || defined(__MINGW32__)
#include "../win/msc_term.hpp"
#elif defined(__APPLE__)
#include "../mac/osx_term.hpp"
#elif defined(__linux)
#include "../linux/linux_term.hpp"
#endif

#include "serial_common.hpp"
#if defined(MWM5_BUILD_RASPI)
# include "serial_duo.hpp"
# include "modctrl_duo.hpp"
# include "modctrl_raspi.hpp"
# include "serial_termios.hpp"
#endif
#include "modctrl_ftdi.hpp"
#include "serial_ftdi.hpp"
#include "serial_srv_pipe.hpp"

#include "esp32/esp32_lcd_color.h"

#include "twe_sys.hpp"

//Using SDL
#include "sdl2_common.h"
#include "sdl2_button.hpp"
#include "sdl2_keyb.hpp"
#include "sdl2_icon.h"
#include "sdl2_utils.hpp"

// include getopt.c
#include "../oss/oss_getopt.h"

/***********************************************************
 * PROTOTYPES
 ***********************************************************/
static void s_getopt(int argc, char* args[]);
static void s_init();
static void s_init_sdl();

static void s_sketch_setup();
static void s_sketch_loop();

static void exit_err(const char* msg, const char * msg_param = nullptr);
static void signalHandler( int signum );

static void s_ser_hook_on_write(const uint8_t* p, int len);

static int _Get_Physical_CPU_COUNT_query_by_external_command();

#ifdef _DEBUG_MESSAGE
#define DBGOUT(...) fprintf(stderr, __VA_ARGS__) 
#else
#define DBGOUT(msg,...)
#endif

/***********************************************************
 * VARIABLES
 ***********************************************************/

 /** @brief	Number of physical cpus */
static int _physical_cpu_count = 0;

// exit flag
bool g_quit_sdl_loop = false;
uint32_t g_sdl2_user_event_type = 0;

//The window we'll be rendering to
SDL_Window* gWindow = nullptr;

//The window renderer
SDL_Renderer* gRenderer = nullptr;

// Rendering Mutex - if multi-threaded, use mutex for resource access.
#if MWM5_SDL2_USE_MULTITHREAD_RENDER == 1 && MWM5_USE_SDL2_MUTEX == 1
LockGuard::mutex_type gMutex_Render = nullptr;
#elif MWM5_SDL2_USE_MULTITHREAD_RENDER == 1 && MWM5_USE_SDL2_MUTEX == 0
LockGuard::mutex_type gMutex_Render;
#else
const bool gMutex_Render = true;
#endif

volatile bool g_app_busy = false; // if busy flag is set, render darker
volatile uint32_t g_app_busy_tick = 0; // starting time of busy

void screen_set_busy() {
	g_app_busy = true;
	g_app_busy_tick = SDL_GetTicks();
}

void screen_unset_busy() {
	g_app_busy = false;
	g_app_busy_tick = 0;
}

void screen_hide_cursor() {
	SDL_ShowCursor(SDL_DISABLE);
}

void screen_show_cursor() {
	SDL_ShowCursor(SDL_ENABLE);
}

// fade effect switch
#ifndef MWM5_ENABLE_FADE_EFFECT
# define MWM5_ENABLE_FADE_EFFECT 1
#endif
bool g_enable_fade_effect = MWM5_ENABLE_FADE_EFFECT;

// Window icon
SDL_Surface *gSurface_icon_win = nullptr;
std::unique_ptr<uint32_t[]> g_pixdata_icon_win;

//Screen dimension constants
int SCREEN_WIDTH = 640;
int SCREEN_HEIGHT = 480;
int SCREEN_POS_X = 0;
int SCREEN_POS_Y = 0;

// SCREEN window size preset.
static const int screen_size_tbl[][2] = {
#if M5_SCREEN_HIRES == 0
	{ 640,480 }, { 960,720 }, { 1280,720 }, { 1280,960 }, { 1920,1080 }, { 320,240 }, { -1,-1 }
#elif M5_SCREEN_HIRES == 1
	{640,480}, {1280, 720}, {1280,960}, {1920,1440}, {2560,1440}, {320,240}, {-1,-1}
#endif
};

// console
#if defined(_MSC_VER) || defined(__MINGW32__)
TWETerm_WinConsole con_screen(80, 24);
#elif defined(__APPLE__)
TWETerm_MacConsole con_screen(80, 24);
#elif defined(__linux)
TWETerm_LinuxConsole con_screen(80, 24);
#endif
ITerm& TWETERM::the_sys_console = con_screen; // global object reference of system console.
TWECUI::KeyInput TWE::the_sys_keyboard(512); // cosole keyboard buffer (mainly for debugging use)

TWE_GetChar_CONIO con_keyboard;
TWE_PutChar_CONIO TWE::WrtCon;

// The Serial Device
#if defined(_MSC_VER) || defined(__MINGW32__)
	SerialFtdi Serial;
	SerialFtdi Serial2;
	TWE_PutChar_Serial<SerialFtdi> TWE::WrtTWE(Serial2);
	TweModCtlFTDI obj_ftdi(Serial2);
	TweProg TWE::twe_prog(new TweBlProtocol<TWE::SerialFtdi, TweModCtlFTDI>(Serial2, obj_ftdi));
#elif defined(__APPLE__)
#	if defined(MWM5_SERIAL_NO_FTDI)
	SerialDummy Serial;
	SerialSrvPipe Serial2(nullptr);
	TWE_PutChar_Serial<SerialSrvPipe> TWE::WrtTWE(Serial2);
	TweModCtlSerialSrvPipe modctl_ser2(Serial2);
	TweProg TWE::twe_prog(new TweBlProtocol<SerialSrvPipe, TweModCtlSerialSrvPipe>(Serial2, modctl_ser2));
#	elif defined(MWM5_SERIAL_DUMMY)
	SerialDummy Serial;
	SerialDummy Serial2;
	TWE_PutChar_Serial<SerialDummy> TWE::WrtTWE(Serial2);
	TweModCtlDummy obj_dummy;
	TweProg TWE::twe_prog(new TweBlProtocol<TWE::SerialDummy, TweModCtlDummy>(Serial2, obj_dummy));
#	else
	SerialFtdi Serial;
	SerialFtdi Serial2;
	TWE_PutChar_Serial<SerialFtdi> TWE::WrtTWE(Serial2);
	TweModCtlFTDI obj_ftdi(Serial2);
	TweProg TWE::twe_prog(new TweBlProtocol<TWE::SerialFtdi, TweModCtlFTDI>(Serial2, obj_ftdi));
#	endif
#elif defined(__linux)
#	if defined(MWM5_BUILD_RASPI)  && defined(MWM5_SERIAL_DUO)
	// RASPI with /dev/serial0 and GPIO.
	SerialFtdi Serial;
	// Serial2 is supporting both UART(SerialTermios) and FTDI(SerialFTDI).
	SerialDuoRaspi Serial2;
	TWE_PutChar_Serial<SerialDuoRaspi> TWE::WrtTWE(Serial2);

	// TWE BOOTLOADER PROTOCOL
	using TweModCtlDuoRaspi = TweModCtlDuo<SerialDuoRaspi, TweModCtlRaspi, TweModCtlFTDI>;
	TweModCtlRaspi objModC_RaspiGPIO; // GPIO
	TweModCtlFTDI objModC_FTDI(Serial2._objD); // FTDI Bitbang
	TweModCtlDuoRaspi objModC_raspi(Serial2, objModC_RaspiGPIO, objModC_FTDI); // combined object
	TweProg TWE::twe_prog(new TweBlProtocol<SerialDuoRaspi, TweModCtlDuoRaspi>(Serial2, objModC_raspi));
#	else
	// standard linux with FTDI only.
	SerialFtdi Serial;
	SerialFtdi Serial2;
	TWE_PutChar_Serial<SerialFtdi> TWE::WrtTWE(Serial2);
	TweModCtlFTDI obj_ftdi(Serial2);
	TweProg TWE::twe_prog(new TweBlProtocol<TWE::SerialFtdi, TweModCtlFTDI>(Serial2, obj_ftdi));
#	endif
#endif

// the M5 stack instance
#if M5_SCREEN_HIRES == 0
static const int M5_LCD_WIDTH = 320;
static const int M5_LCD_HEIGHT = 240;
static const int M5_LCD_DIV_FACTOR = 2;
#elif M5_SCREEN_HIRES == 1
static const int M5_LCD_WIDTH = 640;
static const int M5_LCD_HEIGHT = 480;
static const int M5_LCD_DIV_FACTOR = 1;
#endif
M5Stack M5(M5_LCD_WIDTH, M5_LCD_HEIGHT);

// settings from getopt
struct _gen_preference {
	int render_engine;    // choose rendering option (osx Metal)
	int serial_safe_mode; // choose serial safe modes
	int game_controller;  // 0: not use, 1: use game controller
} the_pref;

/***********************************************************
 * FILES
 ***********************************************************/
#define LOG_DIRNAME L"log"
#define LOG_FINENAME L"twestage"
#define LOG_FILEEXT L"log"

// procedure for generic operation
struct app_core_generic_procs {
	void module_reset() {
		if (Serial2.is_opened()) {
			Serial2.begin(115200);
			delay(10);
			twe_prog.reset_module();
		}
	}

	void type_plus3() {
		if (Serial2.is_opened()) {
			const uint8_t buf[] = {'+'};
			Serial2.write(buf, 1);
			delay(400);
			Serial2.write(buf, 1);
			delay(400);
			Serial2.write(buf, 1);
		}
	}

	void clip_copy_request() {
		the_clip.copy.request_to_copy();
	}

	void clip_paste() {
		the_clip.paste.past_from_clip();
	}

	void open_lib_dir(const wchar_t *projname, const wchar_t *app_open = nullptr) {
		auto&& dir_project = make_full_path(the_cwd.get_dir_sdk_twenet_lib(), "src", projname);
		
		if (app_open != nullptr) {
			shell_open_by_command(dir_project.c_str(), app_open);
		}
		else {
			shell_open_folder(dir_project.c_str());
		}
	}

} the_generic_ops;

// class for SDL2 handling
struct app_core_sdl {
	static const int M5_LCD_SUB_WIDTH = 640;
	static const int M5_LCD_SUB_HEIGHT = 480;
	
	static const int M5_LCD_TEXTE_WIDTH = 320;
	static const int M5_LCD_TEXTE_HEIGHT = 32;

	static const int SDLOP_NEXT = -1;
	static const int SDLOP_PREV = -2;

	// Texture
	SDL_Texture* mTexture;
	SDL_Texture* mTexture_sub;
	SDL_Texture* mTexture_texte;

	int nAltDown;
	static const int N_ALTDOWN_HIDE = 0;
	static const int N_ALTDOWN_SHOWN = 2;
	static const int N_ALTDOWN_ON_KEY = 1;
	static const int N_ALTDOWN_FADE_COUNT_MAX = -7;
	bool IS_N_ALTDOWN_SHOWN_OR_ONKEY(int n) { return n > 0; }

	int nAltState;
	int nTextEditing;

	// main screen
	TWETerm_M5_Console sub_screen; // reinit_state the screen.
	TWETerm_M5_Console sub_screen_tr; // reinit_state the screen.
	TWETerm_M5_Console sub_screen_br; // reinit_state the screen.

	// key input display
	TWETerm_M5_Console sub_textediting;

	// button quit
	std::unique_ptr<twe_wid_button> sp_btn_quit;

	// button quit
	std::unique_ptr<twe_wid_button> sp_btn_A;

	// button quit
	std::unique_ptr<twe_wid_button> sp_btn_B;

	// button quit
	std::unique_ptr<twe_wid_button> sp_btn_C;

	// FULL SCREEN RENDERING OBJECT (mimic M5Stack)
	M5Stack M5_SUB;

	// Text Editing Object
	M5Stack M5_TEXTE;
	int _nTextEdtLen;
	
	// SCREEN render mode (0:scanline, 1:LCD like, 2:simple blur 3:blocky)
	int render_mode_m5_main;
	static const int render_mode_m5_main_maxval = 3;

	// fading out when quitting.
	int quit_loop_count;
	static const int QUIT_LOOP_COUNT_MAX = 32;
	int backgound_render_count;

	// fullscreen
	int _bfullscr;
	int _nscrsiz;

	// focus
	bool _is_get_focus;
	bool _is_window_hidden;
	int _render_cnt;

	// tick control
	uint32_t _u32tick_sdl_loop_head;

	// log file
	bool _b_logging;
	std::unique_ptr<std::filebuf> _file_buf;
	std::unique_ptr<std::ostream> _file_os;
	SmplBuf_ByteSL<1024> _file_fullpath; // not in wchar_t (for ShellExecureA())

	// constructor
	app_core_sdl()
		: mTexture(nullptr)
		, mTexture_sub(nullptr)
		, mTexture_texte(nullptr)
		, nAltDown(0)
		, nAltState(0)
		, sub_screen(80, 32, { 0, 0, M5_LCD_SUB_WIDTH / 2, M5_LCD_SUB_HEIGHT }, M5_SUB)
		, sub_screen_tr(80, 16, { M5_LCD_SUB_WIDTH / 2, 0, M5_LCD_SUB_WIDTH / 2, M5_LCD_SUB_HEIGHT / 2 }, M5_SUB)
		, sub_screen_br(80, 16, { M5_LCD_SUB_WIDTH / 2, M5_LCD_SUB_HEIGHT / 2, M5_LCD_SUB_WIDTH / 2, M5_LCD_SUB_HEIGHT / 2 }, M5_SUB)
		, sub_textediting(80, 1, { 0, 0, M5_LCD_TEXTE_WIDTH, M5_LCD_TEXTE_HEIGHT }, M5_TEXTE)
#if M5_SCREEN_HIRES == 0
		, sp_btn_A(new twe_wid_button(L"[  A  ]", { 4, 480 - 32, 208, 32 }))
		, sp_btn_B(new twe_wid_button(L"[  B  ]", { 4 + 208 + 4, 480 - 32, 208, 32 }))
		, sp_btn_C(new twe_wid_button(L"[  C  ]", { 4 + 208 + 4 + 208 + 4, 480 - 32, 208, 32 }))
		, sp_btn_quit(new twe_wid_button(L"[閉]", { 640 - 64, 0, 64, 32 }))
#elif M5_SCREEN_HIRES == 1
		, sp_btn_A(new twe_wid_button(L"[  A  ]", { 4, 480 - 24, 208, 24 }))
		, sp_btn_B(new twe_wid_button(L"[  B  ]", { 4 + 208 + 4, 480 - 24, 208, 24 }))
		, sp_btn_C(new twe_wid_button(L"[  C  ]", { 4 + 208 + 4 + 208 + 4, 480 - 24, 208, 24 }))
		, sp_btn_quit(new twe_wid_button(L"[閉]", { 640 - 48, 0, 48, 24 }))
#endif
		, M5_SUB(M5_LCD_SUB_WIDTH, M5_LCD_SUB_HEIGHT)
		, M5_TEXTE(M5_LCD_TEXTE_WIDTH, M5_LCD_TEXTE_HEIGHT)
		, render_mode_m5_main(0)
		, quit_loop_count(-1), backgound_render_count(0)
		, _bfullscr(0)
		, _nscrsiz(0)
		, _nTextEdtLen(0)
		, nTextEditing(0)
		, _file_buf(), _file_os(), _file_fullpath()
		, _b_logging(false)
		, _is_get_focus(true)
		, _is_window_hidden(false)
		, _render_cnt(0)
		, _u32tick_sdl_loop_head(0)
	{
	}

	// destructor
	~app_core_sdl() {
		SDL_DestroyTexture(mTexture);
		mTexture = NULL;

		SDL_DestroyTexture(mTexture_sub);
		mTexture_sub = NULL;

		SDL_DestroyTexture(mTexture_texte);
		mTexture_texte = NULL;

		sp_btn_A.release();
		sp_btn_B.release();
		sp_btn_C.release();
		sp_btn_quit.release();

		//Destroy window	
		SDL_FreeSurface(gSurface_icon_win);
		SDL_DestroyRenderer(gRenderer);
		SDL_DestroyWindow(gWindow);

		gWindow = NULL;
		gRenderer = NULL;

		//Quit SDL subsystems
		SDL_Quit();
	}

	/* SDL Initialize */
	void init_sdl() {
		// Texture
		mTexture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
			M5_LCD_WIDTH * 2, M5_LCD_HEIGHT * 2); // alloc double size (for optional rendering)
		if (mTexture == NULL)
			exit_err("SDL_CreateTexture()");

		// Texture
		mTexture_sub = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
			M5_LCD_SUB_WIDTH, M5_LCD_SUB_HEIGHT);
		if (mTexture_sub == NULL)
			exit_err("SDL_CreateTexture()");

		// Texture of input area
		mTexture_texte = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
			M5_LCD_TEXTE_WIDTH, M5_LCD_TEXTE_HEIGHT);
		if (mTexture_texte == NULL)
			exit_err("SDL_CreateTexture()");

		// enable text input
		SDL_StartTextInput();
		
		// set candidate window position other than (0,0)
		const SDL_Rect rctIME = { 100, 100, 100, 100 };
		SDL_SetTextInputRect((SDL_Rect*)&rctIME); 

		
#if defined(MWM5_FULLSCREEN_AT_LAUNCH)
		// set fullscreen as default (for some platform)
		_bfullscr = true;
		SDL_SetWindowFullscreen(gWindow, SDL_WINDOW_FULLSCREEN);
#endif
	}

	// calculate screen position from 640,480 based screen to actual screen coordinates.
	inline int screen_weight(int i) {
		return i * SCREEN_WIDTH / 640;
	}

	void update_help_screen() {
		sub_screen_tr << "\033[2J";
		sub_screen_tr << "\033[31;1m[シリアルポート]\033[0m";
		
		#if defined(__APPLE__)
		# define STR_ALT "Cmd"
		#else
		# define STR_ALT "Alt"
		#endif
		
		if (!Serial2.is_opened()) sub_screen_tr << " - ｵﾌﾗｲﾝ";

		if (Serial2.ser_count > 0) {
			for (int i = 0; i < Serial2.ser_count; i++) {
				sub_screen_tr << crlf << printfmt(STR_ALT "+%d ", i + 1); // ALT+1 .. 3

				bool bOpened = false;
				if (Serial2.ser_devname[i][0] != 0 && !strncmp(Serial2.ser_devname[i], Serial2.get_devname(), sizeof(Serial2.ser_devname[i]))) {
					bOpened = true;
				}

				if (bOpened) {
					sub_screen_tr << "\033[33;1m";
				}

				sub_screen_tr << printfmt("%s(%s)%c"
					, Serial2.ser_desc[i]
					, Serial2.ser_devname[i]
					, bOpened ? '*' : ' ');

				if (bOpened) {
					sub_screen_tr << "\033[0m";
				}
			}
		}
		sub_screen_tr << crlf << STR_ALT "+0 : 切断&再スキャン";


		sub_screen_tr << crlf;
		sub_screen_tr << crlf << STR_ALT "+L : ログ";
		if (_b_logging) {
			sub_screen_tr << "\033[33;1m(記録中)\033[0m";
		}
#if (defined(_MSC_VER) || defined(__APPLE__) || defined(__MINGW32__))
		sub_screen_tr << crlf << "  Shift+" STR_ALT " ログフォルダを開く";
#endif

		static bool b_static_message = false;
		if (!b_static_message) {
			b_static_message = true;

			sub_screen << "[基本操作]";
			sub_screen << crlf << "ESC   : 戻る";
			sub_screen << crlf << "Enter/↑↓→← : 選択";

			sub_screen << crlf << crlf << "[TWELITE 操作]";
			sub_screen << crlf << STR_ALT"+I : + + + (ｲﾝﾀﾗｸﾃｨﾌﾞﾓｰﾄﾞ)";
			sub_screen << crlf << STR_ALT"+R : モジュールのリセット";

			sub_screen << crlf << crlf << "[ボタン操作]";
			sub_screen << crlf << STR_ALT"+A : Ａ(左)ボタン";
			sub_screen << crlf << STR_ALT"+S : Ｂ(中)ボタン";
			sub_screen << crlf << STR_ALT"+D : Ｃ(右)ボタン";
			sub_screen << crlf << "  Shift+" STR_ALT " 長押し";

			sub_screen << crlf << crlf << "[コピー＆ペースト]";
			sub_screen << crlf << STR_ALT"+C : ｸﾘｯﾌﾟﾎﾞｰﾄﾞへコピー";
			sub_screen << crlf << STR_ALT"+V : ｸﾘｯﾌﾟﾎﾞｰﾄﾞよりペースト";

			sub_screen << crlf << crlf << "[その他]";
			sub_screen << crlf << STR_ALT"+F : フルスクリーン";
			sub_screen << crlf << "  Shift+" STR_ALT " 可能なら更に拡大";
			sub_screen << crlf << STR_ALT"+G : 描画方法変更";
			sub_screen << crlf << STR_ALT"+J : ｳｲﾝﾄﾞｳｻｲｽﾞ変更";

			sub_screen << crlf;
			sub_screen << crlf << STR_ALT"+Q : 終了";
		}
	}

	/**
	 * @fn	void update_help_desc(const wchar_t* msg) 
	 *
	 * @brief	Put a message on the bottom left screen.
	 */
	void update_help_desc(const wchar_t* msg) {
		sub_screen_br << L"\033[2J\033[H" << msg;
	}

	/**
	 * @fn	void update_textebox(const char* msg)
	 *
	 * @brief	Updates the textebox described by msg
	 *
	 * @param	msg	The message.
	 */
	void update_textebox(const char* msg, bool IME = false) {
		_nTextEdtLen = (int)strlen(msg);

		if (IME) {
			sub_textediting << "\033[2J\033[HIME:\033[31m" << msg << "\033[0m";
		}
		else {
			_nTextEdtLen = 1;
			sub_textediting << "\033[2J\033[H" << msg;
		}
	}

	/**
	 * @fn	void recalc_viewport()
	 *
	 * @brief	Calc view position especially when full screen.
	 */
	void recalc_viewport(SDL_Window *window,
										SDL_Renderer *renderer,
										SDL_Rect *viewport) {
		Uint8 r, g, b, a;
		SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);
		SDL_SetRenderDrawColor(renderer, r, g, b, a);
		int w, h;
		SDL_GetWindowSize(window, &w, &h);

		switch (_bfullscr) {
		case 0:
			SCREEN_WIDTH = screen_size_tbl[_nscrsiz][0];
			SCREEN_HEIGHT = screen_size_tbl[_nscrsiz][1];
			viewport->w = SCREEN_WIDTH;
			viewport->h = SCREEN_HEIGHT;
			viewport->x = 0;
			viewport->y = 0;
			break;
		case 1:
			SCREEN_WIDTH = screen_size_tbl[_nscrsiz][0];
			SCREEN_HEIGHT = screen_size_tbl[_nscrsiz][1];
			viewport->w = SCREEN_WIDTH;
			viewport->h = SCREEN_HEIGHT;
			viewport->x = (w - viewport->w) / 2;
			viewport->y = (h - viewport->h) / 2;
			break;
		case 2:
			SCREEN_WIDTH = w;
			SCREEN_HEIGHT = h;
			viewport->w = SCREEN_WIDTH;
			viewport->h = SCREEN_HEIGHT;
			viewport->x = 0;
			viewport->y = 0;
			break;
		}

		#if defined(_DEBUG) && defined(_DEBUG_MESSAGE)
		fprintf(stderr, "\n{recalc_viewport: scr=(%d,%d)%c vp(%d,%d,%d,%d)}",
			w, h, _bfullscr ? 'F' : 'W',
			viewport->x, viewport->y, viewport->w, viewport->h 
		);
		#endif
		SDL_RenderSetViewport(renderer, viewport);
	}

	/**
	 * @fn	void refresh_entirescreen()
	 
	 * @brief	Refresh entire screen
	 */
	void refresh_entirescreen() {
		SDL_Rect rct;
		recalc_viewport(gWindow, gRenderer, &rct);
		SCREEN_POS_X = rct.x;
		SCREEN_POS_Y = rct.y;

		if (auto l = TWE::LockGuard(gMutex_Render, 32)) {
			M5.Lcd.update_line_all();
		}
		else {
			WrtCon << "!";
			// may corrupt display, anyway try to do.
			M5.Lcd.update_line_all();
		}
		
		M5_SUB.Lcd.update_line_all();
		M5_TEXTE.Lcd.update_line_all();

		sp_btn_quit->redraw();
		sp_btn_A->redraw();
		sp_btn_B->redraw();
		sp_btn_C->redraw();
	}

	inline void draw_point(uint32_t *tgt, RGBA c, uint8_t lumi = 0xFF) {
		uint8_t* p = (uint8_t*)tgt;

		if (lumi == 0xff) {
			*(p + 0) = 0xff; // c.u8col[3];
			*(p + 1) = c.u8col[2];
			*(p + 2) = c.u8col[1];
			*(p + 3) = c.u8col[0];
		} else {
			*(p + 0) = 0xff; //((signed)c.u8col[3] * lumi) >> 8;
			*(p + 1) = ((signed)c.u8col[2] * lumi) >> 8;
			*(p + 2) = ((signed)c.u8col[1] * lumi) >> 8;
			*(p + 3) = ((signed)c.u8col[0] * lumi) >> 8;
		}
	}

	void init_sdl_sub() {
		// font set (Shinonome 16 dot, full chars set)
		TWEFONT::createFontShinonome16_full(0x80, 0, 0);
		TWEFONT::createFontShinonome16_full(0x81, 0, 0, TWEFONT::U32_OPT_FONT_TATEBAI);
		TWEFONT::createFontShinonome16_full(0x82, 0, 0, TWEFONT::U32_OPT_FONT_YOKOBAI);
#if M5_SCREEN_HIRES == 0
		TWEFONT::createFontShinonome16_full(0x83, 0, 0, TWEFONT::U32_OPT_FONT_YOKOBAI | TWEFONT::U32_OPT_FONT_TATEBAI);
#elif M5_SCREEN_HIRES == 1
		TWEFONT::createFontMP12_std(0x83, 0, 0, TWEFONT::U32_OPT_FONT_YOKOBAI | TWEFONT::U32_OPT_FONT_TATEBAI);
#endif

		// help screen (shown by Alt press)
		sub_screen.set_font(0x80);
		int cols = sub_screen.get_cols();
		sub_screen.set_font(0x80, cols - 2);
		sub_screen.set_color(ALMOST_WHITE, BLACK);
		sub_screen.set_cursor(0); // 0: no 1: curosr 2: blink cursor
		sub_screen.force_refresh();

		sub_screen_tr.set_font(0x80);
		cols = sub_screen_tr.get_cols();
		sub_screen_tr.set_font(0x80, cols - 2);
		sub_screen_tr.set_color(ALMOST_WHITE, color565(0x00, 0x00, 0x40));
		sub_screen_tr.set_cursor(0); // 0: no 1: curosr 2: blink cursor
		sub_screen_tr.force_refresh();

		sub_screen_br.set_font(0x80);
		cols = sub_screen_br.get_cols();
		sub_screen_br.set_font(0x81, cols - 2);
		sub_screen_br.set_color(ALMOST_WHITE, color565(0x00, 0x40, 0x00));
		sub_screen_br.set_cursor(0); // 0: no 1: curosr 2: blink cursor
		sub_screen_br.force_refresh();

		sub_textediting.set_font(0x83);
		sub_textediting.set_color(color565(0x80, 0x80, 0x80), ALMOST_WHITE);
		sub_textediting.set_cursor(0); // 0: no 1: curosr 2: blink cursor
		sub_textediting.force_refresh();

		// quit button
		sp_btn_quit->setup(gRenderer);
		sp_btn_A->setup(gRenderer);
		sp_btn_B->setup(gRenderer);
		sp_btn_C->setup(gRenderer);

		// render help screen
		update_help_screen();
	}

	bool sdlop_window_size(int n) {
		const int MAX_ENTRY = sizeof(screen_size_tbl) / sizeof(int) / 2 - 1;

		int next_mode;
		if (n == SDLOP_NEXT)
			next_mode = _nscrsiz + 1;
		else if (n == SDLOP_PREV)
			next_mode = _nscrsiz - 1;
		else
			next_mode = n;

		if (next_mode < 0) next_mode = MAX_ENTRY - 1;
		if (next_mode >= MAX_ENTRY) next_mode = 0;
		
		if (next_mode != _nscrsiz) {
			_nscrsiz = next_mode;

			if (screen_size_tbl[_nscrsiz][0] == -1) _nscrsiz = 0;

			SCREEN_WIDTH = screen_size_tbl[_nscrsiz][0];
			SCREEN_HEIGHT = screen_size_tbl[_nscrsiz][1];

			SDL_SetWindowSize(gWindow, SCREEN_WIDTH, SCREEN_HEIGHT);
			refresh_entirescreen();

			return true;
		}
		return false;
	}
	
	bool sdlop_render_mode(int n) {
		int next_mode;
		if (n == SDLOP_NEXT)
			next_mode = render_mode_m5_main + 1;
		else if (n == SDLOP_PREV)
			next_mode = render_mode_m5_main - 1;
		else
			next_mode = n;

		if (next_mode < 0) next_mode = render_mode_m5_main_maxval;
		if (next_mode > render_mode_m5_main_maxval) next_mode = 0;
		
		render_mode_m5_main = next_mode;
		refresh_entirescreen();

		return true;
	}
	
	void handle_sdl_event(SDL_Event&e) {
		//User requests quit
		if (e.type == SDL_QUIT)
		{
			g_quit_sdl_loop = true;
		}

		/* USER EVENT */
		if (e.type == SDL_USEREVENT) {
			if (IS_SDL2_USERCODE(e.user.code, SDL2_USERCODE_CHANGE_SCREEN_SIZE)) {
				sdlop_window_size(SDL2_USERCODE_GET_BYTE1(e.user.code));
			} else
			if (IS_SDL2_USERCODE(e.user.code, SDL2_USERCODE_CHANGE_SCREEN_RENDER)) {
				sdlop_render_mode(SDL2_USERCODE_GET_BYTE1(e.user.code));
			}
			return;
		}

		/* EXPOSURE EVENT, REQUIRE ENTIRE REDRAW */
		if (e.type == SDL_WINDOWEVENT) {
			if (e.window.event == SDL_WINDOWEVENT_EXPOSED) {
				refresh_entirescreen();
			}

			if (e.window.event  == SDL_WINDOWEVENT_FOCUS_GAINED) {
				_is_get_focus = true;
				SDL_DisableScreenSaver();
			}

			if (e.window.event  == SDL_WINDOWEVENT_FOCUS_LOST) {
				_is_get_focus = false;
				SDL_EnableScreenSaver();
			}

			if (   e.window.event == SDL_WINDOWEVENT_MINIMIZED
				|| e.window.event == SDL_WINDOWEVENT_HIDDEN) {
				_is_window_hidden = true;
			}

			if (   e.window.event == SDL_WINDOWEVENT_SHOWN
				|| e.window.event == SDL_WINDOWEVENT_RESTORED
				|| e.window.event == SDL_WINDOWEVENT_EXPOSED) {
				_is_window_hidden = false;
			}

		}

		/* HANDLE BUTTON PRESS EVENT */
		if (_is_get_focus) { // behave only in focus.
			// handles button
			bool b_btn_handled = false;

			if (sp_btn_quit->update(e)) {
				if (sp_btn_quit->available()) {
					auto readstate = sp_btn_quit->read();
					g_quit_sdl_loop = true;
					return;
				}
				b_btn_handled = true;
			}
			
			if (sp_btn_A->update(e)) {
				if (sp_btn_A->available()) {
					auto readstate = sp_btn_A->read();
					if (readstate == 1) M5.BtnA._press = true;
					if (readstate == 2) M5.BtnA._lpress = true;
				}
				b_btn_handled = true;
			}

			if (sp_btn_B->update(e)) {
				if (sp_btn_B->available()) {
					auto readstate = sp_btn_B->read();
					if (readstate == 1) M5.BtnB._press = true;
					if (readstate == 2) M5.BtnB._lpress = true;
				}
				b_btn_handled = true;
			}

			if (sp_btn_C->update(e)) {
				if (sp_btn_C->available()) {
					auto readstate = sp_btn_C->read();
					if (readstate == 1) M5.BtnC._press = true;
					if (readstate == 2) M5.BtnC._lpress = true;
				}
				b_btn_handled = true;
			}

			if (b_btn_handled) return;

			// main screen
			//  push mouse event as KEY INPUT QUEUE
			static uint32_t t_last_R_clk;
			if (e.type == SDL_MOUSEBUTTONUP) {
				int x = (e.button.x - ::SCREEN_POS_X) * 640 / ::SCREEN_WIDTH / M5_LCD_DIV_FACTOR;
				int y = (e.button.y - ::SCREEN_POS_Y) * 480 / ::SCREEN_HEIGHT / M5_LCD_DIV_FACTOR;

				// con_screen << printfmt("{MOUSE %d,%d}", x, y);
				if (e.button.clicks == 1 || e.button.clicks == 0) {
					if (e.button.button == SDL_BUTTON_LEFT) {
						the_keyboard_sdl2.push(KeyInput::MOUSE_UP(x, y, 0));
					}
					if (e.button.button == SDL_BUTTON_RIGHT) {
						the_keyboard_sdl2.push(KeyInput::MOUSE_UP(x, y, 1));
					}
				}
				else if (e.button.clicks == 2) {
					if (e.button.button == SDL_BUTTON_RIGHT) {
						if (e.button.timestamp - t_last_R_clk < 400) {
							the_keyboard_sdl2.push(KeyInput::KEY_ESC);
						}
					}
				}
			}

			//  push mouse event as KEY INPUT QUEUE
			if (e.type == SDL_MOUSEBUTTONDOWN) {
				int x = (e.button.x - ::SCREEN_POS_X) * 640 / ::SCREEN_WIDTH / M5_LCD_DIV_FACTOR;
				int y = (e.button.y - ::SCREEN_POS_Y) * 480 / ::SCREEN_HEIGHT / M5_LCD_DIV_FACTOR;

				// con_screen << printfmt("{MOUSE %d,%d}", x, y);
				if (e.button.clicks == 1) {
					if (e.button.button == SDL_BUTTON_LEFT) {
						the_keyboard_sdl2.push(KeyInput::MOUSE_DOWN(x, y, 0));
					}
					if (e.button.button == SDL_BUTTON_RIGHT) {
						the_keyboard_sdl2.push(KeyInput::MOUSE_DOWN(x, y, 1));
						t_last_R_clk = e.button.timestamp;
					}
				}
				else if (e.button.clicks == 2) {
					if (e.button.button == SDL_BUTTON_LEFT) {
						the_keyboard_sdl2.push(KeyInput::MOUSE_DOUBLE(x, y, 0));
					}
					if (e.button.button == SDL_BUTTON_RIGHT) {
						//the_keyboard_sdl2.push(KeyInput::KEY_ESC);
						the_keyboard_sdl2.push(KeyInput::MOUSE_DOUBLE(x, y, 1));
					}
				}
			}

			//  push mouse event as KEY INPUT QUEUE
			if (e.type == SDL_MOUSEMOTION) {
				int x = (e.button.x - ::SCREEN_POS_X) * 640 / ::SCREEN_WIDTH / M5_LCD_DIV_FACTOR;
				int y = (e.button.y - ::SCREEN_POS_Y) * 480 / ::SCREEN_HEIGHT / M5_LCD_DIV_FACTOR;
				
				// con_screen << printfmt("{MOUSE %d,%d}", x, y);
				the_keyboard_sdl2.push(KeyInput::MOUSE_MOVE(x, y));
			}

			// wheel event
			if (e.type == SDL_MOUSEWHEEL) {
				//con_screen << printfmt("{WHEEL %d,%d}", e.wheel.x, e.wheel.y);
				the_keyboard_sdl2.push(KeyInput::MOUSE_WHEEL(e.wheel.x, e.wheel.y));				
			}

			// finger
			if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP || e.type == SDL_FINGERMOTION) {
				int x_s = int((::SCREEN_POS_X * M5_LCD_DIV_FACTOR + ::SCREEN_WIDTH) * e.tfinger.x - ::SCREEN_POS_X);
				int y_s = int((::SCREEN_POS_Y * M5_LCD_DIV_FACTOR + ::SCREEN_HEIGHT) * e.tfinger.y - ::SCREEN_POS_Y);

				int x = x_s * 640 / ::SCREEN_WIDTH / M5_LCD_DIV_FACTOR;
				int y = y_s * 480 / ::SCREEN_HEIGHT / M5_LCD_DIV_FACTOR;
				
				switch(e.type) {
				case SDL_FINGERDOWN:
					the_keyboard_sdl2.push(KeyInput::MOUSE_MOVE(x, y));
					the_keyboard_sdl2.push(KeyInput::MOUSE_DOWN(x, y)); break;
				case SDL_FINGERUP: the_keyboard_sdl2.push(KeyInput::MOUSE_UP(x, y)); break;
				case SDL_FINGERMOTION: the_keyboard_sdl2.push(KeyInput::MOUSE_MOVE(x, y)); break;
				}

				//printf("{TF:%f,%f->%d,%d}", e.tfinger.x, e.tfinger.y, x, y);
			}
		}

		/* MOUSE MOTION */
		if (e.type == SDL_MOUSEMOTION && _is_get_focus) { // behave only in focus.
			int d = screen_weight(32);

			if (e.motion.x < d && e.motion.y < d) {
				if (nAltDown != N_ALTDOWN_ON_KEY) { // when press/release event is happened, set nAltDown as 1.
					nAltDown = N_ALTDOWN_SHOWN; // set nAltDown flag as help screen is shown
				}
			}
			else {
				if (nAltDown == N_ALTDOWN_SHOWN) { // when help screen is shown.
					if (_is_get_focus) {
						if (g_enable_fade_effect) {
							nAltDown = N_ALTDOWN_FADE_COUNT_MAX;
						}
						else {
							nAltDown = N_ALTDOWN_HIDE;
						}
					}
					else nAltDown = N_ALTDOWN_HIDE;
				}
			}
		}

		if (the_pref.game_controller) {
			// game controller attached
			if (e.type == SDL_CONTROLLERDEVICEADDED) {
				int id = e.cdevice.which;

				if (SDL_IsGameController(id)) {
					SDL_GameController* pad = SDL_GameControllerOpen(id);

					if (pad) {
						SDL_Joystick* joy = SDL_GameControllerGetJoystick(pad);
						int instanceID = SDL_JoystickInstanceID(joy);

						// You can add to your own map of joystick IDs to controllers here.
						// YOUR_FUNCTION_THAT_CREATES_A_MAPPING(id, pad);
					}
				}
			}

			if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
			}

			/*
			 * e.jbuton.button
			 * A:0 B:1 X:2 Y:3 SELECT:4 START:6 LB:9 RB:10
			 * UP:11 DOWN:12 LEFT:13 RIGHT:14
			 */
			static uint32_t u32bmask = 0;
			static uint32_t u32timestamp_button_down[16];
			
			const uint32_t JOY_BTN_A = 0;
			const uint32_t JOY_BTN_B = 1;
			const uint32_t JOY_BTN_X = 2;
			const uint32_t JOY_BTN_Y = 3;
			const uint32_t JOY_BTN_SELECT = 4;
			const uint32_t JOY_BTN_START = 6;
			const uint32_t JOY_BTN_LB = 9;
			const uint32_t JOY_BTN_RB = 10;
			const uint32_t JOY_BTN_UP = 11;
			const uint32_t JOY_BTN_DOWN = 12;
			const uint32_t JOY_BTN_LEFT = 13;
			const uint32_t JOY_BTN_RIGHT = 14;
			const uint32_t MAX_BUTTON_NUMBER = 14;
			
			if (e.type == SDL_CONTROLLERBUTTONDOWN) {
				auto b = e.jbutton.button;
				if (b <= MAX_BUTTON_NUMBER) {
					u32timestamp_button_down[b] = e.jbutton.timestamp;
					u32bmask |= (1UL << b);
				}

				switch (b) {
				case JOY_BTN_UP: the_keyboard_sdl2.push(KeyInput::KEY_UP); break;
				case JOY_BTN_DOWN: the_keyboard_sdl2.push(KeyInput::KEY_DOWN); break;
				case JOY_BTN_LEFT: the_keyboard_sdl2.push(KeyInput::KEY_ESC); break;
				case JOY_BTN_RIGHT: the_keyboard_sdl2.push(KeyInput::KEY_ENTER); break;
					// M5.BtnB._press = true; sp_btn_B->show_button((int)twe_wid_button::E_BTN_STATE::BTNDOWN); break;
				case JOY_BTN_Y: the_keyboard_sdl2.push(KeyInput::KEY_ESC); break;
				}

				return;
			}

			if (e.type == SDL_CONTROLLERBUTTONUP) {
				auto b = e.jbutton.button;
				if (b <= MAX_BUTTON_NUMBER) {
					uint32_t tdiff = e.jbutton.timestamp - u32timestamp_button_down[e.jbutton.button];
					bool b_long_press = false;
					// check if it's long press or not
					if (u32bmask & (1UL << b)
						&& (tdiff > 700 && tdiff < 10000))
					{
						b_long_press = true;
					}
					u32bmask &= ~(1UL << b); // clear the bit

					if (b_long_press) {
						switch (b) {
						case JOY_BTN_A: M5.BtnB._lpress = true; sp_btn_B->show_button((int)twe_wid_button::E_BTN_STATE::BTNUP_LONG); true; break;
						case JOY_BTN_B: M5.BtnC._lpress = true; sp_btn_C->show_button((int)twe_wid_button::E_BTN_STATE::BTNUP_LONG); true; break;
						case JOY_BTN_X: M5.BtnA._lpress = true; sp_btn_A->show_button((int)twe_wid_button::E_BTN_STATE::BTNUP_LONG); true; break;
						default: break;
						}
					}
					else {
						switch (b) {
						case JOY_BTN_A: M5.BtnB._press = true; sp_btn_B->show_button((int)twe_wid_button::E_BTN_STATE::BTNDOWN); break;
						case JOY_BTN_B: M5.BtnC._press = true; sp_btn_C->show_button((int)twe_wid_button::E_BTN_STATE::BTNDOWN); break;
						case JOY_BTN_X: M5.BtnA._press = true; sp_btn_A->show_button((int)twe_wid_button::E_BTN_STATE::BTNDOWN); break;
						
						default: break;
						}
					}
				}
				return;
			}

			if (e.type == SDL_CONTROLLERAXISMOTION) {
				return;
			}
		}

		// just test
		if (e.type == SDL_TEXTEDITING) {
			if (e.text.text[0] == 0) {
				nTextEditing = -31;
			} else {
				nTextEditing = 1;
			}
			update_textebox(e.text.text, true);
		}

		if (e.type == SDL_TEXTINPUT) {
			// normal keyinput
			// the_keyboard_sdl2.handle_event(e);
			if (!nAltState && the_keyboard_sdl2.handle_event(e)) {
				nTextEditing = -31;

				update_textebox(e.text.text);
				return;
			}
		}

		// handle KP_
		if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_KP_ENTER) {
			the_keyboard_sdl2.handle_event(e);
		}

		if (e.type == SDL_DROPFILE)	{
			char* str_dir;

			str_dir = e.drop.file;
#if 0
			SDL_ShowSimpleMessageBox(
				SDL_MESSAGEBOX_INFORMATION,
				"File dropped on window",
				str_dir,
				gWindow
			);
#endif
			the_file_drop.new_drop(e.drop.file);

			SDL_free(str_dir);
			return;
		}
		
		if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
			// normal keyinput
			if (the_keyboard_sdl2.handle_event(e)) {
				return;
			}

			if (e.key.keysym.mod & (KMOD_STG)) {
				if (
#ifdef __APPLE__
					   e.key.keysym.scancode == SDL_SCANCODE_LGUI
					|| e.key.keysym.scancode == SDL_SCANCODE_RGUI
#else
					   e.key.keysym.scancode == SDL_SCANCODE_LALT
					|| e.key.keysym.scancode == SDL_SCANCODE_RALT
#endif
					) {
					nAltDown = N_ALTDOWN_ON_KEY;
					nAltState = 1;
					update_help_desc(L"");
				}
			}

			bool bhandled = false;
			switch (e.key.keysym.scancode) {
			case SDL_SCANCODE_1:
			case SDL_SCANCODE_2:
			case SDL_SCANCODE_3:
			case SDL_SCANCODE_4:
			case SDL_SCANCODE_5:
			case SDL_SCANCODE_6:
			case SDL_SCANCODE_7:
			case SDL_SCANCODE_8:
				if (e.key.keysym.mod & (KMOD_STG)) {
					if (e.type == SDL_KEYDOWN) {
						update_help_desc(L"シリアルポートをオープンします");
					} else {
						int n = e.key.keysym.scancode - SDL_SCANCODE_1;
						if (n >= 0 && n < Serial2.ser_count) {
							Serial2.close();
							Serial2.open(Serial2.ser_devname[n]);
						}
						
						bhandled = true;
					}
				}
				break;
			case SDL_SCANCODE_0:
				if (e.key.keysym.mod & (KMOD_STG)) {
					if (e.type == SDL_KEYDOWN) {
						update_help_desc(L"シリアルポートをクローズします");
					} else {
						Serial2.close();
						
						Serial2.list_devices();
						update_help_screen();
						
						bhandled = false;
					}
				}
				break;

			case SDL_SCANCODE_C:
				if (e.key.keysym.mod & (KMOD_STG)) {
					if (e.type == SDL_KEYDOWN) {
						update_help_desc(L"画面文字列をクリップボードにコピーします");
					}
					else {
						the_generic_ops.clip_copy_request();
						bhandled = true;
					}
				}
				break;

			case SDL_SCANCODE_V:
				if (e.key.keysym.mod & (KMOD_STG)) {
					if (e.type == SDL_KEYDOWN) {
						update_help_desc(L"クリップボードの文字列をペーストします");
					}
					else {
						the_generic_ops.clip_paste();
						bhandled = true;
					}
				}
				break;

			case SDL_SCANCODE_A:
				if (e.key.keysym.mod & (KMOD_STG)) {
					if (e.type == SDL_KEYDOWN) {
						update_help_desc(L"ボタンAを入力します");
					} else {
						if (e.key.keysym.mod & (KMOD_SHIFT))
							M5.BtnA._lpress = true;
						else
							M5.BtnA._press = true;

						bhandled = true;
					}
				}
				break;
			case SDL_SCANCODE_S:
				if (e.key.keysym.mod & (KMOD_STG)) {
					if (e.type == SDL_KEYDOWN) {
						update_help_desc(L"ボタンBを入力します");
					} else {
						if (e.key.keysym.mod & (KMOD_SHIFT))
							M5.BtnB._lpress = true;
						else
							M5.BtnB._press = true;

						bhandled = true;
					}
				}
				break;
			case SDL_SCANCODE_D:
				if (e.key.keysym.mod & (KMOD_STG)) {
					if (e.type == SDL_KEYDOWN) {
						update_help_desc(L"ボタンCを入力します");
					} else {
						if (e.key.keysym.mod & (KMOD_SHIFT))
							M5.BtnC._lpress = true;
						else
							M5.BtnC._press = true;
							
						bhandled = true;
					}
				}
				break;

			case SDL_SCANCODE_R:
				if (e.key.keysym.mod & (KMOD_STG)) {
					if (e.type == SDL_KEYDOWN) {
						update_help_desc(L"TWELITE無線モジュールのハードウェアリセットをします");
					} else {
						the_generic_ops.module_reset();
						bhandled = true;
					}
				}
				break;

			case SDL_SCANCODE_RETURN:
			case SDL_SCANCODE_F:
				if (e.key.keysym.mod & (KMOD_STG)) {
					if (e.type == SDL_KEYDOWN) {
						update_help_desc(L"フルスクリーン画面に設定します。Shift+で、スクリーン最大に拡大します");
					} else {
						_bfullscr = (_bfullscr == 0);

						if (_bfullscr && e.key.keysym.mod & (KMOD_SHIFT)) _bfullscr = 2;

						if (_bfullscr) {
							SDL_SetWindowFullscreen(gWindow, SDL_WINDOW_FULLSCREEN);
						} else {
							SDL_SetWindowFullscreen(gWindow, 0);
						}

						refresh_entirescreen();
						bhandled = true;
					}
				}
				break;

			case SDL_SCANCODE_I:
			case SDL_SCANCODE_P:
				if (e.key.keysym.mod & (KMOD_STG)) {
					if (e.type == SDL_KEYDOWN) {
						update_help_desc(L"TWELITE無線モジュールに + + + を入力します");
					} else {
						the_generic_ops.type_plus3();
						
						bhandled = true;
					}
				}
				break;

			case SDL_SCANCODE_J:
				if (e.key.keysym.mod & (KMOD_STG)) {
					if (e.type == SDL_KEYDOWN) {
						update_help_desc(L"画面サイズを変更します。"
						                 L"キーの入力ごとに640x480/960x720/1280x720/1920x1080/320x240の順で変更します");

					} else {
						sdlop_window_size(SDLOP_NEXT);

						bhandled = true;
					}
				}
				break;

			case SDL_SCANCODE_G:
				if (e.key.keysym.mod & (KMOD_STG)) {
					if (e.type == SDL_KEYDOWN) {
						update_help_desc(L"画面の描画方式を変更します。キーを押すたびにLCD風/CRT風/ぼやけ/ブロックの順で切り替えます");
					} else {
						sdlop_render_mode(SDLOP_NEXT);
						bhandled = true;
					}
				}
				break;

			case SDL_SCANCODE_Q:
				if (e.key.keysym.mod & (KMOD_STG)) {
					if (e.type == SDL_KEYDOWN) {
						update_help_desc(L"TWELITE STAGEの終演です。ごきげんよう");
					} else {
						if (_bfullscr) {
							// exit wiht fullscr will cause hang up! (SDL2.0.12, OSX)
							SDL_SetWindowFullscreen(gWindow, 0);
							_bfullscr = 0;
						}
						
						g_quit_sdl_loop = true;
						bhandled = true;
					}
				}
				break;

			case SDL_SCANCODE_T: // open TWENET/current/src/twesettigns via `code'
				if ((e.key.keysym.mod & KMOD_STG)
					&& (e.key.keysym.mod & KMOD_SHIFT)
					&& (e.key.keysym.mod & KMOD_CTRL)
					) {
					if (e.type == SDL_KEYDOWN) {
						the_generic_ops.open_lib_dir(L"twesettings", L"code");
					}
				}
				break;

			case SDL_SCANCODE_M: // open TWENET/current/src/mwx via `code'
				if ((e.key.keysym.mod & KMOD_STG) 
					&& (e.key.keysym.mod & KMOD_SHIFT)
					&& (e.key.keysym.mod & KMOD_CTRL)
				) {
					if (e.type == SDL_KEYDOWN) {
						the_generic_ops.open_lib_dir(L"mwx", L"code");
					}
				}
				break;

			case SDL_SCANCODE_L:
				if (e.key.keysym.mod & (KMOD_STG)) {
					if (e.type == SDL_KEYDOWN) {
						update_help_desc(L"twestage.logにシリアルログを追記します");
					} else
					if (e.key.keysym.mod & (KMOD_SHIFT)) { // KEY UP WITH SHIFT
						// opens log storing dir
						if (the_cwd.get_dir_log().length() > 0) {
							shell_open_default(the_cwd.get_dir_log());
						}
						else {
							SDL_ShowSimpleMessageBox(
								SDL_MESSAGEBOX_INFORMATION,
								"TWELITE Stage",
								"ログファイルを格納するディレクトリが存在しません",
								gWindow
							);
						}

						bhandled = true;
					} else { // KEY UP
						if (!_b_logging) {
							_file_fullpath.get().emptify(); // clear buffer
							SmplBuf_ByteSL<64> _txt_date;   // date text "YYYYMMDD_hhmmss"
							SmplBuf_WChar _file_name;       // "twestage_YYYYMMDD_hhmmss.log"

							// create time text as "YYYYMMDD_hhmmss" format
#if (defined(_MSC_VER) || defined(__MINGW32__))
							SYSTEMTIME sysTime;
							GetLocalTime(&sysTime);

							_txt_date  << printfmt("%04d%02d%02d",
													sysTime.wYear, sysTime.wMonth, sysTime.wDay)
									   << printfmt("-%02d%02d%02d",
													sysTime.wHour, sysTime.wMinute, sysTime.wSecond);							
#elif defined(__APPLE__) || defined(__linux)
							time_t rawtime;
							struct tm* info;
							time(&rawtime);
							info = localtime(&rawtime);

							_txt_date  << printfmt("%04d%02d%02d",
													(info->tm_year + 1900), (info->tm_mon + 1), info->tm_mday)
									   << printfmt("-%02d%02d%02d",
													info->tm_hour, info->tm_min, info->tm_sec);

#endif
							// create filename and fullpath
							_file_name << LOG_FINENAME << '_' << (const char*)_txt_date.c_str() << '.' << LOG_FILEEXT;
							_file_fullpath << make_full_path(the_cwd.get_dir_log(), _file_name);

							// open log file
							try {
								if (the_cwd.get_dir_log().length() == 0) throw nullptr;

								_file_buf.reset(new std::filebuf());
								_file_buf->open((const char*)_file_fullpath.c_str(), std::ios::binary |std::ios::app);
								_file_os.reset(new std::ostream(_file_buf.get()));

								// output timestamp string as the first line.
								*_file_os << "[" << (const char*)_txt_date.c_str() << "]" << std::endl;

								_b_logging = true;
							}
							catch (...) {
								_file_buf.reset();
								_file_os.reset();
								_b_logging = false;

								SDL_ShowSimpleMessageBox(
									SDL_MESSAGEBOX_INFORMATION,
									"TWELITE Stage",
									"ログファイルの作成に失敗しました",
									gWindow
								);
							}
						}
						else {
							// close log file
							if(_file_buf) _file_buf->close();
							_file_os.reset();
							_file_buf.reset();

							_b_logging = false;

							// open a log file
							SDL_Delay(100);

							// open log file
							shell_open_default(_file_fullpath.c_str());
						}

						update_help_screen();

						bhandled = true;
					}
				}
				break;

			default:
				break;
			}

			if (bhandled && IS_N_ALTDOWN_SHOWN_OR_ONKEY(nAltDown))
				nAltDown = g_enable_fade_effect ? N_ALTDOWN_FADE_COUNT_MAX : N_ALTDOWN_HIDE; // HIDE HELP SCREEN
		}

		if (e.type == SDL_KEYUP) {
			if (!(e.key.keysym.mod & (KMOD_STG))) {
				nAltState = 0;
				if (IS_N_ALTDOWN_SHOWN_OR_ONKEY(nAltDown)) {
					nAltDown = g_enable_fade_effect ? N_ALTDOWN_FADE_COUNT_MAX : N_ALTDOWN_HIDE; // HIDE HELP SCREEN
				}
			}
		}
	}

	void _render_main_screen_copy_buffer(void* mPixels) {
		if (render_mode_m5_main == 0) {
			for (int y = 0; y < M5_LCD_HEIGHT; y++) {
				if (M5.Lcd.update_line(y)) {
					uint32_t* p1 = (uint32_t*)mPixels + (y * M5_LCD_WIDTH * 4);
					uint32_t* p2 = p1 + (M5_LCD_WIDTH * 2);

					for (int x = 0; x < M5_LCD_WIDTH; x++) {
						auto c = M5.Lcd.get_pt(x, y);

						// RENDER LIKE LCD
						draw_point(p1, c);
						draw_point(p1 + 1, c, 192);
						draw_point(p2, c, 128);
						draw_point(p2 + 1, c, 128);

						p1 += 2;
						p2 += 2;
					}
				}
			}
		}
		else if (render_mode_m5_main == 1) {
			for (int y = 0; y < M5_LCD_HEIGHT; y++) {
				if (M5.Lcd.update_line(y)) {
					uint32_t* p1 = (uint32_t*)mPixels + (y * M5_LCD_WIDTH * 4);
					uint32_t* p2 = p1 + (M5_LCD_WIDTH * 2);

					for (int x = 0; x < M5_LCD_WIDTH; x++) {
						RGBA c = M5.Lcd.get_pt(x, y);

						draw_point(p1, c);
						draw_point(p2, c, 128);

						p1 += 1;
						p2 += 1;
					}
				}
			}
		}
		else if (render_mode_m5_main == 2) {
			for (int y = 0; y < M5_LCD_HEIGHT; y++) {
				if (M5.Lcd.update_line(y)) {
					uint32_t* p1 = (uint32_t*)mPixels + (y * M5_LCD_WIDTH * 2);
					uint32_t* p2 = p1 + (M5_LCD_WIDTH * 1);

					for (int x = 0; x < M5_LCD_WIDTH; x++) {
						auto c = M5.Lcd.get_pt(x, y);

						// RENDER BLUR
						draw_point(p1, c);
						draw_point(p2, c);

						p1 += 1;
						p2 += 1;
					}
				}
			}

		}
		else if (render_mode_m5_main == 3) {
			for (int y = 0; y < M5_LCD_HEIGHT; y++) {
				if (M5.Lcd.update_line(y)) {
					uint32_t* p1 = (uint32_t*)mPixels + (y * M5_LCD_WIDTH * 4);
					uint32_t* p2 = p1 + (M5_LCD_WIDTH * 2);

					for (int x = 0; x < M5_LCD_WIDTH; x++) {
						auto c = M5.Lcd.get_pt(x, y);

						// RENDER LIKE DIGITAL TEXTURE
						draw_point(p1, c);
						draw_point(p1 + 1, c);
						draw_point(p2, c);
						draw_point(p2 + 1, c);

						p1 += 2;
						p2 += 2;
					}
				}
			}
		}
	}

	void render_main_screen(bool b_background = false) {
		// texture source rect
		const SDL_Rect* p_srcrect = NULL;

		// the start of texure update by memory update.
		void* mPixels;
		int mPitch;
		SDL_LockTexture(mTexture, NULL, &mPixels, &mPitch);

		// render
		if (render_mode_m5_main == 0) {
			static const SDL_Rect srcrect = { 0, 0, M5_LCD_WIDTH * 2, M5_LCD_HEIGHT * 2 };
			p_srcrect = &srcrect;
		}
		else
		if (render_mode_m5_main == 1) {
			static const SDL_Rect srcrect = { 0, 0, M5_LCD_WIDTH, M5_LCD_HEIGHT * 2 };
			p_srcrect = &srcrect;
		}
		else
		if (render_mode_m5_main == 2) {
			static const SDL_Rect srcrect = { 0, 0, M5_LCD_WIDTH, M5_LCD_HEIGHT };
			p_srcrect = &srcrect;
		}
		else
		if (render_mode_m5_main == 3) {
			static const SDL_Rect srcrect = { 0, 0, M5_LCD_WIDTH * 2, M5_LCD_HEIGHT * 2 };
			p_srcrect = &srcrect;
		}

		if (!g_app_busy) {
			if (auto l = TWE::LockGuard(gMutex_Render, 32)) {
				_render_main_screen_copy_buffer(mPixels);
			}
			else {
				// timeout, but perform transferring app screen buffer anyway.
				_render_main_screen_copy_buffer(mPixels);
				WrtCon << "!";
			}
		}
		
		// the end of texture update by memory update
		SDL_UnlockTexture(mTexture);

		//Reset render target
		SDL_SetRenderTarget(gRenderer, nullptr);

		//Show rendered to texture
		const SDL_Rect dstrect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };

		// calculate mainscreen alpha
		//  if the app is in bg, render darker.
		uint8_t alpha = 0xFF;
		if (b_background) {
			backgound_render_count = 4;
		}
		else {
			if (backgound_render_count > 0) backgound_render_count--;
		}

		if (backgound_render_count) alpha = 160;

		// face effect on exit.
		if (quit_loop_count >= 0) {
			if (backgound_render_count) backgound_render_count = -1; // keep background alpha (refrain from being lighter again).
			alpha = quit_loop_count * alpha / QUIT_LOOP_COUNT_MAX;	
		}

		auto t_now = SDL_GetTicks();
		if (g_app_busy && t_now - g_app_busy_tick > 1000) {
			alpha /= 2;
		}

		SDL_SetTextureColorMod(mTexture, alpha, alpha, alpha);
		SDL_RenderCopy(gRenderer, mTexture, p_srcrect, &dstrect);
	}

	void render_texte_box() {
		if (nTextEditing && _nTextEdtLen > 0) {
			SDL_Rect dstrect_sub = { 0, 0, M5_LCD_TEXTE_WIDTH, M5_LCD_TEXTE_HEIGHT };
			SDL_Rect srcrect_sub = { 0, 0, M5_LCD_TEXTE_WIDTH, M5_LCD_TEXTE_HEIGHT };

			if (_nTextEdtLen == 1) {
				dstrect_sub.w = 16;
				srcrect_sub.w = 16;
			}

			// the start of texure update by memory update.
			void* mPixels;
			int mPitch;
			SDL_LockTexture(mTexture_texte, NULL, &mPixels, &mPitch);
			for (int y = 0; y < M5_LCD_TEXTE_HEIGHT; y++) {
				if (M5_TEXTE.Lcd.update_line(y)) {
					uint32_t* p1 = (uint32_t*)mPixels + (y * M5_LCD_TEXTE_WIDTH);

					for (int x = 0; x < M5_LCD_TEXTE_WIDTH; x++) {
						auto c = M5_TEXTE.Lcd.get_pt(x, y);
						draw_point(p1, c);
						p1++;
					}
				}
			}
			SDL_UnlockTexture(mTexture_texte);

			uint8_t alpha = (nTextEditing < 0) ? (-nTextEditing * 0xc0) / 32 : 0xc0;

			SDL_SetRenderTarget(gRenderer, NULL);
			SDL_SetTextureBlendMode(mTexture_texte, SDL_BLENDMODE_BLEND);
			SDL_SetTextureAlphaMod(mTexture_texte, alpha);
			SDL_RenderCopy(gRenderer, mTexture_texte, &srcrect_sub, &dstrect_sub);

			if (nTextEditing < 0) {
				nTextEditing++;
				if (nTextEditing == 0) _nTextEdtLen = 0; // set to 0
			}
		}
	}

	void render_help_screen() {
		if (nAltDown != N_ALTDOWN_HIDE) {
			const SDL_Rect dstrect_sub = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
			const SDL_Rect srcrect_sub = { 0, 0, M5_LCD_SUB_WIDTH, M5_LCD_SUB_HEIGHT };
			
			// the start of texure update by memory update.
			void* mPixels;
			int mPitch;
			SDL_LockTexture( mTexture_sub, NULL, &mPixels, &mPitch );
			for (int y = 0; y < M5_LCD_SUB_HEIGHT; y++) {
				if (M5_SUB.Lcd.update_line(y)) {
					uint32_t* p1 = (uint32_t*)mPixels + (y * M5_LCD_SUB_WIDTH);

					for (int x = 0; x < M5_LCD_SUB_WIDTH; x++) {
						auto c = M5_SUB.Lcd.get_pt(x, y);

						draw_point(p1, c);
						p1++;
					}
				}
			}
			SDL_UnlockTexture(mTexture_sub);

			uint8_t alpha;
			if (g_enable_fade_effect) {
				alpha = (nAltDown < 0) ? (-nAltDown * 0xc0) / 8 : 0xc0;
			}
			else {
				alpha = 0xff;
			}

			SDL_SetRenderTarget(gRenderer, NULL);
			SDL_SetTextureBlendMode(mTexture_sub, SDL_BLENDMODE_BLEND);
			SDL_SetTextureAlphaMod(mTexture_sub, alpha);
			SDL_RenderCopy(gRenderer, mTexture_sub, &srcrect_sub, &dstrect_sub);

			if (nAltDown < 0) nAltDown++;
		}
	}

	void log_write(char_t c) {
		if (_b_logging) {
			*_file_os << c;
		}
	}

	void setup() {
		init_sdl();
		init_sdl_sub();
	}

	void loop() {
		SDL_Event e;
		SDL_Point screenCenter = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };

		while (g_quit_sdl_loop == false || quit_loop_count > 0) {
			//Event handler
			while (SDL_PollEvent(&e) != 0) {
				handle_sdl_event(e);
			}

			// if true, render screen
			bool render = true;

			if (_is_window_hidden) {
				render = false;
			}
			else if (!_is_get_focus) {
				_render_cnt++;

				if (!_is_window_hidden) {
					render = ((_render_cnt & 15) == 15); // every 4 frames (background)
				}
				else {
					render = ((_render_cnt & 63) == 63); // evert frames (normally, in focus)
				}
			}

			if (render) {
				// Update Alt Screen	
				static int ser2handle = -1;
				if (Serial2.get_handle() != ser2handle) {
					ser2handle = Serial2.get_handle();
					update_help_screen();
				}
				sub_screen.refresh(); // update sub screen
				sub_screen_tr.refresh();
				sub_screen_br.refresh();
				sub_textediting.refresh();

				// clear back margin
				if (_bfullscr > 0 && (SCREEN_POS_X != 0 || SCREEN_POS_Y != 0)) {
					Uint8 r, g, b, a;
					SDL_GetRenderDrawColor(gRenderer, &r, &g, &b, &a);
					SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
					SDL_RenderClear(gRenderer);
					SDL_SetRenderDrawColor(gRenderer, r, g, b, a);
				}

				// render M5stack area
				render_main_screen(!_is_get_focus);

				if (_is_get_focus) {
					/* RENDER ALT SCREEN (HELP, SOME OPERATION) */
					render_help_screen();

					// TEXTEDITING box
					render_texte_box();

					/* RENDER BOTTONS */
					sp_btn_A->render_sdl(gRenderer);
					sp_btn_B->render_sdl(gRenderer);
					sp_btn_C->render_sdl(gRenderer);

					// BUTTON QUIT
					sp_btn_quit->render_sdl(gRenderer);
				}

				// Wait vsync and render screen.
				SDL_RenderPresent(gRenderer);
			}

			// delay some for next tick.
			// - if rendered, SDL_RenderPreset() will wait for VSYNC.
			// - otherwise keep tick close to LOOP_MS.
			if (!render) {
				const int LOOP_MS = 15;
				uint32_t u32tick_now = SDL_GetTicks();
				int delay = (u32tick_now - _u32tick_sdl_loop_head);

				if (delay >= LOOP_MS) {
					delay = -1; // already passed LOOP_MS
				}
				else if (delay >= 0) {
					delay = LOOP_MS - delay;
				}
				else {
					delay = LOOP_MS; // error?
				}
				if (delay > 0) SDL_Delay(delay);
			}

#if 0
			// DEBUG MESSAGE FOR CHECKING LOOP PERIOD
			{
				static uint32_t last_tick;
				con_screen << printfmt("[%d,%d]", _u32tick_sdl_loop_head - last_tick, SDL_GetTicks() - _u32tick_sdl_loop_head);
				last_tick = _u32tick_sdl_loop_head;
			}
#endif

			// set timing
			_u32tick_sdl_loop_head = SDL_GetTicks();

#if MWM5_SDL2_USE_MULTITHREAD_RENDER == 0
			// RUN SKETCH
			if (g_quit_sdl_loop == false) ::s_sketch_loop(); // run the app loop in the same thread.
#endif

			if (quit_loop_count == -1 && g_quit_sdl_loop) {
				// exitting message
				con_screen << crlf << "exiting";

				if (g_enable_fade_effect) {
					quit_loop_count = QUIT_LOOP_COUNT_MAX;
				}
				else {
					quit_loop_count = 0; // exit the loop.
					break;
				}
			}
			else if (quit_loop_count >= 0) {
				con_screen << '.';
				con_screen.refresh();

				if (quit_loop_count > 0) quit_loop_count--;
			}
		}
	}
};
std::unique_ptr<app_core_sdl> the_app_core;

/**
 * @fn	int exit_err(const char* msg, const char * msg_param = NULL)
 *
 * @brief	Exit with an error
 *
 * @param	msg		 	The message.
 * @param	msg_param	(Optional) The message parameter.
 *
 * @returns	An int.
 */
static void exit_err(const char* msg, const char * msg_param) {
	fprintf(stderr, "ERROR: ");
	fprintf(stderr, msg, msg_param);
	fprintf(stderr, "\r\n");

#ifdef __APPLE__
	_exit(1);
#else
	exit(1);
#endif
}

// handle terminal Ctrl+C
static void signalHandler( int signum ) {  
	if (signum == SIGINT 
#if defined(__APPLE__) || defined(__linux)
		|| signum == SIGQUIT
#endif	
		) {
		g_quit_sdl_loop = true;
	}
	// fprintf(stderr, "signal handled = %d\n", signum);
}

// initialize
static void s_init() {
	// set dir
	the_cwd.begin();
#ifndef _DEBUG
	the_cwd.change_dir(the_cwd.get_dir_exe());
#endif

	// find physical CPU count
	_physical_cpu_count = _Get_Physical_CPU_COUNT_query_by_external_command();

	// DLL delay loading (only for VC++)
#if defined(_MSC_VER)  // || defined(__MINGW32__) // not for MINGW32, because /DELAYLOAD is not supported.
	SetDllDirectoryW(make_full_path(the_cwd.get_dir_exe(), L"dll").c_str()); // DLL directory, needs to specify /DELAYLOAD (VC++).
#endif

	// capture Ctrl+C on the console
	signal(SIGINT, signalHandler);
#if defined(__APPLE__) || defined(__linux)
	signal(SIGQUIT, signalHandler);
#endif

	// the OS dependent initialize
	TWESYS::SysInit();

	// prepare serial port
#if defined(__APPLE__) && defined(MWM5_SERIAL_NO_FTDI)
	{	// load serial server (if not preset, program cannot be run)
		SmplBuf_ByteSL<1024> buff;
		buff << make_full_path(the_cwd.get_dir_exe(), L"TWELITE_Stage/bin/sersrv_ftdi.command");

		if (!std::filesystem::exists(buff.c_str())) { // find current dir 
			buff.clear();
			buff << make_full_path(the_cwd.get_dir_exe(), "sersrv_ftdi.command");
		}

		if (!std::filesystem::exists(buff.c_str())) {
			exit_err("Cannot find Serial Server command (sersrv_ftdi.command)!");
		}
		else {
			if (!Serial2.open_sersrv(buff.c_str())) {
				exit_err("Cannot open Serial Server command (sersrv_ftdi.command)!");
			}
		}
	}
#endif
	Serial2.list_devices();
	Serial2.set_hook_on_write(s_ser_hook_on_write);
}

static void s_init_sdl() {
	SDL_SetMainReady();

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER 
				                | (the_pref.game_controller ? SDL_INIT_GAMECONTROLLER : 0)
	            ) < 0) {
		exit_err("SDL_Init() [%s]");
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	switch (the_pref.render_engine) {
	case 1:
		SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
		DBGOUT("\nSDL_HINT_RENDER_DRIVER=opengl");
		break;
	case 2:
#if defined(_MSC_VER) || defined(__MINGW32__)
		SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d");
		DBGOUT("\nSDL_HINT_RENDER_DRIVER=direct3d");
#elif defined(__APPLE__)
		SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
		DBGOUT("\nSDL_HINT_RENDER_DRIVER=metal");
#elif defined(__linux)
		DBGOUT("\nSDL_HINT_RENDER_DRIVER is not set");
#endif
		break;
	case 3:
		SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
		DBGOUT("\nSDL_HINT_RENDER_DRIVER=software");
		break;
	default:
		DBGOUT("\nSDL_HINT_RENDER_DRIVER is not set");
	}

	char title[256];
	snprintf(title, 256, "%s (v%d-%d-%d)", MWM5_APP_NAME, MWM5_APP_VERSION_MAIN, MWM5_APP_VERSION_SUB, MWM5_APP_VERSION_VAR);
	DBGOUT("\nTILTLE=%s",title);

	// Create Main window.
	gWindow = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
										SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN
										); // | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if (gWindow == NULL)
		exit_err("SDL_CreateWindow()");
	
	// system icon
#if defined(_MSC_VER) || defined(__MINGW32__)
	// for Windows, choose 32x32 for Window ICON
	g_pixdata_icon_win.reset(new uint32_t[32 * 32]);
	prepare_icon_data(g_pixdata_icon_win.get(), 32, 32, pix_icon_win_32);
	gSurface_icon_win = SDL_CreateRGBSurfaceFrom(g_pixdata_icon_win.get(), 32, 32, 32, 32 * 4, 0xFF000000, 0xFF0000, 0xFF00, 0xFF);

	if (gSurface_icon_win == NULL) exit_err("SDL_CreateRGBSurfaceFrom()");
	SDL_SetWindowIcon(gWindow, gSurface_icon_win);
#elif defined(__APPLE__) || defined(__linux)
	// for Linux/osx choose 128x128.
	g_pixdata_icon_win.reset(new uint32_t[128 * 128]);
	prepare_icon_data(g_pixdata_icon_win.get(), 128, 128, pix_icon_win_128);
	gSurface_icon_win = SDL_CreateRGBSurfaceFrom(g_pixdata_icon_win.get(), 128, 128, 32, 128 * 4, 0xFF000000, 0xFF0000, 0xFF00, 0xFF);

	if (gSurface_icon_win == NULL) exit_err("SDL_CreateRGBSurfaceFrom()");
	SDL_SetWindowIcon(gWindow, gSurface_icon_win);
#endif

	// Renderer
	// gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
	gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	if (gRenderer == NULL)
		exit_err("SDL_CreateRenderer()");

	// user events
	if ((g_sdl2_user_event_type = SDL_RegisterEvents(1)) == -1) {
		exit_err("SDL_RegisterEvents()");
	}

#if MWM5_SDL2_USE_MULTITHREAD_RENDER == 1 && MWM5_USE_SDL2_MUTEX == 1
	gMutex_Render = SDL_CreateMutex();
	if (gMutex_Render == nullptr) {
		exit_err("SDL_CreateMutex()");
	}
#endif
}

static void s_sketch_setup() {
	// external setup() procedure
	::setup();
}

static void s_sketch_loop() {
	// update tick counter
	u32TickCount_ms = TWESYS::u32GetTick_ms();

	// get serial2 buffer
	int nSer2 = Serial2.update();

	// console test
	if (!twe_prog.is_protocol_busy()) {
		// handle serial input from TWE
		if (nSer2 >= 1) {
			for (int i = 0; i < nSer2; i++) {
				char_t c = Serial2._get_last_buf(i);
				con_screen << c;
				the_app_core->log_write(c);
			}
		}

		// handle console input
		while (1) {
			int c = con_keyboard.get_a_byte();
			if (c == -1) break;

#if 0 && defined(__APPLE__)
			// APPLE TERMINAL will input latin char when pressed Alt+?.
			// con_screen << printfmt("[%02X]", c); // debug
			// ALT+R
			if (c == 0xC2) {
				c = con_keyboard.get_a_byte();
				if (c == 0xAE) {
					con_screen << crlf << "opt+R pressed, reset module";
					con_screen.force_refresh();
					
					if (Serial2.is_opened()) {
						the_generic_ops.module_reset();
					}	
				}
				c = -1;
			}
			
			// Alt+P
			if (c == 0xCF) {
				c = con_keyboard.get_a_byte();
				if (c == 0x80) {
					if (Serial2.is_opened()) {
						con_screen << crlf << "opt+C pressed, + + +";
						con_screen.force_refresh();

						the_generic_ops.type_plus3();
					}
				}
				c = -1;
			}

			// Alt+X
			if (c == 0xE2) {
				c = con_keyboard.get_a_byte();
				if (c == 0x89) {
					c = con_keyboard.get_a_byte();
					if (c == 0x88) {
						con_screen << crlf << "opt+X pressed, ";
						con_screen.force_refresh();
						g_quit_sdl_loop = true;
					}
				}
				c = -1;
			}
#endif
			if (c >= 0) the_sys_keyboard.push(c & 0xFF); // WrtTWE << char_t(c & 0xFF);
		}

		con_screen.refresh();

		::loop(); // external loop()
	} else {
		// is in bootloader protocol.
		// too make it faster, process a bulk of command at one time, otherwise it's affected by VSYNC wait.
		static int ct = 0; // bulk process counter
		
		// more loop
#if MWM5_SDL2_USE_MULTITHREAD_RENDER == 0
		do {
			if (Serial2.update() > 0) ct++;
			::loop();

			if (ct >= 4) { // process 4 commands at every loop.
				ct = 0;
				break;
			}
		} while (twe_prog.is_protocol_busy());
#else 
		::loop();
#endif
	}	
}

/**
 * @fn	static void s_ser_hook_on_write(const uint8_t p, int len)
 *
 * @brief	log input chars passed to TWELITE with quote character.
 *
 * @param	p  	An uint8_t to process.
 * @param	len	The length.
 */
static void s_ser_hook_on_write(const uint8_t *p, int len) {
#if defined(_MSC_VER) || defined(__MINGW32__)
	const char_t cIn = char_t(0xA2);  // ｢ (for Japanese windows, CP932 is mostly assumed)
	const char_t cOut = char_t(0xA3); // ｣
#elif defined(__APPLE__) || defined(__linux)
	const char_t cIn = 0xAB;  // « (for others, chose quote char from latin1ex part)
	const char_t cOut = 0xBB; // »
#endif

	if (!twe_prog.is_protocol_busy()) {
		the_app_core->log_write(cIn);
		for (int i = 0; i < len; i++)
			the_app_core->log_write(p[i]);
		the_app_core->log_write(cOut);
	}
}

/**
 * @fn	static void s_getopt(int argc, char* args[])
 *
 * @brief	getopts using oss_getopt() (simple implementation of getopt())
 *
 * @param 		  	argc	The argc.
 * @param [in,out]	args	If non-null, the arguments.
 */
static void s_getopt(int argc, char* args[]) {
	// clear preference data and set defaults
	memset(&the_pref, 0, sizeof(the_pref));
	the_pref.game_controller = MWM5_USE_GAMECONTROLLER;

	int opt = 0;
	ts_opt_getopt* popt = oss_getopt_ref();

    while ((opt = oss_getopt(argc, args, "E:R:")) != -1) {
        switch (opt) {
		case 'E': // effects
			{
				int optval = atoi(popt->optarg);
				switch (optval) {
				case 0: g_enable_fade_effect = false; break;
				case 1: g_enable_fade_effect = true; break;
				default: g_enable_fade_effect = (MWM5_ENABLE_FADE_EFFECT == 1);
				}
			}
			break;
        case 'R': // Render engine (0:default 1:opengl 2:metal)
            the_pref.render_engine = atoi(popt->optarg);
            break;

		case 'J':
			the_pref.game_controller = 1;
			break;

        default: /* '?' */
            fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n",
                    args[0]);
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * @fn	void push_window_event(int32_t code, void* data1, void* data2)
 *
 * @brief	Pushes a window event
 *
 * @param 		  	code 	The code.
 * @param [in,out]	data1	If non-null, the first data.
 * @param [in,out]	data2	If non-null, the second data.
 */
void push_window_event(int32_t code, void* data1, void* data2) {
    SDL_Event event;
    SDL_memset(&event, 0, sizeof(event)); /* or SDL_zero(event) */
    event.type = g_sdl2_user_event_type;
    event.user.code = code;
    event.user.data1 = data1;
    event.user.data2 = data2;
    SDL_PushEvent(&event);
}

/**
 * @fn	int TWESYS::Get_Logical_CPU_COUNT()
 *
 * @brief	Gets logical CPU count
 *
 * @returns	The logical CPU count.
 */
int TWESYS::Get_Logical_CPU_COUNT() {
	return SDL_GetCPUCount();
}


/**
 * @fn	int _Get_Physical_CPU_COUNT_init()
 *
 * @brief	Gets physical CPU count initialize
 *
 * @returns	The physical CPU count initialize.
 */
static int _Get_Physical_CPU_COUNT_query_by_external_command() {
	int cpu_count = 0;
#if defined(ESP32)
	return 1; // actuall 2 cores
#elif defined(_MSC_VER) || defined(__MINGW32__)
	/*
		@echo off
		for /f "tokens=*" %%f in ('wmic cpu get NumberOfCores /value ^| find "="') do set %%f
	*/
	TweCmdPipe cmd("wmic cpu get NumberOfCores /value");
	if (!cmd) return 0;
	if (cmd.available()) {
		SmplBuf_Byte buff;

		while (cmd.readline(buff)) {
			// remove endl.
			remove_endl(buff);

			if (buff.size() > 0) {
				static const char KEYSTR[] = "NumberOfCores=";
				if (beginsWith_NoCase(buff, KEYSTR)) {
					auto ptr = buff.c_str();
					cpu_count = atoi(ptr + sizeof(KEYSTR) - 1);
				}
			}
		}
	}
	if (cmd) cmd.close();
	return cpu_count;
#elif defined(__APPLE__) || defined(__linux)
# if defined(__APPLE__)
	TweCmdPipe cmd("sysctl -n hw.physicalcpu");
# elif defined(__linux)
#  if defined(MWM5_BUILD_RASPI)
	TweCmdPipe cmd("awk '/^processor/ { if (CPU<$3) CPU=$3 } END{print (CPU+1)}' < /proc/cpuinfo");
#  else
	TweCmdPipe cmd("grep \"^cpu.cores\" /proc/cpuinfo | sed -e \"s/ [\\t]//g\" -e \"s/cpucores://\"");
#  endif
# endif
	if (cmd.available()) {
		SmplBuf_Byte buff;

		while (cmd.readline(buff)) {
			// remove endl.
			remove_endl(buff);
			if (buff.size() > 0) {
				auto ptr = buff.c_str();
				cpu_count = atoi(ptr);
				if (cpu_count > 0 && cpu_count <= 128) break;
			}
		}
	}

	if (cmd) cmd.close();
	return cpu_count;
#endif
}

int TWESYS::Get_Physical_CPU_COUNT() {
	return _physical_cpu_count;
}

/**
 * @fn	int TWESYS::Get_CPU_COUNT()
 *
 * @brief	Gets CPU count
 *
 * @returns	The CPU count, limited 16 cpus as maximum.
 */
int TWESYS::Get_CPU_COUNT() {
	int n = 0;

	if (_physical_cpu_count) n = _physical_cpu_count;
	else n = TWESYS::Get_Logical_CPU_COUNT() / 2;

	if (n == 0) n = 1;
	if (n > 16) n = 16;

	return n;
}


#if MWM5_SDL2_USE_MULTITHREAD_RENDER == 1
/**
 * @fn	static uint32_t callbackTimerApp(uint32_t interval, void* param)
 *
 * @brief	Callback timer for application loop.
 * 			This callback is run on a separate thread.
 *
 * @param 		  	interval	The interval.
 * @param [in,out]	param   	If non-null, the parameter.
 *
 * @returns	An uint32_t.
 */
static uint32_t callbackTimerApp(uint32_t interval, void* param) {
	const int T_LOOP = 1000/166; // 166Hz=6ms

	if (g_quit_sdl_loop) return 0;

	uint32_t t_now = SDL_GetTicks();
	static uint32_t t_last;

	uint32_t t_d = t_now - t_last;
	t_last = t_now;

	//printf("\n[%02d/%07d]", t_d, t_now);

	if (twe_prog.is_protocol_busy()) {
		// loop 8 times at once
		auto l = TWE::LockGuard(gMutex_Render, 32);
		if (!l) WrtCon << "*"; // lock fails (timeout)
		for (int i = 0; i < 8; i++) {
			::s_sketch_loop(); // loop last (w/ keeping lock)
		}

		return 1;
	} else {
		{
			auto l = TWE::LockGuard(gMutex_Render, 32);
			if (!l) WrtCon << "*"; // lock fails (timeout)
			::s_sketch_loop(); // loop last
		}

		return (t_d == T_LOOP || t_d >= T_LOOP) ? T_LOOP : T_LOOP - t_d;
	}

	return T_LOOP;
}
#endif

/**
 * @fn	int main(int argc, char* args[])
 *
 * @brief	Main entry-point for this application
 *
 * @param	argc	The number of command-line arguments provided.
 * @param	args	An array of command-line argument strings.
 *
 * @returns	Exit-code for the process - 0 for success, else an error code.
 */
int main(int argc, char* args[]) {
	// check command line agrs
	s_getopt(argc, args);

	printf("\033[2J\033[H");

	// initialize
	the_app_core.reset(new app_core_sdl());
	s_init();
	s_init_sdl();
	
	// console setup
	con_screen.setup();
	con_screen << printfmt("*** TWELITE STAGE (v%d-%d-%d) ***", MWM5_APP_VERSION_MAIN, MWM5_APP_VERSION_SUB, MWM5_APP_VERSION_VAR) << crlf;

	// init SDL instance
	the_app_core->setup();

	// call sketch setup();
	s_sketch_setup();

#if MWM5_SDL2_USE_MULTITHREAD_RENDER == 1
	// main loop is invoked by SDL Timer.
	SDL_AddTimer(10, callbackTimerApp, nullptr);
#endif

	// SDL MainLoop
	the_app_core->loop();

	// delete app instance
	TWE::the_app._destroy_app_instance();

	// force exit
	auto func = [](uint32_t timeout) {
		#if defined(_MSC_VER) || defined(__MINGW32__)
		Sleep(timeout);
		_exit(0);
		#elif defined(__APPLE__) || defined(__linux)
		usleep(timeout * 1000UL);

		// force terminate here!
		_exit(0);
		#endif
	};
	std::thread th_exit(func, 1000);

	// delete instance
	try {
		the_app_core.reset();
	}
	catch (...) {
		; /* exception */
	}
	
	// on exit 
	con_screen.close_term(); // shall take the screen back before calling _exit().

#if defined(__APPLE__) && defined(MWM5_SERIAL_NO_FTDI)
	// close serial server
	Serial2.close_sersrv();
#endif

#if defined(__APPLE__) || defined(__linux)
	// clear console screen
	int apiret = system("clear"); (void)apiret;
#endif

	th_exit.join();

	return 0;
}

#endif // WIN/MAC
