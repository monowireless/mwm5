#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <mwm5.h>
#include <M5Stack.h>

#include "common.h"

#ifndef ESP32
#include <cstdio>
#include <cstdlib>
#include <regex>
#endif

class App_FirmProg : public TWE::APP_DEF {
public:
	static const int APP_ID = int(E_APP_ID::FIRM_PROG);
	typedef App_FirmProg self_type;

	int get_APP_ID() { return APP_ID; }
	const wchar_t* get_APP_INIT_MSG() { return L""; }
	
private:

	// setup procedure
	void setup_screen();

	// update screen
	void screen_refresh();

	// change screen font of `the_screen'
	void change_screen_font();

	// change screen color of `the_screen'
	void change_screen_color();

private:
	// Serial Parser
	AsciiParser parse_ascii;

	// top bar
	TWETerm_M5_Console the_screen_t; // init the screen.

	// main screen
	TWETerm_M5_Console the_screen; // init the screen.

	// main screen_list
	TWETerm_M5_Console the_screen_l; // init the screen.

	// bottom area (for debug information)
	TWETerm_M5_Console the_screen_b; // init the screen.

	// bottom area (for debug information)
	TWETerm_M5_Console the_screen_c; // init the screen.

	// default color
	uint16_t default_bg_color;
	uint16_t default_fg_color;

	// true if running
	bool _b_running_tweprog;

	// list view
	TWE_ListView _listFiles;

	// dir/filenames
	static const wchar_t _dirname_regular[];

#ifndef ESP32
	static SmplBuf_WChar _dirname_drop; // dropped dirname
	bool _b_drop_available;

	static int _last_menu_number;
	static SmplBuf_WChar _build_name;
	static SmplBuf_WChar _build_workspace;
	static SmplBuf_WChar _build_project;
	static SmplBuf_WChar _build_project_prev;
#endif

	static SmplBuf_WChar _firmfile_dir;
	static SmplBuf_WChar _firmfile_name;
	static SmplBuf_WChar _firmfile_disp;
	static TWE::TweProg::E_MOD_TYPE _firmfile_modtype;

	// APPS
	TWE::APP_MGR _subscr;

	// CONSTRUCTOR OPTION
	int _n_opt_construct;

public:
	App_FirmProg(int opt=0x0)
		: parse_ascii(256)
#if M5_SCREEN_HIRES == 0
		, the_screen(64, 20, { 0,  18, 320, 144 }, M5)
		, the_screen_l(64, 2, { 0,  18 + 144, 320, 28 }, M5)
		, the_screen_t(64, 1, { 0, 0, 320, 18 }, M5)
		, the_screen_b(64, 4, { 0, 18 + 172, 320, 40 }, M5)
		, the_screen_c(64, 1, { 0, 18 + 192 + 20, 320, 10 }, M5)
#elif M5_SCREEN_HIRES == 1
		, the_screen_t(64, 1, { 0,   0, 640,  24 }, M5)
		, the_screen(80, 25, { 0,  24, 640, 352 }, M5)
		, the_screen_l(80, 4, { 0, 376, 640,  32 }, M5)
		, the_screen_b(120, 4, { 0, 408, 640,  48 }, M5)
		, the_screen_c(64, 1, { 0, 456, 640,  24 }, M5)
#endif
		, default_bg_color(0)
		, default_fg_color(0)
		, _b_running_tweprog(false)
#ifdef ESP32
		, _listFiles(16, 64) // max files & max text len
#else
		, _listFiles(64, 64) // max file & max text len
		//, _dirname_drop(TWE::TWE_FILE_NAME_MAX)
		, _b_drop_available(false)
#endif
		, _subscr()
		, _n_opt_construct(opt) // constuctor option
	{
		set_appobj((void*)static_cast<ITerm*>(&the_screen)); // store app specific obj into APPDEF class storage.
		SubScreen::_parent = this;
	}

	~App_FirmProg() {}

	void setup();

	void loop();


private: // SUBSCREEN
	enum class E_SUBSCREEN {
		MOD_IDENTIFY = 1,
		OPEN_MENU,
		FILE_BROWSE,
		ACT_BUILD,
		FILE_PROG,
		FATAL_ERROR
	};

	struct SubScreen {
		static self_type* _parent;

		static const int EXIT_BACK_TO_MENU = -1;
		static const int EXIT_NOMAL = 0;
		static const int EXIT_NEXT = 1;
		static const int EXIT_NEXT2 = 2;
		static const int EXIT_NEXT3 = 3;
		static const int EXIT_PREV = 4;
		static const int EXIT_WITH_APPID = 6;
		static const int EXIT_ERROR = 7;

		ITerm& the_screen;
		ITerm& the_screen_l;
		ITerm& the_screen_t;
		ITerm& the_screen_b;
		ITerm& the_screen_c;
		TWE_ListView& _listFiles;

		SubScreen()
			: the_screen(_parent->the_screen)
			, the_screen_l(_parent->the_screen_l)
			, the_screen_t(_parent->the_screen_t)
			, the_screen_b(_parent->the_screen_b)
			, the_screen_c(_parent->the_screen_c)
			, _listFiles(_parent->_listFiles)
		{}

		void screen_refresh() {
			_parent->screen_refresh();
		}

		void exit(int n, int next_app = APP_MGR::NEXT_APP_DEFAULT) {
			_parent->_subscr.exit(n, next_app);
		}
	};

	static int change_app(TWE::APP_MGR& sub_app, int next_app, int prev_app, int exit_code);

	class Screen_ModIdentify : public TWE::APP_DEF, public SubScreen {
		// Timeout
		TWESYS::TimeOut _timer;
		bool _b_protocol;
	public:
		static const int SCR_ID = (int)E_SUBSCREEN::MOD_IDENTIFY;
		Screen_ModIdentify() : SubScreen(), _timer(), _b_protocol(false) {}
		void setup();
		void loop();
		static void cb_protocol(
			TweProg::E_ST_TWEBLP cmd,
			int req_or_resp,
			TWE::APIRET evarg,
			TWEUTILS::SmplBuf_Byte& payl,
			void* pobj
		);

		void start_protocol();
	};
	friend class Screen_ModIdentify;

#ifndef ESP32
	class Screen_OpenMenu : public TWE::APP_DEF, public SubScreen {
		TWE_ListView _listMenu;
		static int _i_selected;

	public:
		static const int SCR_ID = (int)E_SUBSCREEN::OPEN_MENU;
		Screen_OpenMenu() : SubScreen(), _listMenu(8, 64) {}
		void setup();
		void loop();

	private:
		void update_dropmenu();
		void update_navigation();

	public:
		static const int MENU_REGULAR_APP = 0;
		static const int MENU_ACT = 1;
		static const int MENU_TWEAPPS = 2;
		static const int MENU_ACT_EXTRA = 3;
		static const int MENU_DROP_DIR = 4;
		static const int MENU_LAST = 5;
		static const int MENU_NONE = -1;
	};
	friend class Screen_OpenMenu;
#endif

	class Screen_FileBrowse : public TWE::APP_DEF, public SubScreen {
		int _error_status;
		TWESYS::TimeOut _exit_timer;

	public:
		static const int SCR_ID = (int)E_SUBSCREEN::FILE_BROWSE;
		Screen_FileBrowse() : SubScreen(), _error_status(0) {}
		void setup();
		void loop();
		bool dispFileName(const wchar_t* name, SmplBuf_WChar& name_disp);

	public:
	};
	friend class Screen_FileBrowse;


	class Screen_FatalError : public TWE::APP_DEF, public SubScreen {
	public:
		static const int SCR_ID = (int)E_SUBSCREEN::FATAL_ERROR;
		Screen_FatalError() : SubScreen() {}
		void setup();
		void loop();
	};
	friend class Screen_FatalError;


	class Screen_FileProg : public TWE::APP_DEF, public SubScreen, public TWE::APP_HNDLR<Screen_FileProg> {
		int _error_status;

		TWESYS::TimeOut _timer_null;
		TWESYS::TimeOut _timer_exit;
		size_t _siz_file;
		bool _b_protocol;
		uint32_t _u32_tickstart;

		TweProg::file_type_shared _file;

	public:
		static const int SCR_ID = (int)E_SUBSCREEN::FILE_PROG;
		Screen_FileProg() : SubScreen(), _error_status(0), _timer_null(), _timer_exit(), _siz_file(0), _b_protocol(false), _u32_tickstart(0), _file() {}
		~Screen_FileProg() { APP_HNDLR::on_close(); }
		void setup();
		void loop();
	
		void hndlr_start(event_type ev, arg_type arg = 0);
		void hndlr_success(event_type ev, arg_type arg = 0);
		void hndlr_error(event_type ev, arg_type arg = 0);
		void hndlr_waitexit(event_type ev, arg_type arg = 0);

		void start_protocol();
		static void cb_protocol(
			TweProg::E_ST_TWEBLP cmd,
			int req_or_resp,
			TWE::APIRET evarg,
			TWEUTILS::SmplBuf_Byte& payl,
			void* pobj
		);
	};
	friend class Screen_FileBrowse;


#ifndef ESP32
	class Screen_ActBuild : public TWE::APP_DEF, public SubScreen, public TWE::APP_HNDLR<Screen_ActBuild> {
		TWESYS::TimeOut _timer_exit;
		SmplBuf_WChar _act_dir;
		SmplBuf_WChar _act_build_file;
		TweCmdPipe _pipe;
		std::regex _re_gcc, _re_link, _re_cfile, _re_target;
		uint32_t _opt;

		TweDesc _desc; // description loaded form 000desc.txt.
	public:
		static const int SCR_ID = (int)E_SUBSCREEN::ACT_BUILD;
		Screen_ActBuild(uint32_t opt = OPT_START_DIR_LIST_ACT) : SubScreen(), _act_dir(), _pipe(), _opt(opt), _desc() {
			the_sys_console.visible(false); // set system console as shell_mode (stop ITerm display)
		}
		~Screen_ActBuild(){
			the_sys_console.visible(true); // set system console under ITerm control.
			APP_HNDLR::on_close();
		}
		void setup();
		void loop();

		void hndlr_actdir(event_type ev, arg_type arg = 0);
		void actdir_update_bottom();
		void hndlr_build(event_type ev, arg_type arg = 0);
		void hndlr_error(event_type ev, arg_type arg = 0);
	
	public:
		static const uint32_t OPT_START_DIR_LIST_ACT = 0x1001;
		static const uint32_t OPT_START_DIR_LIST_TWEAPPS = 0x1002;
		static const uint32_t OPT_START_BUILD_ACT = 0x1011;
		static const uint32_t OPT_START_BUILD_TWEAPPS = 0x1012;
	};
	friend class Screen_ActBuild;
#endif
};